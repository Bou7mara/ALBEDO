#pragma once
#include <cmath>
#include <algorithm>
#include <numbers>
#include "rt/core/vector3.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"

namespace rt {

    // Converts perceptual roughness value in [0, 1] to GGX alpha roughness parameter (clamped above 0.001)
    inline float AlphaFromRoughness(float roughness) {
        float alpha = roughness * roughness;
        return std::max(alpha, 1e-3f);
    }

    // Calculates GGX Normal Distribution Function D(H)
    // Measures the relative area of microfacets oriented in direction H
    inline float GgxD(float NdotH, float alpha) {
        if (NdotH <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float NdotH2 = NdotH * NdotH;
        float denom = NdotH2 * (alpha2 - 1.0f) + 1.0f;
        return alpha2 / (std::numbers::pi_v<float> * denom * denom);
    }

    // Calculates Smith joint height-correlated masking-shadowing function G(V, L) divided by (4 * NdotV * NdotL)
    // Measures the fraction of microfacets visible from both viewing direction V and lighting direction L
    inline float SmithG(float NdotV, float NdotL, float alpha) {
        if (NdotV <= 0.0f || NdotL <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float lambdaV = NdotL * std::sqrt(std::max(0.0f, NdotV * NdotV * (1.0f - alpha2) + alpha2));
        float lambdaL = NdotV * std::sqrt(std::max(0.0f, NdotL * NdotL * (1.0f - alpha2) + alpha2));
        return 0.5f / (lambdaV + lambdaL);
    }

    // Samples microfacet normal H from the Visible Normal Distribution Function (VNDF) using Heitz (2018) algorithm
    // wo is view direction, alpha is GGX roughness, u1 and u2 are random numbers
    inline Vector3f SampleGgxVndf(const Vector3f& wo, float alpha, float u1, float u2) {
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

    // Calculates PDF for sampling reflection direction wi generated from the VNDF microfacet distribution
    inline float GgxVndfPdf(float NdotV, float NdotH, float alpha) {
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

