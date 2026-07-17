#pragma once
#include <cmath>
#include <algorithm>
#include <numbers>
#include "rt/core/vector3.h"
#include "rt/core/onb.h"

namespace rt {

    // Artist-facing roughness in [0,1] to GGX's alpha shape parameter.
    // Squaring roughness (Disney's remapping, now close to universal) gives a
    // perceptually more linear roughness slider than using alpha directly --
    // this is the one place that mapping lives; nothing else in the codebase
    // should compute alpha by hand.
    inline float AlphaFromRoughness(float roughness) {
        float alpha = roughness * roughness;
        return std::max(alpha, 1e-3f);
    }

    // GGX / Trowbridge-Reitz normal distribution function (TOC 6.6.2). Answers
    // "what fraction of microfacets have their normal aligned with H" -- this
    // is D in the Cook-Torrance f = D*G*F / (4*cosThetaI*cosThetaO) formula.
    // alpha is the GGX shape parameter, NOT the artist-facing roughness slider
    // -- see AlphaFromRoughness below for that conversion. NdotH must already
    // be computed by the caller (Dot(n, h)); this function takes the cosine,
    // not the vectors, since it's called from inner sampling loops where the
    // caller already has it on hand.
    inline float GgxD(float NdotH, float alpha) {
        if (NdotH <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float NdotH2 = NdotH * NdotH;
        float denom = NdotH2 * (alpha2 - 1.0f) + 1.0f;
        return alpha2 / (std::numbers::pi_v<float> * denom * denom);
    }

    // Height-correlated Smith geometric shadowing-masking term (TOC 6.6.1),
    // for the GGX distribution specifically. Takes both view and light cosines
    // together -- this is one physical quantity (the joint probability that a
    // facet is unshadowed AND unmasked), not two independent ones a caller
    // should multiply by hand.
    //
    // Note: this returns G / (4 * NdotV * NdotL), NOT G alone. It folds in
    // the 4 * cos(theta_v) * cos(theta_l) Cook-Torrance denominator term.
    inline float SmithG(float NdotV, float NdotL, float alpha) {
        if (NdotV <= 0.0f || NdotL <= 0.0f) {
            return 0.0f;
        }
        float alpha2 = alpha * alpha;
        float lambdaV = NdotL * std::sqrt(std::max(0.0f, NdotV * NdotV * (1.0f - alpha2) + alpha2));
        float lambdaL = NdotV * std::sqrt(std::max(0.0f, NdotL * NdotL * (1.0f - alpha2) + alpha2));
        return 0.5f / (lambdaV + lambdaL);
    }

    // Sample a microfacet normal from the GGX Visible Normal Distribution
    // Function (VNDF), given the outgoing direction wo (Heitz 2018,
    // "Sampling the GGX Distribution of Visible Normals"). This is what makes
    // GGX importance sampling efficient at grazing angles -- every sample is
    // weighted by visibility from wo already, so no samples are wasted on
    // self-shadowed facets the way naive D-sampling would waste them.
    //
    // CONTRACT: wo must already be expressed in local shading space, i.e. the
    // frame where the macro surface normal is (0,0,1) -- this function has no
    // knowledge of the true surface normal or the world-space frame. The
    // caller (Microfacet::Sample_f) is responsible for building that frame
    // (same OrthonormalBasis machinery as TOC 3.3.3) and transforming both wo
    // in and the returned half-vector back out to world space. wo.z must be
    // >= 0 (i.e. wo is on the same side as the local +z normal); this
    // function does not flip it for you, matching the no-flipping convention
    // already established for Reflect/Refract in fresnel.h.
    //
    // u1, u2: two independent uniform random numbers in [0,1), same role as
    // the Point2f u already used throughout the BSDF interface.
    inline Vector3f SampleGgxVndf(const Vector3f& wo, float alpha, float u1, float u2) {
        // 1. Stretch the view direction into the space where the distribution
        //    is a hemisphere (GGX is an affine-stretched hemisphere in disguise)
        Vector3f wStretched = Normalize(Vector3f(alpha * wo.x, alpha * wo.y, wo.z));

        // 2. Build an orthonormal basis (T1, T2) around wStretched.
        //    Using the same near-degenerate handling pattern from onb.h.
        ONB onb(wStretched);
        Vector3f T1 = onb.u;
        Vector3f T2 = onb.v;

        // 3. Sample a point on the projected disk with a low-distortion
        //    concentric mapping (uniform disk sampling)
        float r = std::sqrt(u1);
        float phi = 2.0f * std::numbers::pi_v<float> * u2;
        float p1 = r * std::cos(phi);
        float p2 = r * std::sin(phi);
        float s = 0.5f * (1.0f + wStretched.z);
        p2 = (1.0f - s) * std::sqrt(std::max(0.0f, 1.0f - p1 * p1)) + s * p2;

        // 4. Compute the normal in stretched space
        float nStretchedZ = std::sqrt(std::max(0.0f, 1.0f - p1 * p1 - p2 * p2));
        Vector3f nStretched = p1 * T1 + p2 * T2 + nStretchedZ * wStretched;

        // 5. Unstretch back to the true (non-affine) space and renormalize
        return Normalize(Vector3f(alpha * nStretched.x, alpha * nStretched.y, std::max(0.0f, nStretched.z)));
    }

    // PDF of the half-vector produced by SampleGgxVndf, converted from
    // half-vector measure to incident-direction (wi) measure -- this is the
    // density Sample_f must report, not the raw VNDF value, since the
    // rendering equation's Monte Carlo estimator integrates over wi, and the
    // Jacobian of the half-vector-to-wi reflection map is 1/(4*Dot(wo,h)).
    // NdotV, NdotH, VdotH, and alpha must all correspond to the SAME sample
    // SampleGgxVndf just produced -- this is not a general-purpose PDF
    // evaluator for an arbitrary half-vector, it is paired 1:1 with the
    // sampling routine above.
    inline float GgxVndfPdf(float NdotV, float NdotH, float VdotH, float alpha) {
        if (NdotV <= 0.0f) {
            return 0.0f;
        }
        // G1 = the single-direction Smith term for NdotV alone.
        // Note: this is different from SmithG, which is the height-correlated
        // two-direction masking-shadowing term (G2).
        float alpha2 = alpha * alpha;
        float lambdaV = std::sqrt(std::max(0.0f, NdotV * NdotV * (1.0f - alpha2) + alpha2));
        float G1 = (2.0f * NdotV) / (NdotV + lambdaV);

        float D = GgxD(NdotH, alpha);
        
        // return (G1 * VdotH * D) / (NdotV * 4.0f * VdotH), simplifying to:
        return (G1 * D) / (4.0f * NdotV);
    }

}
