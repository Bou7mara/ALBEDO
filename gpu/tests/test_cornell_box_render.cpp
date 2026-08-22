#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "optix_context.h"
#include "device_scene.h"
#include "device_material.h"
#include "device_light.h"
#include "rt/cam/perspective_camera.h"
#include "rt/scene/scene_node.h"
#include "rt/scene/scene.h"
#include "rt/scene/showcase.h"
#include "rt/shapes/triangle.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/emissive.h"
#include "rt/core/rng.h"
#include "rt/integrator_constants.h"

#include <optix_stubs.h>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>
#include <cmath>

using Catch::Matchers::WithinAbs;

namespace {

    struct PathTracerParams {
        OptixTraversableHandle iasHandle;
        rt::PerspectiveCamera camera;
        rtx::DeviceLightList lights;
        unsigned int width;
        unsigned int height;
        unsigned int samplesPerPixel;
        unsigned int maxDepth;
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

    rt::Vector3f ALBEDO_Ref(rt::Ray r, const rt::Scene& scene, rt::RNG& rng, int maxDepth) {
        rt::Vector3f L(0.0f, 0.0f, 0.0f);
        rt::Vector3f throughput(1.0f, 1.0f, 1.0f);
        bool specularBounce = true;
        float prevBsdfPdf = 0.0f;

        for (int depth = 0; depth < maxDepth; ++depth) {
            rt::SurfaceInteraction isect;
            if (!scene.Intersect(r, &isect)) {
                rt::Vector3f unitDir = Normalize(r.d);
                float t = 0.5f * (unitDir.y + 1.0f);
                rt::Vector3f bg = 0.15f * ((1.0f - t) * rt::Vector3f(0.8f, 0.8f, 0.9f) + t * rt::Vector3f(0.4f, 0.5f, 0.7f));
                L += throughput * bg;
                break;
            }

            const rt::BSDF* bsdf = isect.shape->GetBSDF();
            if (!bsdf) {
                rt::Vector3f n = rt::Vector3f(isect.n);
                L += throughput * rt::Vector3f(0.5f * (n.x + 1.0f), 0.5f * (n.y + 1.0f), 0.5f * (n.z + 1.0f));
                break;
            }

            rt::Vector3f emitted = bsdf->Le(isect.wo, rt::Vector3f(isect.n));
            if (emitted.x > 0.0f || emitted.y > 0.0f || emitted.z > 0.0f) {
                if (specularBounce || scene.Lights().empty()) {
                    L += throughput * emitted;
                } else {
                    float pmf = scene.LightPmf(isect.shape);
                    float lightPdf = isect.shape->Pdf(r.o, r.d) * pmf;
                    float weight = rt::PowerHeuristic(1, prevBsdfPdf, 1, lightPdf);
                    L += throughput * emitted * weight;
                }
            }

            if (!scene.Lights().empty()) {
                int lightIdx = -1;
                float pmf = 0.0f;
                const rt::Light* light = scene.SampleLight(rng.Uniform1D(), &lightIdx, &pmf);

                if (light && pmf > 0.0f) {
                    rt::Light::LiSample lightSample = light->Sample_Li(isect.p, rng.Uniform2D());
                    if (lightSample.pdf > 0.0f) {
                        constexpr float kEpsilon = 1e-4f;
                        rt::Vector3f offsetNormal = (Dot(lightSample.wi, isect.n) > 0.0f) ? rt::Vector3f(isect.n) : -rt::Vector3f(isect.n);
                        rt::Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;

                        rt::Ray shadowRay(offsetOrigin, lightSample.wi, lightSample.dist - 2.0f * kEpsilon);
                        if (!scene.IntersectP(shadowRay)) {
                            rt::Vector3f f = bsdf->f(isect.wo, lightSample.wi, rt::Vector3f(isect.n));
                            if (f.x > 0.0f || f.y > 0.0f || f.z > 0.0f) {
                                float bsdfPdf = bsdf->Pdf(isect.wo, lightSample.wi, rt::Vector3f(isect.n));
                                if (bsdfPdf > 0.0f) {
                                    float lPdf = lightSample.pdf * pmf;
                                    float weight = rt::PowerHeuristic(1, lPdf, 1, bsdfPdf);
                                    float cosTheta = AbsDot(lightSample.wi, isect.n);
                                    L += throughput * f * lightSample.Li * cosTheta * weight / lPdf;
                                }
                            }
                        }
                    }
                }
            }

            rt::Vector3f wi;
            float pdf;
            rt::Vector3f f = bsdf->Sample_f(isect.wo, rt::Vector3f(isect.n), rng.Uniform2D(), &wi, &pdf);

            if (pdf <= 0.0f || (f.x == 0.0f && f.y == 0.0f && f.z == 0.0f)) break;

            float cosTheta = AbsDot(wi, isect.n);
            throughput = throughput * f * cosTheta / pdf;

            prevBsdfPdf = bsdf->Pdf(isect.wo, wi, rt::Vector3f(isect.n));
            specularBounce = (prevBsdfPdf == 0.0f);

            if (depth >= rt::kRRStartDepth) {
                const float q = std::clamp(rt::MaxChannel(throughput), rt::kRRProbabilityMinimumThreshold, rt::kRRProbabilityMaximumThreshold);
                if (rng.Uniform1D() > q) break;
                throughput = throughput / q;
            }

            constexpr float kEpsilon = 1e-4f;
            rt::Vector3f offsetNormal = (Dot(wi, isect.n) > 0.0f) ? rt::Vector3f(isect.n) : -rt::Vector3f(isect.n);
            rt::Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;

            r = rt::Ray(offsetOrigin, wi);
        }
        return L;
    }

} // namespace

TEST_CASE("Cornell Box full path tracer RMSE convergence against CPU", "[gpu][pathtracer][cornell]") {
    constexpr int kWidth = 32;
    constexpr int kHeight = 32;
    constexpr int kSpp = 64;
    constexpr int kMaxDepth = 5;

    // 1. Build Scene
    rt::ShowcaseSetup setup = rt::CreateCornellBoxShowcaseScene(kWidth, kHeight, kSpp);

    auto ctx = rtx::OptixContext::Create();
    REQUIRE(ctx != nullptr);
    OptixDeviceContext optixContext = ctx->GetOptixDeviceContext();
    CUstream stream = ctx->GetCudaStream();

    // 2. Build GPU Scene
    // Convert CPU Scene shapes into SceneNode graph for GPU translation
    auto root = std::make_shared<rt::SceneNode>();
    for (const auto& shape : setup.scene.Shapes()) {
        auto node = std::make_shared<rt::SceneNode>();
        node->mesh = std::dynamic_pointer_cast<rt::TriangleMesh>(shape);
        node->bsdf = std::shared_ptr<rt::BSDF>(const_cast<rt::BSDF*>(shape->GetBSDF()), [](rt::BSDF*){});
        if (node->mesh) {
            root->children.push_back(node);
        }
    }

    rtx::DeviceScene devScene = rtx::DeviceScene::Build(root, optixContext, stream);
    REQUIRE(devScene.iasHandle != 0);
    REQUIRE(devScene.lightList.count > 0);

    // 3. Load OptiX Module & Pipeline
    std::filesystem::path shaderPath = FindShaderBinary("path_tracer.optixir");
    std::vector<char> optixirCode = ReadBinaryFile(shaderPath);

    OptixModuleCompileOptions moduleCompileOptions{};
    OptixPipelineCompileOptions pipelineCompileOptions{};
    pipelineCompileOptions.usesMotionBlur = 0;
    pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY;
    pipelineCompileOptions.numPayloadValues = 2; // pointer to PathHitPayload
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
    raygenPGDesc.raygen.entryFunctionName = "__raygen__albedo";

    OptixProgramGroup raygenPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &raygenPGDesc, 1, &pgOptions, logBuffer, &logSize, &raygenPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroupDesc missPGDesc{};
    missPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    missPGDesc.miss.module = module;
    missPGDesc.miss.entryFunctionName = "__miss__albedo";

    OptixProgramGroup missPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &missPGDesc, 1, &pgOptions, logBuffer, &logSize, &missPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroupDesc hitPGDesc{};
    hitPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitPGDesc.hitgroup.moduleCH = module;
    hitPGDesc.hitgroup.entryFunctionNameCH = "__closesthit__albedo";

    OptixProgramGroup hitPG = nullptr;
    logSize = sizeof(logBuffer);
    res = optixProgramGroupCreate(optixContext, &hitPGDesc, 1, &pgOptions, logBuffer, &logSize, &hitPG);
    REQUIRE(res == OPTIX_SUCCESS);

    OptixProgramGroup programGroups[] = { raygenPG, missPG, hitPG };
    OptixPipelineLinkOptions linkOptions{};
    linkOptions.maxTraceDepth = kMaxDepth;

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

    // 4. Setup SBT
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

    // 5. Launch GPU Path Tracer
    size_t bufferSize = static_cast<size_t>(kWidth) * kHeight * sizeof(rt::Vector3f);
    CUdeviceptr d_outputBuffer = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_outputBuffer), bufferSize);

    PathTracerParams params{};
    params.iasHandle = devScene.iasHandle;
    params.camera = setup.camera;
    params.lights = devScene.lightList;
    params.width = kWidth;
    params.height = kHeight;
    params.samplesPerPixel = kSpp;
    params.maxDepth = kMaxDepth;
    params.frameSeed = 42;
    params.outputBuffer = reinterpret_cast<rt::Vector3f*>(d_outputBuffer);

    CUdeviceptr d_params = 0;
    cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(PathTracerParams));
    cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(PathTracerParams), cudaMemcpyHostToDevice);

    res = optixLaunch(pipeline, stream, d_params, sizeof(PathTracerParams), &sbt, kWidth, kHeight, 1);
    REQUIRE(res == OPTIX_SUCCESS);
    cudaStreamSynchronize(stream);

    std::vector<rt::Vector3f> gpuFramebuffer(kWidth * kHeight);
    cudaMemcpy(gpuFramebuffer.data(), reinterpret_cast<void*>(d_outputBuffer), bufferSize, cudaMemcpyDeviceToHost);

    // 6. CPU Reference Rendering
    std::vector<rt::Vector3f> cpuFramebuffer(kWidth * kHeight);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            rt::RNG rng(1337 + y * kWidth + x);
            rt::Vector3f colorSum(0.0f, 0.0f, 0.0f);
            for (int s = 0; s < kSpp; ++s) {
                rt::Point2f jitter = rng.Uniform2D();
                rt::CameraSample sample{ rt::Point2f(x + jitter.x, y + jitter.y) };
                rt::Ray ray = setup.camera.GenerateRay(sample);
                colorSum += ALBEDO_Ref(ray, setup.scene, rng, kMaxDepth);
            }
            cpuFramebuffer[y * kWidth + x] = colorSum / static_cast<float>(kSpp);
        }
    }

    // 7. Compute Quantitative Parity (RMSE)
    double sumSquaredError = 0.0;
    for (size_t i = 0; i < gpuFramebuffer.size(); ++i) {
        rt::Vector3f diff = gpuFramebuffer[i] - cpuFramebuffer[i];
        sumSquaredError += (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    }
    double rmse = std::sqrt(sumSquaredError / (3.0 * gpuFramebuffer.size()));

    INFO("Cornell Box RMSE between GPU and CPU: " << rmse);
    // RMSE is within expected stochastic Monte Carlo variance (< 0.15 for 64 spp)
    REQUIRE(rmse < 0.15);

    // 8. Cleanup
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
