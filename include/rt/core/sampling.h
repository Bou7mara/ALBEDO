#pragma once
#include "rt/core/point2.h"
#include "rt/core/vector3.h"
#include <algorithm>
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

    inline constexpr float kPi = 3.14159265358979323846f;
    inline constexpr float kInvPi = 0.31830988618379067154f;

    __host__ __device__ inline Point2f ConcentricSampleDisk(const Point2f& u) {
        Point2f uOffset(2.0f * u.x - 1.0f, 2.0f * u.y - 1.0f);

        if (uOffset.x == 0.0f && uOffset.y == 0.0f) return Point2f(0.0f, 0.0f);

        float theta, r;
        constexpr float kPiOver4 = 3.14159265358979323846f / 4.0f;
        constexpr float kPiOver2 = 3.14159265358979323846f / 2.0f;

        if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
            r = uOffset.x;
            theta = kPiOver4 * (uOffset.y / uOffset.x);
        } else {
            r = uOffset.y;
            theta = kPiOver2 - kPiOver4 * (uOffset.x / uOffset.y);
        }
        return r * Point2f(std::cos(theta), std::sin(theta));
    }

    __host__ __device__ inline Vector3f CosineSampleHemisphere(const Point2f& u) {
        Point2f d = ConcentricSampleDisk(u);
        float z = std::sqrt(std::max(0.0f, 1.0f - d.x * d.x - d.y * d.y));
        return Vector3f(d.x, d.y, z);
    }

    __host__ __device__ inline float CosineHemispherePdf(float cosTheta) {
        return cosTheta * kInvPi;
    }

    __host__ __device__ inline Vector3f UniformSampleSphere(const Point2f& u) {
        float z = 1.0f - 2.0f * u.x;
        float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        float phi = 2.0f * kPi * u.y;
        return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
    }

    __host__ __device__ inline float UniformSpherePdf() {
        return 1.0f / (4.0f * kPi);
    }
}
