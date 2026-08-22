#include "denoiser.h"

#include <optix_stubs.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace rtx {

    namespace {

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

        OptixImage2D MakeImage2D(CUdeviceptr buffer, unsigned int width, unsigned int height) {
            OptixImage2D img{};
            img.data = buffer;
            img.width = width;
            img.height = height;
            img.rowStrideInBytes = width * sizeof(rt::Vector3f);
            img.pixelStrideInBytes = sizeof(rt::Vector3f);
            img.format = OPTIX_PIXEL_FORMAT_FLOAT3;
            return img;
        }

    } // namespace

    OptixDenoiserWrapper OptixDenoiserWrapper::Create(OptixDeviceContext ctx,
                                                     unsigned int width,
                                                     unsigned int height,
                                                     bool useGuideLayers) {
        OptixDenoiserWrapper wrapper{};

        OptixDenoiserOptions options{};
        options.guideAlbedo = useGuideLayers ? 1 : 0;
        options.guideNormal = useGuideLayers ? 1 : 0;

        OPTIX_CHECK(optixDenoiserCreate(ctx, OPTIX_DENOISER_MODEL_KIND_HDR, &options, &wrapper.handle));

        OptixDenoiserSizes sizes{};
        OPTIX_CHECK(optixDenoiserComputeMemoryResources(wrapper.handle, width, height, &sizes));
        wrapper.stateSizeInBytes = sizes.stateSizeInBytes;
        wrapper.scratchSizeInBytes = sizes.withoutOverlapScratchSizeInBytes;

        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&wrapper.stateBuffer), wrapper.stateSizeInBytes));
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&wrapper.scratchBuffer), wrapper.scratchSizeInBytes));

        OPTIX_CHECK(optixDenoiserSetup(
            wrapper.handle,
            nullptr,
            width,
            height,
            wrapper.stateBuffer,
            wrapper.stateSizeInBytes,
            wrapper.scratchBuffer,
            wrapper.scratchSizeInBytes
        ));

        return wrapper;
    }

    void OptixDenoiserWrapper::Denoise(CUdeviceptr colorInputBuffer,
                                       CUdeviceptr colorOutputBuffer,
                                       CUdeviceptr albedoBuffer,
                                       CUdeviceptr normalBuffer,
                                       unsigned int width,
                                       unsigned int height,
                                       CUstream stream) {
        OptixDenoiserLayer layer{};
        layer.input = MakeImage2D(colorInputBuffer, width, height);
        layer.output = MakeImage2D(colorOutputBuffer, width, height);

        OptixDenoiserGuideLayer guideLayer{};
        if (albedoBuffer) {
            guideLayer.albedo = MakeImage2D(albedoBuffer, width, height);
        }
        if (normalBuffer) {
            guideLayer.normal = MakeImage2D(normalBuffer, width, height);
        }

        OptixDenoiserParams params{};
        params.blendFactor = 0.0f; // 0.0 = full denoised image

        OPTIX_CHECK(optixDenoiserInvoke(
            handle,
            stream,
            &params,
            stateBuffer,
            stateSizeInBytes,
            &guideLayer,
            &layer,
            1,
            0,
            0,
            scratchBuffer,
            scratchSizeInBytes
        ));
    }

    void OptixDenoiserWrapper::Destroy() {
        if (scratchBuffer) {
            cudaFree(reinterpret_cast<void*>(scratchBuffer));
            scratchBuffer = 0;
        }
        if (stateBuffer) {
            cudaFree(reinterpret_cast<void*>(stateBuffer));
            stateBuffer = 0;
        }
        if (handle) {
            optixDenoiserDestroy(handle);
            handle = nullptr;
        }
    }

} // namespace rtx
