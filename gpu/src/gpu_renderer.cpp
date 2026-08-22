#include "gpu_renderer.h"
#include "optix_context.h"
#include "device_scene.h"
#include "device_material.h"
#include "device_light.h"
#include "denoiser.h"
#include "rt/cam/perspective_camera.h"
#include "rt/scene/scene_node.h"
#include "rt/scene/scene.h"
#include "rt/shapes/triangle.h"
#include "rt/core/progress.h"

#include <optix_stubs.h>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>
#include <stdexcept>

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
        rt::Vector3f* albedoBuffer;
        rt::Vector3f* normalBuffer;
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

    __global__ void AccumulateKernel(rt::Vector3f* accumBuffer, const rt::Vector3f* scratchBuffer, int count) {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= count) return;
        accumBuffer[idx] += scratchBuffer[idx];
    }

    __global__ void NormalizeKernel(rt::Vector3f* accumBuffer, float invSpp, int count) {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= count) return;
        accumBuffer[idx] = accumBuffer[idx] * invSpp;
    }

} // namespace

namespace rtx {

    void RenderGpu(const rt::ShowcaseSetup& setup, std::vector<rt::Vector3f>& framebuffer, bool denoise) {
        const int width = setup.imageWidth;
        const int height = setup.imageHeight;
        const int spp = setup.samplesPerPixel;
        const int maxDepth = setup.maxDepth;
        const int totalPixels = width * height;

        framebuffer.resize(totalPixels);

        // 1. Context
        auto ctx = OptixContext::Create();
        if (!ctx) throw std::runtime_error("Failed to create OptixContext");
        OptixDeviceContext optixContext = ctx->GetOptixDeviceContext();
        CUstream stream = ctx->GetCudaStream();

        // 2. Build GPU Scene from setup.scene
        auto root = std::make_shared<rt::SceneNode>();
        for (const auto& shape : setup.scene.Shapes()) {
            auto node = std::make_shared<rt::SceneNode>();
            node->mesh = std::dynamic_pointer_cast<rt::TriangleMesh>(shape);
            node->bsdf = std::shared_ptr<rt::BSDF>(const_cast<rt::BSDF*>(shape->GetBSDF()), [](rt::BSDF*){});
            if (node->mesh) {
                root->children.push_back(node);
            }
        }

        DeviceScene devScene = DeviceScene::Build(root, optixContext, stream);
        if (!devScene.iasHandle) throw std::runtime_error("Failed to build DeviceScene IAS");

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
        if (res != OPTIX_SUCCESS) throw std::runtime_error("Failed to create OptiX module");

        OptixProgramGroupOptions pgOptions{};
        OptixProgramGroupDesc raygenPGDesc{};
        raygenPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
        raygenPGDesc.raygen.module = module;
        raygenPGDesc.raygen.entryFunctionName = "__raygen__albedo";

        OptixProgramGroup raygenPG = nullptr;
        logSize = sizeof(logBuffer);
        res = optixProgramGroupCreate(optixContext, &raygenPGDesc, 1, &pgOptions, logBuffer, &logSize, &raygenPG);
        if (res != OPTIX_SUCCESS) throw std::runtime_error("Failed to create raygen program group");

        OptixProgramGroupDesc missPGDesc{};
        missPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
        missPGDesc.miss.module = module;
        missPGDesc.miss.entryFunctionName = "__miss__albedo";

        OptixProgramGroup missPG = nullptr;
        logSize = sizeof(logBuffer);
        res = optixProgramGroupCreate(optixContext, &missPGDesc, 1, &pgOptions, logBuffer, &logSize, &missPG);
        if (res != OPTIX_SUCCESS) throw std::runtime_error("Failed to create miss program group");

        OptixProgramGroupDesc hitPGDesc{};
        hitPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        hitPGDesc.hitgroup.moduleCH = module;
        hitPGDesc.hitgroup.entryFunctionNameCH = "__closesthit__albedo";

        OptixProgramGroup hitPG = nullptr;
        logSize = sizeof(logBuffer);
        res = optixProgramGroupCreate(optixContext, &hitPGDesc, 1, &pgOptions, logBuffer, &logSize, &hitPG);
        if (res != OPTIX_SUCCESS) throw std::runtime_error("Failed to create hitgroup program group");

        OptixProgramGroup programGroups[] = { raygenPG, missPG, hitPG };
        OptixPipelineLinkOptions linkOptions{};
        linkOptions.maxTraceDepth = maxDepth;

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
        if (res != OPTIX_SUCCESS) throw std::runtime_error("Failed to create OptiX pipeline");

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

        // 5. Device Buffers
        size_t bufferBytes = totalPixels * sizeof(rt::Vector3f);
        CUdeviceptr d_accumulationBuffer = 0;
        CUdeviceptr d_scratchBuffer = 0;
        CUdeviceptr d_albedoBuffer = 0;
        CUdeviceptr d_normalBuffer = 0;
        CUdeviceptr d_denoisedBuffer = 0;

        cudaMalloc(reinterpret_cast<void**>(&d_accumulationBuffer), bufferBytes);
        cudaMemset(reinterpret_cast<void*>(d_accumulationBuffer), 0, bufferBytes);
        cudaMalloc(reinterpret_cast<void**>(&d_scratchBuffer), bufferBytes);

        if (denoise) {
            cudaMalloc(reinterpret_cast<void**>(&d_albedoBuffer), bufferBytes);
            cudaMalloc(reinterpret_cast<void**>(&d_normalBuffer), bufferBytes);
            cudaMalloc(reinterpret_cast<void**>(&d_denoisedBuffer), bufferBytes);
        }

        CUdeviceptr d_params = 0;
        cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(PathTracerParams));

