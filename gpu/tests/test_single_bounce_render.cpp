#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "optix_context.h"
#include "device_scene.h"
#include "device_material.h"
#include "rt/cam/perspective_camera.h"
#include "rt/scene/scene_node.h"
#include "rt/scene/scene.h"
#include "rt/shapes/triangle.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/emissive.h"
#include "rt/core/rng.h"

#include <optix_stubs.h>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>

using Catch::Matchers::WithinAbs;

namespace {

    struct SingleBounceParams {
        OptixTraversableHandle iasHandle;
        rt::PerspectiveCamera camera;
        unsigned int width;
        unsigned int height;
        unsigned int frameSeed;
        rt::Vector3f* outputBuffer;
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

    uint32_t HashSeed(uint32_t x, uint32_t y, uint32_t seed) {
        uint32_t h = seed ^ (x * 73856093u) ^ (y * 19349663u);
        h = (h ^ 61u) ^ (h >> 16u);
        h += (h << 3u);
        h ^= (h >> 4u);
        h *= 0x27d4eb2du;
        h ^= (h >> 15u);
        return h;
    }

    rt::Vector3f SkyGradient(const rt::Vector3f& dir) {
        rt::Vector3f unitDir = Normalize(dir);
        float t = 0.5f * (unitDir.y + 1.0f);
        return 0.15f * ((1.0f - t) * rt::Vector3f(0.8f, 0.8f, 0.9f) + t * rt::Vector3f(0.4f, 0.5f, 0.7f));
    }

}

TEST_CASE("Single-bounce direct lighting render parity vs CPU", "[gpu][materials][render]") {
    constexpr int kWidth = 32;
    constexpr int kHeight = 32;
    constexpr unsigned int kSeed = 1337;

    rt::PerspectiveCamera camera(
        rt::Point3f(0.0f, 0.0f, 3.0f),
        rt::Point3f(0.0f, 0.0f, 0.0f),
        rt::Vector3f(0.0f, 1.0f, 0.0f),
        45.0f,
        kWidth, kHeight
    );

    auto mesh = std::make_shared<rt::TriangleMesh>();
    mesh->positions = {
        rt::Point3f(-2.0f, -1.0f,  2.0f),
        rt::Point3f( 2.0f, -1.0f,  2.0f),
        rt::Point3f( 2.0f, -1.0f, -2.0f),
        rt::Point3f(-2.0f, -1.0f, -2.0f)
    };
    mesh->normals = {
        rt::Normal3f(0.0f, 1.0f, 0.0f),
        rt::Normal3f(0.0f, 1.0f, 0.0f),
        rt::Normal3f(0.0f, 1.0f, 0.0f),
        rt::Normal3f(0.0f, 1.0f, 0.0f)
    };
    mesh->indices = { 0, 1, 2, 0, 2, 3 };

    auto floorMat = std::make_shared<rt::Lambertian>(rt::Vector3f(0.8f, 0.8f, 0.8f));
    auto lightMat = std::make_shared<rt::Emissive>(rt::Vector3f(5.0f, 5.0f, 5.0f));

    auto root = std::make_shared<rt::SceneNode>();
    auto floorNode = std::make_shared<rt::SceneNode>();
    floorNode->mesh = mesh;
    floorNode->bsdf = floorMat;
    root->children.push_back(floorNode);

    auto lightMesh = std::make_shared<rt::TriangleMesh>();
    lightMesh->positions = {
        rt::Point3f(-0.5f, 1.5f, -0.5f),
        rt::Point3f( 0.5f, 1.5f, -0.5f),
        rt::Point3f( 0.5f, 1.5f,  0.5f),
        rt::Point3f(-0.5f, 1.5f,  0.5f)
    };
    lightMesh->normals = {
        rt::Normal3f(0.0f, -1.0f, 0.0f),
        rt::Normal3f(0.0f, -1.0f, 0.0f),
        rt::Normal3f(0.0f, -1.0f, 0.0f),
        rt::Normal3f(0.0f, -1.0f, 0.0f)
    };
    lightMesh->indices = { 0, 1, 2, 0, 2, 3 };

    auto lightNode = std::make_shared<rt::SceneNode>();
    lightNode->mesh = lightMesh;
    lightNode->bsdf = lightMat;
    root->children.push_back(lightNode);

    rt::Scene cpuScene;
    rt::FlattenSceneGraph(root, cpuScene);
    cpuScene.Build();

    auto ctx = rtx::OptixContext::Create();
    REQUIRE(ctx != nullptr);
    OptixDeviceContext optixContext = ctx->GetOptixDeviceContext();
    CUstream stream = ctx->GetCudaStream();

    rtx::DeviceScene devScene = rtx::DeviceScene::Build(root, optixContext, stream);
    REQUIRE(devScene.iasHandle != 0);

    std::filesystem::path shaderPath = FindShaderBinary("single_bounce.optixir");
    std::vector<char> optixirCode = ReadBinaryFile(shaderPath);

    OptixModuleCompileOptions moduleCompileOptions{};
    OptixPipelineCompileOptions pipelineCompileOptions{};
    pipelineCompileOptions.usesMotionBlur = 0;
    pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY;
    pipelineCompileOptions.numPayloadValues = 2;
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
    raygenPGDesc.raygen.entryFunctionName = "__raygen__single_bounce";

    OptixProgramGroup raygenPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &raygenPGDesc, 1, &pgOptions, logBuffer, &logSize, &raygenPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroupDesc missPGDesc{};
    missPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    missPGDesc.miss.module = module;
    missPGDesc.miss.entryFunctionName = "__miss__single_bounce";

    OptixProgramGroup missPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &missPGDesc, 1, &pgOptions, logBuffer, &logSize, &missPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroupDesc hitPGDesc{};
    hitPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitPGDesc.hitgroup.moduleCH = module;
    hitPGDesc.hitgroup.entryFunctionNameCH = "__closesthit__single_bounce";

    OptixProgramGroup hitPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &hitPGDesc, 1, &pgOptions, logBuffer, &logSize, &hitPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroup programGroups[] = { raygenPG, missPG, hitPG };
    OptixPipelineLinkOptions linkOptions{};
    linkOptions.maxTraceDepth = 2;

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

    std::vector<HitgroupRecord> hitRecords(devScene.instances.size());
    for (size_t i = 0; i < devScene.instances.size(); ++i) {
        optixSbtRecordPackHeader(hitPG, &hitRecords[i]);
        size_t meshIdx = devScene.instances[i].meshIndex;
        hitRecords[i].data.positions = reinterpret_cast<const rt::Point3f*>(devScene.meshes[meshIdx].d_positions);
        hitRecords[i].data.normals = reinterpret_cast<const rt::Normal3f*>(devScene.meshes[meshIdx].d_normals);
        hitRecords[i].data.uvs = reinterpret_cast<const rt::Point2f*>(devScene.meshes[meshIdx].d_uvs);
        hitRecords[i].data.indices = reinterpret_cast<const int*>(devScene.meshes[meshIdx].d_indices);
        hitRecords[i].data.triangleCount = devScene.meshes[meshIdx].triangleCount;
        hitRecords[i].data.material = devScene.instances[i].material;
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

    SingleBounceParams params{};
    params.iasHandle = devScene.iasHandle;
    params.camera = camera;
    params.width = kWidth;
    params.height = kHeight;
    params.frameSeed = kSeed;
    params.outputBuffer = reinterpret_cast<rt::Vector3f*>(d_outputBuffer);

    CUdeviceptr d_params = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(SingleBounceParams));
    cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(SingleBounceParams), cudaMemcpyHostToDevice);

