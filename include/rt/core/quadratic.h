#pragma once
#include <cmath>
#include <utility>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    inline __host__ __device__ bool Quadratic(float a, float b, float c, float* t0, float* t1) {
        double discrim = double(b) * double(b) - 4.0 * double(a) * double(c);
        if (discrim < 0.0) return false;

        double rootDiscrim = std::sqrt(discrim);

        double q = (b < 0) ? -0.5 * (b - rootDiscrim) : -0.5 * (b + rootDiscrim);
        double r0 = q / a;
        double r1 = c / q;
        
        if (r0 > r1) {
            double tmp = r0;
            r0 = r1;
            r1 = tmp;
        }

        *t0 = static_cast<float>(r0);
        *t1 = static_cast<float>(r1);
        return true;
    }

} // namespace rt
