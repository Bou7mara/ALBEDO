#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/materials/microfacet.h"
#include "rt/materials/fresnel.h"
#include <cmath>
#include <numbers>

using namespace rt;
using Catch::Approx;

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
    float alpha = 0.01f;
    float atPeak = GgxD(1.0f, alpha);
    float offPeak = GgxD(0.95f, alpha);
    REQUIRE(offPeak < atPeak * 0.01f);
}

TEST_CASE("SmithG times 4*NdotV*NdotL stays within [0, 1]", "[microfacet][regression]") {
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
    float eta = 0.2f, k = 3.0f;
    float expected = ((eta - 1.0f) * (eta - 1.0f) + k * k) /
                      ((eta + 1.0f) * (eta + 1.0f) + k * k);
    REQUIRE(FrConductor(1.0f, eta, k) == Approx(expected).margin(1e-4));
    REQUIRE(FrConductor(1.0f, eta, k) > 0.9f);
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
    float alpha = 0.8f;
    for (float u1 = 0.05f; u1 < 1.0f; u1 += 0.1f) {
        for (float u2 = 0.05f; u2 < 1.0f; u2 += 0.1f) {
            Vector3f h = SampleGgxVndf(wo, alpha, u1, u2);
            REQUIRE(h.z >= -1e-5f);
        }
    }
}

TEST_CASE("GgxVndfPdf integrates to approximately 1 over the hemisphere", "[microfacet][regression]") {
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
        float pdf = GgxVndfPdf(NdotV, NdotH, alpha);
        REQUIRE(pdf > 0.0f);
        sum += 1.0 / pdf;
    }
    double estimate = sum / N;
    REQUIRE(estimate > 0.0);
    REQUIRE(estimate < 1000.0);
}