    res = optixLaunch(pipeline, stream, d_params, sizeof(SingleBounceParams), &sbt, kWidth, kHeight, 1);
    REQUIRE(res == OPTIX_SUCCESS);
    cudaStreamSynchronize(stream);

    std::vector<rt::Vector3f> gpuFramebuffer(kWidth * kHeight);
    cudaMemcpy(gpuFramebuffer.data(), reinterpret_cast<void*>(d_outputBuffer), bufferSize, cudaMemcpyDeviceToHost);

    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            int pixelIdx = y * kWidth + x;
            rt::CameraSample sample{ rt::Point2f(x + 0.5f, y + 0.5f) };
            rt::Ray cpuRay = camera.GenerateRay(sample);
            rt::RNG rng(HashSeed(x, y, kSeed));

            rt::Vector3f cpuL(0.0f, 0.0f, 0.0f);
            rt::SurfaceInteraction hit0;
            if (!cpuScene.Intersect(cpuRay, &hit0)) {
                cpuL = SkyGradient(cpuRay.d);
            } else {
                rt::Vector3f wo = Normalize(-cpuRay.d);
                const rt::BSDF* bsdf0 = hit0.shape ? hit0.shape->GetBSDF() : nullptr;
                if (bsdf0) {
                    cpuL = cpuL + bsdf0->Le(wo, static_cast<rt::Vector3f>(hit0.n));
                    rt::Vector3f wi(0.0f, 0.0f, 0.0f);
                    float pdf = 0.0f;
                    rt::Vector3f f = bsdf0->Sample_f(wo, static_cast<rt::Vector3f>(hit0.n), rng.Uniform2D(), &wi, &pdf, hit0.uv);
                    if (pdf > 0.0f && (f.x > 0.0f || f.y > 0.0f || f.z > 0.0f)) {
                        float cosTheta = AbsDot(wi, hit0.n);
                        rt::Vector3f throughput = f * (cosTheta / pdf);
                        rt::Vector3f offsetN = (Dot(wi, hit0.n) > 0.0f) ? static_cast<rt::Vector3f>(hit0.n) : -static_cast<rt::Vector3f>(hit0.n);
                        rt::Ray bounceRay(hit0.p + 1e-3f * offsetN, wi);
                        rt::SurfaceInteraction hit1;
                        if (cpuScene.Intersect(bounceRay, &hit1)) {
                            const rt::BSDF* bsdf1 = hit1.shape ? hit1.shape->GetBSDF() : nullptr;
                            if (bsdf1) {
                                cpuL = cpuL + throughput * bsdf1->Le(Normalize(-wi), static_cast<rt::Vector3f>(hit1.n));
                            }
                        } else {
                            cpuL = cpuL + throughput * SkyGradient(wi);
                        }
                    }
                }
            }

            const rt::Vector3f& gpuL = gpuFramebuffer[pixelIdx];
            INFO("Pixel (" << x << ", " << y << ")");
            REQUIRE_THAT(gpuL.x, WithinAbs(cpuL.x, 1e-2f));
            REQUIRE_THAT(gpuL.y, WithinAbs(cpuL.y, 1e-2f));
            REQUIRE_THAT(gpuL.z, WithinAbs(cpuL.z, 1e-2f));
        }
    }

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
