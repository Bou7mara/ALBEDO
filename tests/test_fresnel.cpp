#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/materials/fresnel.h"
#include <cmath>
#include <numbers>

using namespace rt;
using Catch::Approx;

TEST_CASE("Reflect reverses straight-on incidence exactly along the normal", "[fresnel]") {
    Vector3f n(0, 1, 0);
    Vector3f wo(0, 1, 0);
    Vector3f wi = Reflect(wo, n);
    REQUIRE(wi.x == Approx(0.0f).margin(1e-5));
    REQUIRE(wi.y == Approx(1.0f).margin(1e-5));
    REQUIRE(wi.z == Approx(0.0f).margin(1e-5));
}

TEST_CASE("Reflect preserves angle of incidence", "[fresnel]") {
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(1, 1, 0));
    Vector3f wi = Reflect(wo, n);
    REQUIRE(Dot(wi, n) == Approx(Dot(wo, n)).margin(1e-5));
    REQUIRE(Length(wi) == Approx(1.0f).margin(1e-5));
}

TEST_CASE("Refract with eta == 1 never bends, for any incidence angle", "[fresnel][regression]") {
    Vector3f n(0, 1, 0);
    for (float angleDeg = 5.0f; angleDeg < 85.0f; angleDeg += 10.0f) {
        float rad = angleDeg * std::numbers::pi_v<float> / 180.0f;
        Vector3f wi = Normalize(Vector3f(std::sin(rad), std::cos(rad), 0.0f));
        Vector3f wt;
        REQUIRE(Refract(wi, n, 1.0f, &wt));
        REQUIRE(wt.x == Approx(-wi.x).margin(1e-4));
        REQUIRE(wt.y == Approx(-wi.y).margin(1e-4));
        REQUIRE(wt.z == Approx(-wi.z).margin(1e-4));
    }
}

TEST_CASE("Refract satisfies Snell's law for a genuine index mismatch", "[fresnel][regression]") {
    Vector3f n(0, 1, 0);
    float eta = 1.0f / 1.5f;
    float angleDeg = 30.0f;
    float rad = angleDeg * std::numbers::pi_v<float> / 180.0f;
    Vector3f wi = Normalize(Vector3f(std::sin(rad), std::cos(rad), 0.0f));

    Vector3f wt;
    REQUIRE(Refract(wi, n, eta, &wt));
    REQUIRE(Length(wt) == Approx(1.0f).margin(1e-4));

    float cosThetaI = Dot(wi, n);
    float sinThetaI = std::sqrt(std::max(0.0f, 1.0f - cosThetaI * cosThetaI));
    float cosThetaT = -Dot(wt, n);
    float sinThetaT = std::sqrt(std::max(0.0f, 1.0f - cosThetaT * cosThetaT));

    REQUIRE(sinThetaT == Approx(eta * sinThetaI).margin(1e-4));
}

TEST_CASE("Refract reports total internal reflection beyond the critical angle", "[fresnel][regression]") {
    Vector3f n(0, 1, 0);
    float rad = 60.0f * std::numbers::pi_v<float> / 180.0f;
    Vector3f wi = Normalize(Vector3f(std::sin(rad), std::cos(rad), 0.0f));
    Vector3f wt;
    REQUIRE_FALSE(Refract(wi, n, 1.5f, &wt));
}

TEST_CASE("FrDielectric at normal incidence matches the closed-form R0", "[fresnel]") {
    REQUIRE(FrDielectric(1.0f, 1.0f, 1.5f) == Approx(0.04f).margin(1e-4));
    REQUIRE(FrDielectric(1.0f, 1.5f, 1.0f) == Approx(0.04f).margin(1e-4));
}

TEST_CASE("FrDielectric saturates to exactly 1.0 beyond the critical angle", "[fresnel][regression]") {
    float rad = 60.0f * std::numbers::pi_v<float> / 180.0f;
    float cosThetaI = std::cos(rad);
    REQUIRE(FrDielectric(cosThetaI, 1.5f, 1.0f) == Approx(1.0f).margin(1e-5));
}

TEST_CASE("FrDielectric approaches total reflection at grazing incidence", "[fresnel]") {
    REQUIRE(FrDielectric(0.01f, 1.0f, 1.5f) > 0.9f);
}

TEST_CASE("FrDielectric always stays within [0, 1]", "[fresnel][regression]") {
    for (float cosThetaI = 0.05f; cosThetaI < 1.0f; cosThetaI += 0.05f) {
        float R = FrDielectric(cosThetaI, 1.0f, 1.5f);
        REQUIRE(R >= 0.0f);
        REQUIRE(R <= 1.0f);
    }
}
