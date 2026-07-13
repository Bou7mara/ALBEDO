#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/materials/metal.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Metal f() is always zero, matching a delta distribution", "[metal]") {
    Metal m(Vector3f(0.8f, 0.8f, 0.8f));
    Vector3f result = m.f(Vector3f(0, 0, 1), Vector3f(1, 0, 0), Vector3f(0, 1, 0));
    REQUIRE(result == Vector3f(0.0f, 0.0f, 0.0f));
}

TEST_CASE("Metal Sample_f always reports pdf = 1", "[metal]") {
    Metal m(Vector3f(0.8f, 0.8f, 0.8f));
    Vector3f wo = Normalize(Vector3f(0.3f, 0.7f, 0.5f));
    Vector3f n(0, 1, 0);
    Vector3f wi;
    float pdf;
    m.Sample_f(wo, n, Point2f(0.5f, 0.5f), &wi, &pdf);
    REQUIRE(pdf == Approx(1.0f));
}

TEST_CASE("Straight-on incidence reflects straight back along the normal", "[metal]") {
    Metal m(Vector3f(1, 1, 1));
    Vector3f wo(0, 1, 0);
    Vector3f n(0, 1, 0);
    Vector3f wi;
    float pdf;
    m.Sample_f(wo, n, Point2f(0.5f, 0.5f), &wi, &pdf);
    REQUIRE(wi.x == Approx(0.0f).margin(1e-5));
    REQUIRE(wi.y == Approx(1.0f).margin(1e-5));
    REQUIRE(wi.z == Approx(0.0f).margin(1e-5));
}

TEST_CASE("Angle of incidence equals angle of reflection", "[metal][regression]") {
    Metal m(Vector3f(1, 1, 1));
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(1, 1, 0));   // 45 degrees off the normal
    Vector3f wi;
    float pdf;
    m.Sample_f(wo, n, Point2f(0.5f, 0.5f), &wi, &pdf);
    REQUIRE(Dot(wi, n) == Approx(Dot(wo, n)).margin(1e-5));
}

TEST_CASE("Sample_f direction stays on the same side of the surface as wo", "[metal]") {
    Metal m(Vector3f(1, 1, 1));
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(0.6f, 0.4f, 0.2f));
    Vector3f wi;
    float pdf;
    m.Sample_f(wo, n, Point2f(0.5f, 0.5f), &wi, &pdf);
    REQUIRE(Dot(wi, n) >= 0.0f);
}

TEST_CASE("f * cosTheta / pdf collapses exactly to albedo (specular cancellation)", "[metal][regression]") {
    // Mirrors the equivalent Lambertian test -- same cancellation
    // property, different mechanism (pdf=1 here vs. cosine-weighted
    // pdf there).
    Vector3f albedo(0.9f, 0.6f, 0.3f);
    Metal m(albedo);
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(0.5f, 0.8f, 0.3f));
    Vector3f wi;
    float pdf;
    Vector3f f = m.Sample_f(wo, n, Point2f(0.5f, 0.5f), &wi, &pdf);
    float cosTheta = AbsDot(wi, n);
    Vector3f result = f * cosTheta / pdf;

    REQUIRE(result.x == Approx(albedo.x).margin(1e-4));
    REQUIRE(result.y == Approx(albedo.y).margin(1e-4));
    REQUIRE(result.z == Approx(albedo.z).margin(1e-4));
}
