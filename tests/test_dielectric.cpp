#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/materials/dielectric.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Dielectric f() is always zero, matching a delta distribution", "[dielectric]") {
    Dielectric d(1.5f);
    Vector3f result = d.f(Vector3f(0, 1, 0), Vector3f(0, -1, 0), Vector3f(0, 1, 0));
    REQUIRE(result == Vector3f(0.0f, 0.0f, 0.0f));
}

TEST_CASE("Sample_f with u.x near 0 takes the reflection branch at normal incidence", "[dielectric]") {
    Dielectric d(1.5f);
    Vector3f n(0, 1, 0);
    Vector3f wo(0, 1, 0);
    Vector3f wi;
    float pdf;
    Vector3f f = d.Sample_f(wo, n, Point2f(0.0f, 0.5f), &wi, &pdf);

    REQUIRE(wi.y == Approx(1.0f).margin(1e-4));
    REQUIRE(pdf == Approx(0.04f).margin(1e-3));

    float cosTheta = AbsDot(wi, n);
    Vector3f collapsed = f * cosTheta / pdf;
    REQUIRE(collapsed.x == Approx(1.0f).margin(1e-3));
    REQUIRE(collapsed.y == Approx(1.0f).margin(1e-3));
    REQUIRE(collapsed.z == Approx(1.0f).margin(1e-3));
}

TEST_CASE("Sample_f with u.x near 1 takes the transmission branch and applies the eta^2 radiance term", "[dielectric][regression]") {
    Dielectric d(1.5f);
    Vector3f n(0, 1, 0);
    Vector3f wo(0, 1, 0);
    Vector3f wi;
    float pdf;
    Vector3f f = d.Sample_f(wo, n, Point2f(0.99f, 0.5f), &wi, &pdf);

    REQUIRE(wi.y == Approx(-1.0f).margin(1e-3));
    REQUIRE(pdf == Approx(0.96f).margin(1e-3));

    float cosTheta = AbsDot(wi, n);
    Vector3f collapsed = f * cosTheta / pdf;
    float expected = 1.0f / ((1.0f / 1.5f) * (1.0f / 1.5f));
    REQUIRE(collapsed.x == Approx(expected).margin(1e-2));
    REQUIRE(collapsed.y == Approx(expected).margin(1e-2));
    REQUIRE(collapsed.z == Approx(expected).margin(1e-2));
}

TEST_CASE("Total internal reflection forces the reflect branch regardless of u.x", "[dielectric][regression]") {
    Dielectric d(1.5f);
    Vector3f n(0, 1, 0);
    float rad = 60.0f * std::numbers::pi_v<float> / 180.0f;
    Vector3f wo = Normalize(Vector3f(std::sin(rad), -std::cos(rad), 0.0f));  // inside, exiting
    Vector3f wi;
    float pdf;
    d.Sample_f(wo, n, Point2f(0.99f, 0.5f), &wi, &pdf);

    REQUIRE(pdf == Approx(1.0f).margin(1e-4));
    REQUIRE(Dot(wi, n) == Approx(Dot(wo, n)).margin(1e-4));
}

TEST_CASE("Sample_f direction is always unit length", "[dielectric]") {
    Dielectric d(1.5f);
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(0.3f, 0.8f, 0.2f));
    Vector3f wi;
    float pdf;
    for (float ux : {0.0f, 0.5f, 0.99f}) {
        d.Sample_f(wo, n, Point2f(ux, 0.5f), &wi, &pdf);
        REQUIRE(Length(wi) == Approx(1.0f).margin(1e-4));
    }
}