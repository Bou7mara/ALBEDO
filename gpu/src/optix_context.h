#pragma once

#include <cuda.h>
#include <cuda_runtime.h>
#include <optix.h>
#include <memory>
#include <string>

namespace rtx {

    class OptixContext {
    public:
        OptixContext();
        ~OptixContext();

        // Non-copyable, movable
        OptixContext(const OptixContext&) = delete;
        OptixContext& operator=(const OptixContext&) = delete;
        OptixContext(OptixContext&& other) noexcept;
        OptixContext& operator=(OptixContext&& other) noexcept;

        [[nodiscard]] static std::unique_ptr<OptixContext> Create();

        [[nodiscard]] OptixDeviceContext GetOptixDeviceContext() const noexcept { return optixContext_; }
        [[nodiscard]] CUcontext GetCudaContext() const noexcept { return cudaContext_; }
        [[nodiscard]] CUstream GetCudaStream() const noexcept { return stream_; }
        [[nodiscard]] int GetDeviceId() const noexcept { return deviceId_; }
        [[nodiscard]] std::string GetDeviceName() const { return deviceName_; }

    private:
        int deviceId_ = 0;
        std::string deviceName_;
        CUcontext cudaContext_ = nullptr;
        OptixDeviceContext optixContext_ = nullptr;
        CUstream stream_ = nullptr;
    };

} // namespace rtx
