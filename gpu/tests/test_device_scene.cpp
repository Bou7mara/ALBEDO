#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "optix_context.h"
#include "device_scene.h"
#include "rt/scene/scene_node.h"
#include "rt/scene/scene.h"
#include "rt/shapes/triangle.h"

#include <optix_stubs.h>
#include <optix_stack_size.h>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>

using Catch::Matchers::WithinAbs;

namespace {

    struct TestRay {
        rt::Point3f origin;
        rt::Vector3f direction;
        float tMax;
    };

    struct TestHitResult {
        int hit;
        float t;
        rt::Point3f p;
        rt::Normal3f n;
    };

    struct IntersectionTestParams {
        OptixTraversableHandle iasHandle;
        const TestRay* testRays;
        TestHitResult* results;
        unsigned int numTestRays;
    };

    struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) RaygenRecord {
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    };

    struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) MissRecord {
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    };

    struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) HitgroupRecord {
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
        rtx::MeshSbtData data;
    };

    std::vector<char> ReadBinaryFile(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path.string());
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            throw std::runtime_error("Failed to read binary file: " + path.string());
        }
        return buffer;
    }

    std::filesystem::path FindShaderBinary(const std::string& filename) {
        std::vector<std::filesystem::path> candidates = {
            std::filesystem::current_path() / "optixir" / filename,
            std::filesystem::current_path() / filename,
#ifdef OPTIXIR_DIR
            std::filesystem::path(OPTIXIR_DIR) / filename
#endif
        };

        for (const auto& p : candidates) {
            if (std::filesystem::exists(p)) {
                return p;
            }
        }
        throw std::runtime_error("Could not locate OptiX-IR shader file: " + filename);
    }

} // namespace

