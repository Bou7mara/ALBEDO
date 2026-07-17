#pragma once
#include <cmath>
#include <algorithm>
#include <numbers>
#include "rt/core/vector3.h"
#include "rt/core/onb.h"

namespace rt {

    inline float AlphaFromRoughness(float roughness) {
        float alpha = roughness * roughness;
        return std::max(alpha, 1e-3f);
    }

    inline float GgxD(float NdotH, float alpha) {
        if (NdotH <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float NdotH2 = NdotH * NdotH;
        float denom = NdotH2 * (alpha2 - 1.0f) + 1.0f;
        return alpha2 / (std::numbers::pi_v<float> * denom * denom);
    }

    inline float SmithG(float NdotV, float NdotL, float alpha) {
        if (NdotV <= 0.0f || NdotL <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float lambdaV = NdotL * std::sqrt(std::max(0.0f, NdotV * NdotV * (1.0f - alpha2) + alpha2));
        float lambdaL = NdotV * std::sqrt(std::max(0.0f, NdotL * NdotL * (1.0f - alpha2) + alpha2));
        return 0.5f / (lambdaV + lambdaL);
    }

    inline Vector3f SampleGgxVndf(const Vector3f& wo, float alpha, float u1, float u2) {
        Vector3f wStretched = Normalize(Vector3f(alpha * wo.x, alpha * wo.y, wo.z));

        ONB onb(wStretched);
        Vector3f T1 = onb.u;
        Vector3f T2 = onb.v;

        float r = std::sqrt(u1);
        float phi = 2.0f * std::numbers::pi_v<float> * u2;
        float p1 = r * std::cos(phi);
        float p2 = r * std::sin(phi);
        float s = 0.5f * (1.0f + wStretched.z);
        p2 = (1.0f - s) * std::sqrt(std::max(0.0f, 1.0f - p1 * p1)) + s * p2;

        float nStretchedZ = std::sqrt(std::max(0.0f, 1.0f - p1 * p1 - p2 * p2));
        Vector3f nStretched = p1 * T1 + p2 * T2 + nStretchedZ * wStretched;

        return Normalize(Vector3f(alpha * nStretched.x, alpha * nStretched.y, std::max(0.0f, nStretched.z)));
    }

    inline float GgxVndfPdf(float NdotV, float NdotH, float VdotH, float alpha) {
        if (NdotV <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float lambdaV = std::sqrt(std::max(0.0f, NdotV * NdotV * (1.0f - alpha2) + alpha2));
        float G1 = (2.0f * NdotV) / (NdotV + lambdaV);

        float D = GgxD(NdotH, alpha);
        
        return (G1 * D) / (4.0f * NdotV);
    }
}
