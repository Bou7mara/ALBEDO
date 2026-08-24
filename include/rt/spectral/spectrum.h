#pragma once
#include "rt/core/vector3.h"
#include <cmath>
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

    constexpr float kLambdaMin = 380.0f;
    constexpr float kLambdaMax = 730.0f;
    constexpr float kLambdaRange = kLambdaMax - kLambdaMin;
    constexpr float kCieYIntegral = 106.856917f;

    struct HeroWavelengths {
        float lambda[4];
        float pdf;

        __host__ __device__ constexpr HeroWavelengths()
            : lambda{ 555.0f, 642.5f, 467.5f, 380.0f }, pdf(1.0f / 350.0f) {}

        __host__ __device__ constexpr HeroWavelengths(float l0, float l1, float l2, float l3, float p)
            : lambda{ l0, l1, l2, l3 }, pdf(p) {}
    };

    __host__ __device__ inline HeroWavelengths SampleHeroWavelengths(float u) {
        float hero = kLambdaMin + u * kLambdaRange;
        HeroWavelengths hw;
        hw.pdf = 1.0f / kLambdaRange;
        for (int i = 0; i < 4; ++i) {
            float offset = (kLambdaRange / 4.0f) * static_cast<float>(i);
            float l = hero + offset;
            if (l >= kLambdaMax) l -= kLambdaRange;
            hw.lambda[i] = l;
        }
        return hw;
    }

    __host__ __device__ inline float Gaussian(float x, float mu, float sigma1, float sigma2) {
        float sigma = (x < mu) ? sigma1 : sigma2;
        float d = (x - mu) / sigma;
        return std::exp(-0.5f * d * d);
    }

    __host__ __device__ inline float CieX(float lambdaNm) {
        return 1.056f * Gaussian(lambdaNm, 599.8f, 37.9f, 31.0f)
             + 0.362f * Gaussian(lambdaNm, 442.0f, 16.0f, 26.7f)
             - 0.065f * Gaussian(lambdaNm, 501.1f, 20.4f, 18.9f);
    }

    __host__ __device__ inline float CieY(float lambdaNm) {
        return 0.821f * Gaussian(lambdaNm, 568.8f, 46.9f, 40.5f)
             + 0.286f * Gaussian(lambdaNm, 530.9f, 16.3f, 31.1f);
    }

    __host__ __device__ inline float CieZ(float lambdaNm) {
        return 1.217f * Gaussian(lambdaNm, 437.0f, 11.8f, 36.0f)
             + 0.681f * Gaussian(lambdaNm, 459.0f, 26.0f, 13.8f);
    }

    __host__ __device__ inline Vector3f XyzToSrgb(const Vector3f& xyz) {
        return Vector3f(
             3.079953f * xyz.x - 1.5371385f * xyz.y - 0.542816f * xyz.z,
            -0.921258f * xyz.x + 1.8760108f * xyz.y + 0.0452474f * xyz.z,
             0.0528874f * xyz.x - 0.2040259f * xyz.y + 1.1511385f * xyz.z
        );
    }

    __host__ __device__ inline Vector3f SpectralToRgb(const HeroWavelengths& hw, const float spectralL[4]) {
        Vector3f xyz(0.0f, 0.0f, 0.0f);
        float factor = (kLambdaRange / 4.0f) / kCieYIntegral;
        for (int i = 0; i < 4; ++i) {
            float l = hw.lambda[i];
            float s = spectralL[i];
            xyz.x += CieX(l) * s * factor;
            xyz.y += CieY(l) * s * factor;
            xyz.z += CieZ(l) * s * factor;
        }
        return XyzToSrgb(xyz);
    }

    __host__ __device__ inline float RgbToSpectrum(const Vector3f& rgb, float lambdaNm) {
        if (lambdaNm < 490.0f) {
            float t = std::clamp((lambdaNm - 380.0f) / (490.0f - 380.0f), 0.0f, 1.0f);
            return (1.0f - t) * rgb.z + t * (0.5f * rgb.z + 0.5f * rgb.y);
        } else if (lambdaNm < 570.0f) {
            float t = std::clamp((lambdaNm - 490.0f) / (570.0f - 490.0f), 0.0f, 1.0f);
            return (1.0f - t) * (0.5f * rgb.z + 0.5f * rgb.y) + t * (0.5f * rgb.y + 0.5f * rgb.x);
        } else {
            float t = std::clamp((lambdaNm - 570.0f) / (730.0f - 570.0f), 0.0f, 1.0f);
            return (1.0f - t) * (0.5f * rgb.y + 0.5f * rgb.x) + t * rgb.x;
        }
    }

}
