#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/materials/lambertian.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Lambertian f() is direction-independent and equals albedo/pi", "[lambertian]") {
    Lambertian mat(Vector3f(0.8f, 0.4f, 0.2f));
    Vector3f val = mat.f(Vector3f(0, 0, 1), Vector3f(1, 0, 0));
    REQUIRE(val.x == Approx(0.8f * std::numbers::inv_pi_v<float>));
    REQUIRE(val.y == Approx(0.4f * std::numbers::inv_pi_v<float>));
    REQUIRE(val.z == Approx(0.2f * std::numbers::inv_pi_v<float>));
}

TEST_CASE("Lambertian Sample_f always stays on the same side as the normal", "[lambertian]") {
    Lambertian mat(Vector3f(0.5f, 0.5f, 0.5f));
    Vector3f n(0.0f, 1.0f, 0.0f);

    for (float ux = 0.05f; ux < 1.0f; ux += 0.13f) {
        for (float uy = 0.05f; uy < 1.0f; uy += 0.17f) {
            Vector3f wi;
            float pdf;
            mat.Sample_f(Vector3f(0, 1, 0), n, Point2f(ux, uy), &wi, &pdf);
            REQUIRE(Dot(wi, n) >= -1e-4f);
            REQUIRE(pdf > 0.0f);
            REQUIRE(Length(wi) == Approx(1.0f).margin(1e-4));
        }
    }
}

TEST_CASE("f * cosTheta / pdf collapses exactly to albedo (cosine-sampling cancellation)", "[lambertian][regression]") {
    // This is the identity the whole cosine-weighted-sampling choice
    // rests on. If this test ever fails, either Sample_f's pdf or its
    // cosTheta convention has drifted out of sync with f() -- treat it
    // as load-bearing, not incidental.
    Vector3f albedo(0.7f, 0.3f, 0.9f);
    Lambertian mat(albedo);
    Vector3f n(0.0f, 0.0f, 1.0f);
    Vector3f wo(0.0f, 0.0f, 1.0f);

    Vector3f wi;
    float pdf;
    Vector3f f = mat.Sample_f(wo, n, Point2f(0.37f, 0.61f), &wi, &pdf);
    float cosTheta = AbsDot(wi, n);
    Vector3f throughput = f * cosTheta / pdf;

    REQUIRE(throughput.x == Approx(albedo.x).margin(1e-4));
    REQUIRE(throughput.y == Approx(albedo.y).margin(1e-4));
    REQUIRE(throughput.z == Approx(albedo.z).margin(1e-4));
}
