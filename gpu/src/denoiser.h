#pragma once

#include <optix.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include "rt/core/vector3.h"

namespace rtx {

    struct OptixDenoiserWrapper {
        OptixDenoiser handle = nullptr;
        CUdeviceptr stateBuffer = 0;
        CUdeviceptr scratchBuffer = 0;
        size_t stateSizeInBytes = 0;
        size_t scratchSizeInBytes = 0;

        static OptixDenoiserWrapper Create(OptixDeviceContext ctx,
                                           unsigned int width,
                                           unsigned int height,
                                           bool useGuideLayers = true);

        void Denoise(CUdeviceptr colorInputBuffer,
                     CUdeviceptr colorOutputBuffer,
                     CUdeviceptr albedoBuffer,
                     CUdeviceptr normalBuffer,
                     unsigned int width,
                     unsigned int height,
                     CUstream stream);

        void Destroy();
    };

}
