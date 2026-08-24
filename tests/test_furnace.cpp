#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/energy_compensation.h"
#include "rt/core/rng.h"
#include "rt/core/sampling.h"
#include "rt/core/onb.h"

#include <cmath>
#include <vector>

using namespace rt;
using Catch::Approx;

TEST_CASE("Directional albedo LUT monotonic and clamped sanity", "[energy_compensation][lut]") {
    const auto& lut = GetDirectionalAlbedoLUT();

    for (float alpha = 0.05f; alpha <= 1.0f; alpha += 0.1f) {
        float eAvg = lut.SampleEAvg(alpha);
        REQUIRE(eAvg >= 0.0f);
        REQUIRE(eAvg <= 1.0f);

        for (float cosTheta = 0.05f; cosTheta <= 1.0f; cosTheta += 0.1f) {
            float e = lut.SampleE(cosTheta, alpha);
            REQUIRE(e >= 0.0f);
            REQUIRE(e <= 1.0f);
        }
    }
}

TEST_CASE("Conductor Microfacet white furnace energy conservation test", "[energy_compensation][furnace]") {

    Vector3f eta(0.001f, 0.001f, 0.001f);
    Vector3f k(100.0f, 100.0f, 100.0f);
    Vector3f tint(1.0f, 1.0f, 1.0f);

    float roughness = 1.0f;
    Microfacet mf = Microfacet::MakeConductorMicrofacet(roughness, eta, k, tint);

    Vector3f n(0.0f, 0.0f, 1.0f);
    ONB onb(n);

    std::vector<float> testCosThetas = { 1.0f, 0.8f, 0.5f, 0.2f, 0.1f };

    constexpr int kSamples = 16384;

    for (float cosThetaO : testCosThetas) {
        float sinThetaO = std::sqrt(std::max(0.0f, 1.0f - cosThetaO * cosThetaO));
        Vector3f wo = Normalize(Vector3f(sinThetaO, 0.0f, cosThetaO));

        RNG rng(42 + static_cast<uint64_t>(cosThetaO * 1000.0f));
        Vector3f totalReflected(0.0f, 0.0f, 0.0f);

        for (int i = 0; i < kSamples; ++i) {
            Point2f u = rng.Uniform2D();
            Vector3f localWi = CosineSampleHemisphere(u);
            Vector3f wi = onb.ToWorld(localWi);
            float pdf = CosineHemispherePdf(localWi.z);

            if (pdf > 0.0f) {
                Vector3f f = mf.f(wo, wi, n);
                float cosThetaI = AbsDot(wi, n);
                totalReflected += f * (cosThetaI / pdf);
            }
        }

        Vector3f albedo = totalReflected / static_cast<float>(kSamples);

        INFO("Viewing cosTheta = " << cosThetaO << ", Total Reflected Albedo = " << albedo.x);

        REQUIRE(albedo.x == Approx(1.0f).margin(0.03f));
        REQUIRE(albedo.y == Approx(1.0f).margin(0.03f));
        REQUIRE(albedo.z == Approx(1.0f).margin(0.03f));
    }
}
