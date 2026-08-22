#pragma once
#include <cmath>
#include <algorithm>
#include "rt/core/vector3.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    __host__ __device__ inline float AlphaFromRoughness(float roughness) {
        float alpha = roughness * roughness;
        return std::max(alpha, 1e-3f);
    }

    __host__ __device__ inline float GgxD(float NdotH, float alpha) {
        if (NdotH <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float NdotH2 = NdotH * NdotH;
        float denom = NdotH2 * (alpha2 - 1.0f) + 1.0f;
        return alpha2 / (3.14159265358979323846f * denom * denom);
    }

    __host__ __device__ inline float SmithG(float NdotV, float NdotL, float alpha) {
        if (NdotV <= 0.0f || NdotL <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float lambdaV = NdotL * std::sqrt(std::max(0.0f, NdotV * NdotV * (1.0f - alpha2) + alpha2));
        float lambdaL = NdotV * std::sqrt(std::max(0.0f, NdotL * NdotL * (1.0f - alpha2) + alpha2));
        return 0.5f / (lambdaV + lambdaL);
    }

    __host__ __device__ inline Vector3f SampleGgxVndf(const Vector3f& wo, float alpha, float u1, float u2) {
        Vector3f wStretched = Normalize(Vector3f(alpha * wo.x, alpha * wo.y, wo.z));

        Vector3f T1 = (wStretched.z > 0.9999f) ? Vector3f(1.0f, 0.0f, 0.0f) : Normalize(Vector3f(-wStretched.y, wStretched.x, 0.0f));
        Vector3f T2 = Cross(wStretched, T1);

        Point2f p = ConcentricSampleDisk(Point2f(u1, u2));
        float p1 = p.x;
        float p2 = p.y;
        float s = 0.5f * (1.0f + wStretched.z);
        p2 = (1.0f - s) * std::sqrt(std::max(0.0f, 1.0f - p1 * p1)) + s * p2;

        float nStretchedZ = std::sqrt(std::max(0.0f, 1.0f - p1 * p1 - p2 * p2));
        Vector3f nStretched = p1 * T1 + p2 * T2 + nStretchedZ * wStretched;

        return Normalize(Vector3f(alpha * nStretched.x, alpha * nStretched.y, std::max(0.0f, nStretched.z)));
    }

    __host__ __device__ inline float GgxVndfPdf(float NdotV, float NdotH, float alpha) {
        if (NdotV <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float lambdaV = std::sqrt(std::max(0.0f, NdotV * NdotV * (1.0f - alpha2) + alpha2));
        float G1 = (2.0f * NdotV) / (NdotV + lambdaV);

        float D = GgxD(NdotH, alpha);
        
        return (G1 * D) / (4.0f * NdotV);
    }

    __host__ __device__ inline Vector3f SampleGgx(const Point2f& u, float alpha) {
        float phi = 2.0f * 3.14159265358979323846f * u.y;
        float cosTheta = std::sqrt(std::max(0.0f, (1.0f - u.x) / (1.0f + (alpha * alpha - 1.0f) * u.x)));
        float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
        return Vector3f(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
    }
}