        // 6. Launch Accumulation Loop
        rt::ProgressReporter progress(spp);
        constexpr int blockSize = 256;
        int numBlocks = (totalPixels + blockSize - 1) / blockSize;

        for (int s = 0; s < spp; ++s) {
            PathTracerParams params{};
            params.iasHandle = devScene.iasHandle;
            params.camera = setup.camera;
            params.lights = devScene.lightList;
            params.width = width;
            params.height = height;
            params.samplesPerPixel = 1;
            params.maxDepth = maxDepth;
            params.frameSeed = static_cast<unsigned int>(s);
            params.outputBuffer = reinterpret_cast<rt::Vector3f*>(d_scratchBuffer);
            params.albedoBuffer = reinterpret_cast<rt::Vector3f*>(d_albedoBuffer);
            params.normalBuffer = reinterpret_cast<rt::Vector3f*>(d_normalBuffer);

            cudaMemcpyAsync(reinterpret_cast<void*>(d_params), &params, sizeof(PathTracerParams), cudaMemcpyHostToDevice, stream);

            optixLaunch(pipeline, stream, d_params, sizeof(PathTracerParams), &sbt, width, height, 1);
            AccumulateKernel<<<numBlocks, blockSize, 0, stream>>>(
                reinterpret_cast<rt::Vector3f*>(d_accumulationBuffer),
                reinterpret_cast<const rt::Vector3f*>(d_scratchBuffer),
                totalPixels
            );

            progress.Advance();
        }
        progress.Finish();

        // 7. Normalize by SPP
        NormalizeKernel<<<numBlocks, blockSize, 0, stream>>>(
            reinterpret_cast<rt::Vector3f*>(d_accumulationBuffer),
            1.0f / static_cast<float>(spp),
            totalPixels
        );
        cudaStreamSynchronize(stream);

        // 8. Denoise if requested
        if (denoise) {
            std::cout << "Applying OptiX AI Denoiser with Albedo and Normal guide layers...\n";
            auto denoiserWrapper = OptixDenoiserWrapper::Create(optixContext, width, height, true);
            denoiserWrapper.Denoise(
                d_accumulationBuffer,
                d_denoisedBuffer,
                d_albedoBuffer,
                d_normalBuffer,
                width,
                height,
                stream
            );
            cudaStreamSynchronize(stream);
            denoiserWrapper.Destroy();

            cudaMemcpy(framebuffer.data(), reinterpret_cast<void*>(d_denoisedBuffer), bufferBytes, cudaMemcpyDeviceToHost);
        } else {
            cudaMemcpy(framebuffer.data(), reinterpret_cast<void*>(d_accumulationBuffer), bufferBytes, cudaMemcpyDeviceToHost);
        }

        // 9. Cleanup
        cudaFree(reinterpret_cast<void*>(d_params));
        cudaFree(reinterpret_cast<void*>(d_scratchBuffer));
        cudaFree(reinterpret_cast<void*>(d_accumulationBuffer));
        if (d_albedoBuffer) cudaFree(reinterpret_cast<void*>(d_albedoBuffer));
        if (d_normalBuffer) cudaFree(reinterpret_cast<void*>(d_normalBuffer));
        if (d_denoisedBuffer) cudaFree(reinterpret_cast<void*>(d_denoisedBuffer));
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

} // namespace rtx
