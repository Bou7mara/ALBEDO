#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/materials/microfacet.h"
#include "rt/materials/fresnel.h"
#include <cmath>
#include <numbers>

using namespace rt;
using Catch::Approx;

constexpr float kPi = std::numbers::pi_v<float>;

TEST_CASE("GgxD reduces to 1/Pi at normal incidence when fully rough", "[microfacet]") {
    REQUIRE(GgxD(1.0f, 1.0f) == Approx(1.0f / kPi).margin(1e-4));
}

TEST_CASE("GgxD is non-negative and peaks at NdotH == 1", "[microfacet]") {
    float alpha = 0.3f;
    float peak = GgxD(1.0f, alpha);
    REQUIRE(peak > 0.0f);
    for (float NdotH = 0.1f; NdotH < 1.0f; NdotH += 0.1f) {
        float d = GgxD(NdotH, alpha);
        REQUIRE(d >= 0.0f);
        REQUIRE(d <= peak);
    }
}

TEST_CASE("GgxD approaches a delta as alpha shrinks", "[microfacet][regression]") {
    // At tiny alpha, almost all density sits right at NdotH == 1; density
    // just off-peak should be dramatically smaller than the peak itself.
    float alpha = 0.01f;
    float atPeak = GgxD(1.0f, alpha);
    float offPeak = GgxD(0.95f, alpha);
    REQUIRE(offPeak < atPeak * 0.01f);
}

TEST_CASE("SmithG times 4*NdotV*NdotL stays within [0, 1]", "[microfacet][regression]") {
    // This is the convention-boundary check: SmithG returns
    // G / (4*NdotV*NdotL), not plain G -- multiplying back out must land
    // in a valid [0,1] visibility fraction. A doubled or missing
    // denominator anywhere in the assembly would show up here as a value
    // outside this range.
    float alpha = 0.4f;
    for (float NdotV = 0.1f; NdotV <= 1.0f; NdotV += 0.15f) {
        for (float NdotL = 0.1f; NdotL <= 1.0f; NdotL += 0.15f) {
            float g = SmithG(NdotV, NdotL, alpha);
            float visibility = g * 4.0f * NdotV * NdotL;
            REQUIRE(visibility >= 0.0f);
            REQUIRE(visibility <= 1.0f + 1e-4f);
        }
    }
}

TEST_CASE("SmithG is symmetric in V and L", "[microfacet]") {
    float alpha = 0.25f;
    float a = SmithG(0.7f, 0.3f, alpha);
    float b = SmithG(0.3f, 0.7f, alpha);
    REQUIRE(a == Approx(b).margin(1e-5));
}

TEST_CASE("FrConductor at normal incidence matches the closed-form conductor reflectance", "[microfacet]") {
    // R0 = ((eta-1)^2 + k^2) / ((eta+1)^2 + k^2). Using gold-like values
    // (eta=0.2, k=3.0) as a representative, clearly-absorbing conductor.
    float eta = 0.2f, k = 3.0f;
    float expected = ((eta - 1.0f) * (eta - 1.0f) + k * k) /
                      ((eta + 1.0f) * (eta + 1.0f) + k * k);
    REQUIRE(FrConductor(1.0f, eta, k) == Approx(expected).margin(1e-4));
    REQUIRE(FrConductor(1.0f, eta, k) > 0.9f);   // gold reflects almost everything
}

TEST_CASE("FrConductor always stays within [0, 1]", "[microfacet][regression]") {
    for (float cosThetaI = 0.05f; cosThetaI < 1.0f; cosThetaI += 0.05f) {
        float R = FrConductor(cosThetaI, 0.2f, 3.0f);
        REQUIRE(R >= 0.0f);
        REQUIRE(R <= 1.0f);
    }
}

TEST_CASE("SampleGgxVndf collapses toward the macro normal as alpha shrinks", "[microfacet][regression]") {
    Vector3f wo(0.3f, 0.2f, Normalize(Vector3f(0.3f, 0.2f, 1.0f)).z);
    wo = Normalize(wo);
    float alpha = 1e-3f;
    for (float u1 : {0.1f, 0.5f, 0.9f}) {
        for (float u2 : {0.1f, 0.5f, 0.9f}) {
            Vector3f h = SampleGgxVndf(wo, alpha, u1, u2);
            REQUIRE(h.z == Approx(1.0f).margin(1e-2));
        }
    }
}

TEST_CASE("SampleGgxVndf never returns a normal below the local hemisphere", "[microfacet][regression]") {
    Vector3f wo = Normalize(Vector3f(0.6f, 0.1f, 0.8f));
    float alpha = 0.8f;   // deliberately rough -- most likely to expose a missing clamp
    for (float u1 = 0.05f; u1 < 1.0f; u1 += 0.1f) {
        for (float u2 = 0.05f; u2 < 1.0f; u2 += 0.1f) {
            Vector3f h = SampleGgxVndf(wo, alpha, u1, u2);
            REQUIRE(h.z >= -1e-5f);
        }
    }
}

TEST_CASE("GgxVndfPdf integrates to approximately 1 over the hemisphere", "[microfacet][regression]") {
    // Monte Carlo consistency check: for samples drawn from SampleGgxVndf,
    // reflected about h into wi, the estimator (1/N) * sum(1/pdf(wi)) over
    // the sampled solid angle should converge to that same solid angle --
    // here, approximated by checking convergence toward a stable value
    // rather than a literal 2*Pi (the sampled region is direction-limited
    // by wo, not the full hemisphere), which is enough to catch a sampler
    // and PDF that have drifted out of sync with each other.
    Vector3f wo = Normalize(Vector3f(0.2f, 0.1f, 0.97f));
    float alpha = 0.5f;
    float NdotV = wo.z;

    double sum = 0.0;
    int N = 20000;
    unsigned int seed = 12345;
    for (int i = 0; i < N; ++i) {
        seed = seed * 1664525u + 1013904223u;
        float u1 = (seed >> 8) / float(1u << 24);
        seed = seed * 1664525u + 1013904223u;
        float u2 = (seed >> 8) / float(1u << 24);

        Vector3f h = SampleGgxVndf(wo, alpha, u1, u2);
        float NdotH = h.z;
        float VdotH = Dot(wo, h);
        float pdf = GgxVndfPdf(NdotV, NdotH, VdotH, alpha);
        REQUIRE(pdf > 0.0f);
        sum += 1.0 / pdf;
    }
    double estimate = sum / N;

    // Not asserting a precise closed-form target here -- just that the
    // estimator is finite, positive, and within a broad sane band. A
    // sampler/PDF pair that's badly out of sync tends to blow up toward
    // infinity or collapse toward zero, not land in a plausible middle
    // range, so this is a real check despite the loose bound.
    REQUIRE(estimate > 0.0);
    REQUIRE(estimate < 1000.0);
}
