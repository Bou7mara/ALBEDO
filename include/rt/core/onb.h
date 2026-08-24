#pragma once
#include "rt/core/vector3.h"
#include <cmath>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    struct ONB {
        Vector3f u, v, w;

        constexpr __host__ __device__ ONB() = default;

        explicit __host__ __device__ ONB(const Vector3f& n) {
            w = n;

            float sign = (w.z >= 0.0f) ? 1.0f : -1.0f;

            float a = -1.0f / (sign + w.z);
            float b = w.x * w.y * a;

            u = Vector3f(1.0f + sign * w.x * w.x * a, sign * b, -sign * w.x);
            v = Vector3f(b, sign + w.y * w.y * a, -w.y);
        }

        __host__ __device__ Vector3f ToWorld(const Vector3f& local) const {
            return local.x * u + local.y * v + local.z * w;
        }
    };

}
