#include "optix_context.h"
#include "launch_params.h"
#include "rt/core/vector3.h"
#include "rt/io/png_writer.h"

#include <optix_stubs.h>
#include <optix_stack_size.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <stdexcept>

namespace {

#define CUDA_CHECK(call)                                                                     \
    do {                                                                                     \
        cudaError_t error = (call);                                                          \
        if (error != cudaSuccess) {                                                          \
            std::cerr << "CUDA Error (" << cudaGetErrorName(error) << "): "                  \
                      << cudaGetErrorString(error) << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            return 1;                                                                        \
        }                                                                                    \
    } while (0)

#define OPTIX_CHECK(call)                                                                    \
    do {                                                                                     \
        OptixResult res = (call);                                                            \
        if (res != OPTIX_SUCCESS) {                                                          \
            std::cerr << "OptiX Error (" << optixGetErrorName(res) << "): "                  \
                      << optixGetErrorString(res) << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            return 1;                                                                        \
        }                                                                                    \
    } while (0)

    struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) RaygenRecord {
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
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

    std::filesystem::path FindShaderBinary(const std::string& filename, const char* argv0) {
        std::vector<std::filesystem::path> candidates = {
            std::filesystem::current_path() / "optixir" / filename,
            std::filesystem::current_path() / filename,
            std::filesystem::path(argv0).parent_path() / "optixir" / filename,
            std::filesystem::path(argv0).parent_path() / filename,
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

int main(int argc, char* argv[]) {
    int width = 1280;
    int height = 720;

    if (argc > 1) width = std::atoi(argv[1]);
    if (argc > 2) height = std::atoi(argv[2]);

    std::cout << "==================================================\n"
              << " ALBEDO GPU Smoke Test (Milestone 1)\n"
              << "==================================================\n";

    try {

        auto ctx = rtx::OptixContext::Create();
        OptixDeviceContext optixContext = ctx->GetOptixDeviceContext();
        CUstream stream = ctx->GetCudaStream();

        int driverVersion = 0;
        cudaDriverGetVersion(&driverVersion);
        int runtimeVersion = 0;
        cudaRuntimeGetVersion(&runtimeVersion);

        std::cout << "GPU Device:      " << ctx->GetDeviceName() << " (ID " << ctx->GetDeviceId() << ")\n"
                  << "NVIDIA Driver:   CUDA Version " << (driverVersion / 1000) << "." << ((driverVersion % 100) / 10) << "\n"
                  << "CUDA Runtime:    " << (runtimeVersion / 1000) << "." << ((runtimeVersion % 100) / 10) << "\n"
                  << "OptiX Version:   " << (OPTIX_VERSION / 10000) << "." << ((OPTIX_VERSION % 10000) / 100) << "." << (OPTIX_VERSION % 100) << "\n"
                  << "Resolution:      " << width << "x" << height << "\n\n";

        std::filesystem::path shaderPath = FindShaderBinary("smoke_test.optixir", argv[0]);
        std::cout << "Loading OptiX-IR: " << shaderPath.string() << "\n";
        std::vector<char> optixirCode = ReadBinaryFile(shaderPath);

        OptixModuleCompileOptions moduleCompileOptions{};
        OptixPipelineCompileOptions pipelineCompileOptions{};
        pipelineCompileOptions.usesMotionBlur = 0;
        pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY;
        pipelineCompileOptions.numPayloadValues = 0;
        pipelineCompileOptions.numAttributeValues = 0;
        pipelineCompileOptions.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
        pipelineCompileOptions.pipelineLaunchParamsVariableName = "params";

        OptixModule module = nullptr;
        char logBuffer[2048] = {0};
        size_t logSize = sizeof(logBuffer);

        OPTIX_CHECK(optixModuleCreate(
            optixContext,
            &moduleCompileOptions,
            &pipelineCompileOptions,
            optixirCode.data(),
            optixirCode.size(),
            logBuffer,
            &logSize,
            &module
        ));
        if (logSize > 1) {
            std::cout << "[OptiX Module Log]: " << logBuffer << "\n";
        }

        OptixProgramGroupDesc pgDesc{};
        pgDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
        pgDesc.raygen.module = module;
        pgDesc.raygen.entryFunctionName = "__raygen__smoke_test";

        OptixProgramGroupOptions pgOptions{};
        OptixProgramGroup raygenPG = nullptr;
        logSize = sizeof(logBuffer);

        OPTIX_CHECK(optixProgramGroupCreate(
            optixContext,
            &pgDesc,
            1,
            &pgOptions,
            logBuffer,
            &logSize,
            &raygenPG
        ));
        if (logSize > 1) {
            std::cout << "[OptiX ProgramGroup Log]: " << logBuffer << "\n";
        }

        OptixPipelineLinkOptions linkOptions{};
        linkOptions.maxTraceDepth = 1;

        OptixPipeline pipeline = nullptr;
        logSize = sizeof(logBuffer);

        OPTIX_CHECK(optixPipelineCreate(
            optixContext,
            &pipelineCompileOptions,
            &linkOptions,
            &raygenPG,
            1,
            logBuffer,
            &logSize,
            &pipeline
        ));
        if (logSize > 1) {
            std::cout << "[OptiX Pipeline Log]: " << logBuffer << "\n";
        }

        RaygenRecord raygenRecord{};
        OPTIX_CHECK(optixSbtRecordPackHeader(raygenPG, &raygenRecord));

        CUdeviceptr d_raygenRecord = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_raygenRecord), sizeof(RaygenRecord)));
        CUDA_CHECK(cudaMemcpy(
            reinterpret_cast<void*>(d_raygenRecord),
            &raygenRecord,
            sizeof(RaygenRecord),
            cudaMemcpyHostToDevice
        ));

        OptixShaderBindingTable sbt{};
        sbt.raygenRecord = d_raygenRecord;

        size_t bufferSize = static_cast<size_t>(width) * height * sizeof(rt::Vector3f);
        CUdeviceptr d_outputBuffer = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_outputBuffer), bufferSize));

        rtx::LaunchParams params{};
        params.width = static_cast<unsigned int>(width);
        params.height = static_cast<unsigned int>(height);
        params.outputBuffer = reinterpret_cast<rt::Vector3f*>(d_outputBuffer);

        CUdeviceptr d_params = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(rtx::LaunchParams)));
        CUDA_CHECK(cudaMemcpy(
            reinterpret_cast<void*>(d_params),
            &params,
            sizeof(rtx::LaunchParams),
            cudaMemcpyHostToDevice
        ));

        std::cout << "Launching OptiX pipeline on GPU grid " << width << "x" << height << "...\n";
        OPTIX_CHECK(optixLaunch(
            pipeline,
            stream,
            d_params,
            sizeof(rtx::LaunchParams),
            &sbt,
            static_cast<unsigned int>(width),
            static_cast<unsigned int>(height),
            1
        ));
        CUDA_CHECK(cudaStreamSynchronize(stream));
        std::cout << "OptiX kernel execution completed successfully.\n";

        std::vector<rt::Vector3f> framebuffer(width * height);
        CUDA_CHECK(cudaMemcpy(
            framebuffer.data(),
            reinterpret_cast<void*>(d_outputBuffer),
            bufferSize,
            cudaMemcpyDeviceToHost
        ));

        std::string pngPath = "smoke_test.png";
        if (rt::WritePNG(pngPath, width, height, framebuffer)) {
            std::cout << "Successfully saved GPU output image to " << pngPath << "\n";
        } else {
            std::cerr << "Failed to write PNG image to " << pngPath << "!\n";
            return 1;
        }

        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_params)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_outputBuffer)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_raygenRecord)));

        OPTIX_CHECK(optixPipelineDestroy(pipeline));
        OPTIX_CHECK(optixProgramGroupDestroy(raygenPG));
        OPTIX_CHECK(optixModuleDestroy(module));

        std::cout << "Milestone 1 smoke test completed with 0 errors.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
}
