#pragma once
#include "rt/core/vector3.h"
#include <algorithm>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {
    inline constexpr int kRRStartDepth = 3;
    inline constexpr float kRRProbabilityMinimumThreshold = 0.5f;
    inline constexpr float kRRProbabilityMaximumThreshold = 0.95f;

    [[nodiscard]] __host__ __device__ inline constexpr float MaxChannel(const Vector3f& v) {
        return (v.x > v.y) ? ((v.x > v.z) ? v.x : v.z) : ((v.y > v.z) ? v.y : v.z);
    }

    __host__ __device__ inline float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
        float f = nf * fPdf;
        float g = ng * gPdf;
        float f2 = f * f;
        float g2 = g * g;
        if (f2 + g2 == 0.0f) return 0.0f;
        return f2 / (f2 + g2);
    }
}
