#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/materials/emissive.h"
#include "rt/materials/lambertian.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Emissive f() is always zero, it does not scatter light", "[emissive]") {
    Emissive light(Vector3f(5.0f, 5.0f, 5.0f));
    Vector3f result = light.f(Vector3f(0, 1, 0), Vector3f(0, -1, 0), Vector3f(0, 1, 0));
    REQUIRE(result == Vector3f(0.0f, 0.0f, 0.0f));
}

TEST_CASE("Emissive Sample_f reports pdf <= 0, terminating the path", "[emissive]") {
    Emissive light(Vector3f(5.0f, 5.0f, 5.0f));
    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo(0.0f, 1.0f, 0.0f);
    Vector3f wi;
    float pdf = 1.0f;
    Vector3f f = light.Sample_f(wo, n, Point2f(0.5f, 0.5f), &wi, &pdf);

    REQUIRE(pdf <= 0.0f);
    REQUIRE(f == Vector3f(0.0f, 0.0f, 0.0f));
}

TEST_CASE("Emissive Le returns the stored radiance on the normal's side", "[emissive]") {
    Vector3f radiance(3.0f, 2.0f, 1.0f);
    Emissive light(radiance);
    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo(0.0f, 1.0f, 0.0f); // same side as n

    Vector3f le = light.Le(wo, n);
    REQUIRE(le.x == Approx(radiance.x));
    REQUIRE(le.y == Approx(radiance.y));
    REQUIRE(le.z == Approx(radiance.z));
}

TEST_CASE("Emissive Le is black on the back side of the surface", "[emissive]") {
    Emissive light(Vector3f(3.0f, 2.0f, 1.0f));
    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo(0.0f, -1.0f, 0.0f); // opposite side of n

    Vector3f le = light.Le(wo, n);
    REQUIRE(le == Vector3f(0.0f, 0.0f, 0.0f));
}

TEST_CASE("Emissive Le is black exactly at grazing incidence", "[emissive][regression]") {
    Emissive light(Vector3f(3.0f, 2.0f, 1.0f));
    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo(1.0f, 0.0f, 0.0f); // perpendicular to n, Dot == 0

    Vector3f le = light.Le(wo, n);
    REQUIRE(le == Vector3f(0.0f, 0.0f, 0.0f));
}

TEST_CASE("Non-emissive materials still default to black Le, unaffected by this change",
          "[emissive][regression]") {
    Lambertian diffuse(Vector3f(0.8f, 0.8f, 0.8f));
    Vector3f le = diffuse.Le(Vector3f(0, 1, 0), Vector3f(0, 1, 0));
    REQUIRE(le == Vector3f(0.0f, 0.0f, 0.0f));
}
