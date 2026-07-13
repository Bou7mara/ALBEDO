#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/core/bounds3.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Union of two bounds produces the enclosing box", "[bounds3]") {
    Bounds3f a(Point3f(0, 0, 0), Point3f(1, 1, 1));
    Bounds3f b(Point3f(2, -1, 0.5f), Point3f(3, 2, 2));
    Bounds3f u = Union(a, b);
    REQUIRE(u.pMin == Point3f(0, -1, 0));
    REQUIRE(u.pMax == Point3f(3, 2, 2));
}

TEST_CASE("Union with a point that's already inside doesn't change bounds", "[bounds3]") {
    Bounds3f a(Point3f(0, 0, 0), Point3f(2, 2, 2));
    Bounds3f u = Union(a, Point3f(1, 1, 1));
    REQUIRE(u.pMin == a.pMin);
    REQUIRE(u.pMax == a.pMax);
}

TEST_CASE("Default-constructed bounds absorb the first union unchanged", "[bounds3][regression]") {
    Bounds3f empty;
    Bounds3f u = Union(empty, Point3f(5, -3, 1));
    REQUIRE(u.pMin == Point3f(5, -3, 1));
    REQUIRE(u.pMax == Point3f(5, -3, 1));
}

TEST_CASE("SurfaceArea matches a hand-computed box", "[bounds3]") {
    Bounds3f b(Point3f(0, 0, 0), Point3f(2, 3, 4));
    // 2*(2*3 + 3*4 + 4*2) = 2*(6+12+8) = 52
    REQUIRE(b.SurfaceArea() == Approx(52.0f));
}

TEST_CASE("MaxExtent picks the longest axis", "[bounds3]") {
    REQUIRE(Bounds3f(Point3f(0,0,0), Point3f(5,1,1)).MaxExtent() == 0);
    REQUIRE(Bounds3f(Point3f(0,0,0), Point3f(1,5,1)).MaxExtent() == 1);
    REQUIRE(Bounds3f(Point3f(0,0,0), Point3f(1,1,5)).MaxExtent() == 2);
}

TEST_CASE("IntersectP hits a box a ray passes through", "[bounds3]") {
    Bounds3f b(Point3f(-1,-1,-1), Point3f(1,1,1));
    Ray r(Point3f(0, 0, -5), Vector3f(0, 0, 1));
    REQUIRE(b.IntersectP(r));
}

TEST_CASE("IntersectP misses a box entirely off to the side", "[bounds3]") {
    Bounds3f b(Point3f(-1,-1,-1), Point3f(1,1,1));
    Ray r(Point3f(10, 10, -5), Vector3f(0, 0, 1));
    REQUIRE_FALSE(b.IntersectP(r));
}

TEST_CASE("Precomputed-invDir IntersectP agrees with the general-purpose overload", "[bounds3][regression]") {
    Bounds3f b(Point3f(-2,-1,-3), Point3f(2,1,3));
    Ray r(Point3f(-5, 0, 0), Vector3f(1, 0, 0));
    Vector3f invDir(1.0f/r.d.x, 1.0f/r.d.y, 1.0f/r.d.z);
    int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};
    REQUIRE(b.IntersectP(r) == b.IntersectP(r, invDir, dirIsNeg));
}
