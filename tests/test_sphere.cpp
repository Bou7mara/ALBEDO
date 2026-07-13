#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/shapes/sphere.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Ray hits a sphere at the origin head-on", "[sphere]") {
    Sphere s(Transform::Identity(), 1.0f);
    Ray r(Point3f(0, 0, -5), Vector3f(0, 0, 1));

    SurfaceInteraction isect;
    REQUIRE(s.Intersect(r, &isect));
    REQUIRE(isect.t == Approx(4.0f));
    REQUIRE(isect.p.z == Approx(-1.0f).margin(1e-5));
}

TEST_CASE("Ray misses a sphere entirely", "[sphere]") {
    Sphere s(Transform::Identity(), 1.0f);
    Ray r(Point3f(5, 5, -5), Vector3f(0, 0, 1));

    SurfaceInteraction isect;
    REQUIRE_FALSE(s.Intersect(r, &isect));
}

TEST_CASE("Ray originating inside the sphere hits the far side", "[sphere][regression]") {
    // Regression case: t0 < 0 (behind the origin) must fall through to
    // t1, not be rejected outright.
    Sphere s(Transform::Identity(), 1.0f);
    Ray r(Point3f(0, 0, 0), Vector3f(0, 0, 1));

    SurfaceInteraction isect;
    REQUIRE(s.Intersect(r, &isect));
    REQUIRE(isect.t == Approx(1.0f));
}

TEST_CASE("Tangent ray grazes the sphere at exactly one point", "[sphere]") {
    Sphere s(Transform::Identity(), 1.0f);
    Ray r(Point3f(1.0f, 0.0f, -5.0f), Vector3f(0, 0, 1));   // grazes at x=1

    SurfaceInteraction isect;
    REQUIRE(s.Intersect(r, &isect));
    REQUIRE(isect.p.x == Approx(1.0f).margin(1e-4));
}

TEST_CASE("Hit normal points radially outward and is unit length", "[sphere]") {
    Sphere s(Transform::Identity(), 1.0f);
    Ray r(Point3f(0, 0, -5), Vector3f(0, 0, 1));

    SurfaceInteraction isect;
    REQUIRE(s.Intersect(r, &isect));
    REQUIRE(Length(Vector3f(isect.n)) == Approx(1.0f).margin(1e-5));
    // At (0,0,-1), outward normal should point along -Z
    REQUIRE(isect.n.z == Approx(-1.0f).margin(1e-5));
}

TEST_CASE("Translated sphere hits at the correct world-space location", "[sphere]") {
    Transform t = Transform::Translate(Vector3f(5.0f, 0.0f, 0.0f));
    Sphere s(t, 1.0f);
    Ray r(Point3f(5.0f, 0.0f, -5.0f), Vector3f(0, 0, 1));

    SurfaceInteraction isect;
    REQUIRE(s.Intersect(r, &isect));
    REQUIRE(isect.p.x == Approx(5.0f).margin(1e-5));
    REQUIRE(isect.p.z == Approx(-1.0f).margin(1e-5));
}

TEST_CASE("Non-uniformly scaled sphere (ellipsoid) still produces a unit normal", "[sphere][regression]") {
    // This is the sphere-level equivalent of the Transform regression
    // test: confirms the inverse-transpose normal rule survives being
    // applied through a real Shape, not just directly on a Normal3.
    Transform t = Transform::Scale(1.0f, 1.0f, 2.0f);
    Sphere s(t, 1.0f);
    Ray r(Point3f(0.0f, 0.0f, -5.0f), Vector3f(0.0f, 0.0f, 1.0f));

    SurfaceInteraction isect;
    REQUIRE(s.Intersect(r, &isect));
    REQUIRE(Length(Vector3f(isect.n)) == Approx(1.0f).margin(1e-4));
}

TEST_CASE("IntersectP returns true/false consistently with Intersect", "[sphere]") {
    Sphere s(Transform::Identity(), 1.0f);
    Ray hit(Point3f(0, 0, -5), Vector3f(0, 0, 1));
    Ray miss(Point3f(5, 5, -5), Vector3f(0, 0, 1));

    REQUIRE(s.IntersectP(hit));
    REQUIRE_FALSE(s.IntersectP(miss));
}

TEST_CASE("Translated sphere's WorldBound is centered at its world position", "[sphere]") {
    Sphere s(Transform::Translate(Vector3f(5, 0, 0)), 1.0f);
    Bounds3f b = s.WorldBound();
    REQUIRE(b.pMin == Point3f(4, -1, -1));
    REQUIRE(b.pMax == Point3f(6, 1, 1));
}