TEST_CASE("GAS and IAS scene translation parity with CPU BVH", "[gpu][scene][intersection]") {
    // 1. Initialize OptiX & CUDA context
    auto ctx = rtx::OptixContext::Create();
    REQUIRE(ctx != nullptr);
    OptixDeviceContext optixContext = ctx->GetOptixDeviceContext();
    CUstream stream = ctx->GetCudaStream();

    // 2. Construct Scene Graph with Instanced Geometry
    auto mesh = std::make_shared<rt::TriangleMesh>();
    mesh->positions = {
        rt::Point3f(0.0f, 0.0f, 0.0f),
        rt::Point3f(1.0f, 0.0f, 0.0f),
        rt::Point3f(0.0f, 1.0f, 0.0f)
    };
    mesh->normals = {
        rt::Normal3f(0.0f, 0.0f, 1.0f),
        rt::Normal3f(0.0f, 0.0f, 1.0f),
        rt::Normal3f(0.0f, 0.0f, 1.0f)
    };
    mesh->indices = { 0, 1, 2 };

    auto root = std::make_shared<rt::SceneNode>();

    // Instance 1: at z = -2
    auto node1 = std::make_shared<rt::SceneNode>();
    node1->mesh = mesh;
    node1->localTransform = rt::Transform::Translate(rt::Vector3f(0.0f, 0.0f, -2.0f));
    root->children.push_back(node1);

    // Instance 2: sharing same mesh, shifted to x = 3, z = -2
    auto node2 = std::make_shared<rt::SceneNode>();
    node2->mesh = mesh;
    node2->localTransform = rt::Transform::Translate(rt::Vector3f(3.0f, 0.0f, -2.0f));
    root->children.push_back(node2);

    // 3. Build CPU Scene for ground truth
    rt::Scene cpuScene;
    rt::FlattenSceneGraph(root, cpuScene);
    cpuScene.Build();

    // 4. Build GPU DeviceScene (GAS + IAS)
    rtx::DeviceScene devScene = rtx::DeviceScene::Build(root, optixContext, stream);
    REQUIRE(devScene.iasHandle != 0);
    // Verify GAS caching: exactly 1 GAS built for 2 instances of the same mesh!
    REQUIRE(devScene.meshes.size() == 1);

    // 5. Load OptiX Module & Pipeline
    std::filesystem::path shaderPath = FindShaderBinary("intersection_test.optixir");
    std::vector<char> optixirCode = ReadBinaryFile(shaderPath);

    OptixModuleCompileOptions moduleCompileOptions{};
    OptixPipelineCompileOptions pipelineCompileOptions{};
    pipelineCompileOptions.usesMotionBlur = 0;
    pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY;
    pipelineCompileOptions.numPayloadValues = 2; // pointer to payload
    pipelineCompileOptions.numAttributeValues = 2; // barycentrics
    pipelineCompileOptions.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
    pipelineCompileOptions.pipelineLaunchParamsVariableName = "params";

    OptixModule module = nullptr;
    char logBuffer[2048] = {0};
    size_t logSize = sizeof(logBuffer);

    OptixResult res = optixModuleCreate(
        optixContext,
        &moduleCompileOptions,
        &pipelineCompileOptions,
        optixirCode.data(),
        optixirCode.size(),
        logBuffer,
        &logSize,
        &module
    );
    REQUIRE(res == OPTIX_SUCCESS);

    // Create Program Groups: Raygen, Miss, ClosestHit
    OptixProgramGroupOptions pgOptions{};
    OptixProgramGroupDesc raygenPGDesc{};
    raygenPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    raygenPGDesc.raygen.module = module;
    raygenPGDesc.raygen.entryFunctionName = "__raygen__intersection_test";

    OptixProgramGroup raygenPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &raygenPGDesc, 1, &pgOptions, logBuffer, &logSize, &raygenPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroupDesc missPGDesc{};
    missPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    missPGDesc.miss.module = module;
    missPGDesc.miss.entryFunctionName = "__miss__intersection_test";

    OptixProgramGroup missPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &missPGDesc, 1, &pgOptions, logBuffer, &logSize, &missPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroupDesc hitPGDesc{};
    hitPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitPGDesc.hitgroup.moduleCH = module;
    hitPGDesc.hitgroup.entryFunctionNameCH = "__closesthit__intersection_test";

    OptixProgramGroup hitPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &hitPGDesc, 1, &pgOptions, logBuffer, &logSize, &hitPG);
    REQUIRE(res == OPTIX_SUCCESS);

    // Link Pipeline
    OptixProgramGroup programGroups[] = { raygenPG, missPG, hitPG };
    OptixPipelineLinkOptions linkOptions{};
    linkOptions.maxTraceDepth = 1;

    OptixPipeline pipeline = nullptr;
    logSize = sizeof(logBuffer);
    res = optixPipelineCreate(
        optixContext,
        &pipelineCompileOptions,
        &linkOptions,
        programGroups,
        3,
        logBuffer,
        &logSize,
        &pipeline
    );
    REQUIRE(res == OPTIX_SUCCESS);

    // 6. Build Shader Binding Table (SBT)
    RaygenRecord raygenRecord{};
    optixSbtRecordPackHeader(raygenPG, &raygenRecord);
    CUdeviceptr d_raygenRecord = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_raygenRecord), sizeof(RaygenRecord));
    cudaMemcpy(reinterpret_cast<void*>(d_raygenRecord), &raygenRecord, sizeof(RaygenRecord), cudaMemcpyHostToDevice);

    MissRecord missRecord{};
    optixSbtRecordPackHeader(missPG, &missRecord);
    CUdeviceptr d_missRecord = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_missRecord), sizeof(MissRecord));
    cudaMemcpy(reinterpret_cast<void*>(d_missRecord), &missRecord, sizeof(MissRecord), cudaMemcpyHostToDevice);

    std::vector<HitgroupRecord> hitRecords(devScene.meshes.size());
    for (size_t i = 0; i < devScene.meshes.size(); ++i) {
        optixSbtRecordPackHeader(hitPG, &hitRecords[i]);
        hitRecords[i].data.positions = reinterpret_cast<const rt::Point3f*>(devScene.meshes[i].d_positions);
        hitRecords[i].data.normals = reinterpret_cast<const rt::Normal3f*>(devScene.meshes[i].d_normals);
        hitRecords[i].data.uvs = reinterpret_cast<const rt::Point2f*>(devScene.meshes[i].d_uvs);
        hitRecords[i].data.indices = reinterpret_cast<const int*>(devScene.meshes[i].d_indices);
        hitRecords[i].data.triangleCount = devScene.meshes[i].triangleCount;
    }
    CUdeviceptr d_hitRecord = 0;
    size_t hitBytes = sizeof(HitgroupRecord) * hitRecords.size();
    cudaMalloc(reinterpret_cast<void**>(&d_hitRecord), hitBytes);
    cudaMemcpy(reinterpret_cast<void*>(d_hitRecord), hitRecords.data(), hitBytes, cudaMemcpyHostToDevice);

    OptixShaderBindingTable sbt{};
    sbt.raygenRecord = d_raygenRecord;
    sbt.missRecordBase = d_missRecord;
    sbt.missRecordStrideInBytes = sizeof(MissRecord);
    sbt.missRecordCount = 1;
    sbt.hitgroupRecordBase = d_hitRecord;
    sbt.hitgroupRecordStrideInBytes = sizeof(HitgroupRecord);
    sbt.hitgroupRecordCount = static_cast<unsigned int>(hitRecords.size());

    // 7. Define Test Rays
    std::vector<TestRay> testRays = {
        // Ray 0: Hit instance 1 at (0.25, 0.25, -2.0)
        { rt::Point3f(0.25f, 0.25f, 0.0f), rt::Vector3f(0.0f, 0.0f, -1.0f), 10.0f },
        // Ray 1: Hit instance 2 at (3.25, 0.25, -2.0)
        { rt::Point3f(3.25f, 0.25f, 0.0f), rt::Vector3f(0.0f, 0.0f, -1.0f), 10.0f },
        // Ray 2: Ray through gap between instances (miss)
        { rt::Point3f(1.50f, 0.25f, 0.0f), rt::Vector3f(0.0f, 0.0f, -1.0f), 10.0f },
        // Ray 3: Miss shooting away
        { rt::Point3f(0.25f, 0.25f, 0.0f), rt::Vector3f(0.0f, 0.0f, 1.0f), 10.0f },
        // Ray 4: Grazing angle on instance 1
        { rt::Point3f(0.1f, 0.1f, 1.0f), Normalize(rt::Vector3f(0.0f, 0.0f, -1.0f)), 10.0f }
    };

    const unsigned int numRays = static_cast<unsigned int>(testRays.size());

    CUdeviceptr d_testRays = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_testRays), numRays * sizeof(TestRay));
    cudaMemcpy(reinterpret_cast<void*>(d_testRays), testRays.data(), numRays * sizeof(TestRay), cudaMemcpyHostToDevice);

    CUdeviceptr d_results = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_results), numRays * sizeof(TestHitResult));

    IntersectionTestParams params{};
    params.iasHandle = devScene.iasHandle;
    params.testRays = reinterpret_cast<const TestRay*>(d_testRays);
    params.results = reinterpret_cast<TestHitResult*>(d_results);
    params.numTestRays = numRays;

    CUdeviceptr d_params = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(IntersectionTestParams));
    cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(IntersectionTestParams), cudaMemcpyHostToDevice);

    // 8. Launch OptiX Trace Kernel
    res = optixLaunch(pipeline, stream, d_params, sizeof(IntersectionTestParams), &sbt, numRays, 1, 1);
    REQUIRE(res == OPTIX_SUCCESS);
    cudaStreamSynchronize(stream);

    // 9. Read Back Results & Compare with CPU BVH
    std::vector<TestHitResult> gpuResults(numRays);
    cudaMemcpy(gpuResults.data(), reinterpret_cast<void*>(d_results), numRays * sizeof(TestHitResult), cudaMemcpyDeviceToHost);

    for (size_t i = 0; i < numRays; ++i) {
        rt::Ray cpuRay(testRays[i].origin, testRays[i].direction, testRays[i].tMax);
        rt::SurfaceInteraction cpuIsect;
        bool cpuHit = cpuScene.Intersect(cpuRay, &cpuIsect);

        INFO("Checking ray index " << i);
        REQUIRE(gpuResults[i].hit == (cpuHit ? 1 : 0));

        if (cpuHit) {
            REQUIRE_THAT(gpuResults[i].t, WithinAbs(cpuIsect.t, 1e-3f));
            REQUIRE_THAT(gpuResults[i].p.x, WithinAbs(cpuIsect.p.x, 1e-3f));
            REQUIRE_THAT(gpuResults[i].p.y, WithinAbs(cpuIsect.p.y, 1e-3f));
            REQUIRE_THAT(gpuResults[i].p.z, WithinAbs(cpuIsect.p.z, 1e-3f));
            REQUIRE_THAT(gpuResults[i].n.x, WithinAbs(cpuIsect.n.x, 1e-3f));
            REQUIRE_THAT(gpuResults[i].n.y, WithinAbs(cpuIsect.n.y, 1e-3f));
            REQUIRE_THAT(gpuResults[i].n.z, WithinAbs(cpuIsect.n.z, 1e-3f));
        }
    }

    // 10. Cleanup
    cudaFree(reinterpret_cast<void*>(d_params));
    cudaFree(reinterpret_cast<void*>(d_results));
    cudaFree(reinterpret_cast<void*>(d_testRays));
    cudaFree(reinterpret_cast<void*>(d_hitRecord));
    cudaFree(reinterpret_cast<void*>(d_missRecord));
    cudaFree(reinterpret_cast<void*>(d_raygenRecord));

    optixPipelineDestroy(pipeline);
    optixProgramGroupDestroy(hitPG);
    optixProgramGroupDestroy(missPG);
    optixProgramGroupDestroy(raygenPG);
    optixModuleDestroy(module);

    devScene.Destroy();
}
