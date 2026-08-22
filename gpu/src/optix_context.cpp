#include "optix_context.h"

#include <optix_stubs.h>
#include <optix_function_table_definition.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace rtx {

    namespace {

        void OptixLogCallback(unsigned int level, const char* tag, const char* message, void* /*cbdata*/) {
            std::cerr << "[OptiX][" << level << "][" << (tag ? tag : "STATUS") << "]: "
                      << (message ? message : "") << "\n";
        }

        #define CUDA_CHECK(call)                                                                     \
            do {                                                                                     \
                cudaError_t error = (call);                                                          \
                if (error != cudaSuccess) {                                                          \
                    std::ostringstream ss;                                                           \
                    ss << "CUDA Error (" << cudaGetErrorName(error) << "): "                         \
                       << cudaGetErrorString(error) << " at " << __FILE__ << ":" << __LINE__;       \
                    throw std::runtime_error(ss.str());                                              \
                }                                                                                    \
            } while (0)

        #define CU_CHECK(call)                                                                       \
            do {                                                                                     \
                CUresult res = (call);                                                               \
                if (res != CUDA_SUCCESS) {                                                           \
                    const char* errName = nullptr;                                                   \
                    cuGetErrorName(res, &errName);                                                   \
                    std::ostringstream ss;                                                           \
                    ss << "CUDA Driver Error (" << (errName ? errName : "UNKNOWN") << "): "         \
                       << "code " << res << " at " << __FILE__ << ":" << __LINE__;                  \
                    throw std::runtime_error(ss.str());                                              \
                }                                                                                    \
            } while (0)

        #define OPTIX_CHECK(call)                                                                    \
            do {                                                                                     \
                OptixResult res = (call);                                                            \
                if (res != OPTIX_SUCCESS) {                                                          \
                    std::ostringstream ss;                                                           \
                    ss << "OptiX Error (" << optixGetErrorName(res) << "): "                         \
                       << optixGetErrorString(res) << " at " << __FILE__ << ":" << __LINE__;        \
                    throw std::runtime_error(ss.str());                                              \
                }                                                                                    \
            } while (0)

    } // namespace

    OptixContext::OptixContext() = default;

    OptixContext::~OptixContext() {
        if (optixContext_) {
            optixDeviceContextDestroy(optixContext_);
            optixContext_ = nullptr;
        }
        if (stream_) {
            cudaStreamDestroy(stream_);
            stream_ = nullptr;
        }
    }

    OptixContext::OptixContext(OptixContext&& other) noexcept
        : deviceId_(other.deviceId_),
          deviceName_(std::move(other.deviceName_)),
          cudaContext_(other.cudaContext_),
          optixContext_(other.optixContext_),
          stream_(other.stream_) {
        other.cudaContext_ = nullptr;
        other.optixContext_ = nullptr;
        other.stream_ = nullptr;
    }

    OptixContext& OptixContext::operator=(OptixContext&& other) noexcept {
        if (this != &other) {
            if (optixContext_) optixDeviceContextDestroy(optixContext_);
            if (stream_) cudaStreamDestroy(stream_);

            deviceId_ = other.deviceId_;
            deviceName_ = std::move(other.deviceName_);
            cudaContext_ = other.cudaContext_;
            optixContext_ = other.optixContext_;
            stream_ = other.stream_;

            other.cudaContext_ = nullptr;
            other.optixContext_ = nullptr;
            other.stream_ = nullptr;
        }
        return *this;
    }

    std::unique_ptr<OptixContext> OptixContext::Create() {
        auto ctx = std::make_unique<OptixContext>();

        // 1. Initialize CUDA Driver & Runtime
        CU_CHECK(cuInit(0));

        int deviceCount = 0;
        CUDA_CHECK(cudaGetDeviceCount(&deviceCount));
        if (deviceCount == 0) {
            throw std::runtime_error("No CUDA-capable GPU detected!");
        }

        ctx->deviceId_ = 0;
        CUDA_CHECK(cudaSetDevice(ctx->deviceId_));
        CUDA_CHECK(cudaFree(0)); // Initialize CUDA context for the active thread

        cudaDeviceProp prop{};
        CUDA_CHECK(cudaGetDeviceProperties(&prop, ctx->deviceId_));
        ctx->deviceName_ = prop.name;

        CU_CHECK(cuCtxGetCurrent(&ctx->cudaContext_));
        if (!ctx->cudaContext_) {
            throw std::runtime_error("Failed to acquire active CUDA context!");
        }

        CUDA_CHECK(cudaStreamCreate(&ctx->stream_));

        // 2. Initialize OptiX runtime function table
        OPTIX_CHECK(optixInit());

        // 3. Create OptixDeviceContext
        OptixDeviceContextOptions options{};
        options.logCallbackFunction = &OptixLogCallback;
        options.logCallbackData = nullptr;
        options.logCallbackLevel = 4;

        OPTIX_CHECK(optixDeviceContextCreate(ctx->cudaContext_, &options, &ctx->optixContext_));

        return ctx;
    }

} // namespace rtx
