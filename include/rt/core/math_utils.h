#pragma once

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    __host__ __device__ constexpr float Radians(float degrees) {
        return degrees * (3.14159265358979323846f / 180.0f);
    }

}
