#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "optix_context.h"
#include "device_scene.h"
#include "launch_params.h"
#include "rt/cam/perspective_camera.h"
#include "rt/scene/scene_node.h"
#include "rt/scene/scene.h"
#include "rt/shapes/triangle.h"

#include <optix_stubs.h>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>

using Catch::Matchers::WithinAbs;

namespace {

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

}

TEST_CASE("PerspectiveCamera ray generation sanity", "[camera][gpu]") {
    rt::PerspectiveCamera camera(
        rt::Point3f(0.0f, 1.2f, 4.0f),
        rt::Point3f(0.0f, 0.3f, -1.0f),
        rt::Vector3f(0.0f, 1.0f, 0.0f),
        35.0f,
        100, 100
    );

    rt::CameraSample centerSample{ rt::Point2f(50.0f, 50.0f) };
    rt::Ray centerRay = camera.GenerateRay(centerSample);

    REQUIRE_THAT(centerRay.o.x, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(centerRay.o.y, WithinAbs(1.2f, 1e-5f));
    REQUIRE_THAT(centerRay.o.z, WithinAbs(4.0f, 1e-5f));
    REQUIRE_THAT(Length(centerRay.d), WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Normals-only full resolution render parity vs CPU", "[gpu][normals][render]") {
    constexpr int kWidth = 64;
    constexpr int kHeight = 64;

    rt::PerspectiveCamera camera(
        rt::Point3f(0.0f, 0.0f, 3.0f),
        rt::Point3f(0.0f, 0.0f, 0.0f),
        rt::Vector3f(0.0f, 1.0f, 0.0f),
        45.0f,
        kWidth, kHeight
    );

    auto ctx = rtx::OptixContext::Create();
    REQUIRE(ctx != nullptr);
    OptixDeviceContext optixContext = ctx->GetOptixDeviceContext();
    CUstream stream = ctx->GetCudaStream();

    auto mesh = std::make_shared<rt::TriangleMesh>();
    mesh->positions = {
        rt::Point3f(-1.0f, -1.0f, 0.0f),
        rt::Point3f( 1.0f, -1.0f, 0.0f),
        rt::Point3f( 1.0f,  1.0f, 0.0f),
        rt::Point3f(-1.0f,  1.0f, 0.0f)
    };
    mesh->normals = {
        rt::Normal3f(0.0f, 0.0f, 1.0f),
        rt::Normal3f(0.0f, 0.0f, 1.0f),
        rt::Normal3f(0.0f, 0.0f, 1.0f),
        rt::Normal3f(0.0f, 0.0f, 1.0f)
    };
    mesh->indices = { 0, 1, 2, 0, 2, 3 };

    auto root = std::make_shared<rt::SceneNode>();
    auto node = std::make_shared<rt::SceneNode>();
    node->mesh = mesh;
    node->localTransform = rt::Transform::Translate(rt::Vector3f(0.0f, 0.0f, -1.0f)) * rt::Transform::RotateY(30.0f);
    root->children.push_back(node);

    rt::Scene cpuScene;
    rt::FlattenSceneGraph(root, cpuScene);
    cpuScene.Build();

    rtx::DeviceScene devScene = rtx::DeviceScene::Build(root, optixContext, stream);
    REQUIRE(devScene.iasHandle != 0);

    std::filesystem::path shaderPath = FindShaderBinary("normals_render.optixir");
    std::vector<char> optixirCode = ReadBinaryFile(shaderPath);

    OptixModuleCompileOptions moduleCompileOptions{};
    OptixPipelineCompileOptions pipelineCompileOptions{};
    pipelineCompileOptions.usesMotionBlur = 0;
    pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY;
    pipelineCompileOptions.numPayloadValues = 3;
    pipelineCompileOptions.numAttributeValues = 2;
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

    OptixProgramGroupOptions pgOptions{};
    OptixProgramGroupDesc raygenPGDesc{};
    raygenPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    raygenPGDesc.raygen.module = module;
    raygenPGDesc.raygen.entryFunctionName = "__raygen__normals";

    OptixProgramGroup raygenPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &raygenPGDesc, 1, &pgOptions, logBuffer, &logSize, &raygenPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroupDesc missPGDesc{};
    missPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    missPGDesc.miss.module = module;
    missPGDesc.miss.entryFunctionName = "__miss__normals";

    OptixProgramGroup missPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &missPGDesc, 1, &pgOptions, logBuffer, &logSize, &missPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroupDesc hitPGDesc{};
    hitPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitPGDesc.hitgroup.moduleCH = module;
    hitPGDesc.hitgroup.entryFunctionNameCH = "__closesthit__normals";

    OptixProgramGroup hitPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &hitPGDesc, 1, &pgOptions, logBuffer, &logSize, &hitPG);
    REQUIRE(res == OPTIX_SUCCESS);

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

    size_t bufferSize = static_cast<size_t>(kWidth) * kHeight * sizeof(rt::Vector3f);
    CUdeviceptr d_outputBuffer = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_outputBuffer), bufferSize);

    rtx::NormalsLaunchParams params{};
    params.iasHandle = devScene.iasHandle;
    params.camera = camera;
    params.width = kWidth;
    params.height = kHeight;
    params.outputBuffer = reinterpret_cast<rt::Vector3f*>(d_outputBuffer);

    CUdeviceptr d_params = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(rtx::NormalsLaunchParams));
    cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(rtx::NormalsLaunchParams), cudaMemcpyHostToDevice);

    res = optixLaunch(pipeline, stream, d_params, sizeof(rtx::NormalsLaunchParams), &sbt, kWidth, kHeight, 1);
    REQUIRE(res == OPTIX_SUCCESS);
    cudaStreamSynchronize(stream);

    std::vector<rt::Vector3f> gpuFramebuffer(kWidth * kHeight);
    cudaMemcpy(gpuFramebuffer.data(), reinterpret_cast<void*>(d_outputBuffer), bufferSize, cudaMemcpyDeviceToHost);

    int hitCount = 0;
    int missCount = 0;

    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            int pixelIdx = y * kWidth + x;
            rt::CameraSample sample{ rt::Point2f(x + 0.5f, y + 0.5f) };
            rt::Ray cpuRay = camera.GenerateRay(sample);

            rt::SurfaceInteraction isect;
            rt::Vector3f cpuColor;
            if (!cpuScene.Intersect(cpuRay, &isect)) {
                missCount++;
                rt::Vector3f unitDir = Normalize(cpuRay.d);
                float t = 0.5f * (unitDir.y + 1.0f);
                cpuColor = 0.15f * ((1.0f - t) * rt::Vector3f(0.8f, 0.8f, 0.9f) + t * rt::Vector3f(0.4f, 0.5f, 0.7f));
            } else {
                hitCount++;
                rt::Vector3f n = rt::Vector3f(isect.n);
                cpuColor = 0.5f * (n + rt::Vector3f(1.0f, 1.0f, 1.0f));
            }

            const rt::Vector3f& gpuColor = gpuFramebuffer[pixelIdx];
            INFO("Pixel (" << x << ", " << y << ")");
            REQUIRE_THAT(gpuColor.x, WithinAbs(cpuColor.x, 2e-3f));
            REQUIRE_THAT(gpuColor.y, WithinAbs(cpuColor.y, 2e-3f));
            REQUIRE_THAT(gpuColor.z, WithinAbs(cpuColor.z, 2e-3f));
        }
    }

    REQUIRE(hitCount > 0);
    REQUIRE(missCount > 0);

    cudaFree(reinterpret_cast<void*>(d_params));
    cudaFree(reinterpret_cast<void*>(d_outputBuffer));
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
