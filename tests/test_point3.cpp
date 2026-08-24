#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <type_traits>
#include "rt/core/point3.h"
#include "rt/core/vector3.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Point3 - Point3 yields Vector3", "[point3]") {
    Point3f p1(5.0f, 5.0f, 5.0f);
    Point3f p2(1.0f, 2.0f, 3.0f);
    Vector3f v = p1 - p2;
    REQUIRE(v == Vector3f(4.0f, 3.0f, 2.0f));
}

TEST_CASE("Point3 + Vector3 yields Point3, both directions", "[point3]") {
    Point3f p(1.0f, 1.0f, 1.0f);
    Vector3f v(1.0f, 2.0f, 3.0f);

    REQUIRE(p + v == Point3f(2.0f, 3.0f, 4.0f));
    REQUIRE(v + p == Point3f(2.0f, 3.0f, 4.0f));
}

TEST_CASE("Point3 - Vector3 yields Point3", "[point3]") {
    Point3f p(5.0f, 5.0f, 5.0f);
    Vector3f v(1.0f, 2.0f, 3.0f);
    REQUIRE(p - v == Point3f(4.0f, 3.0f, 2.0f));
}

TEST_CASE("Lerp interpolates correctly at t=0, t=1, t=0.5", "[point3]") {
    Point3f p0(0.0f, 0.0f, 0.0f);
    Point3f p1(10.0f, 20.0f, 30.0f);

    REQUIRE(Lerp(0.0f, p0, p1) == p0);
    REQUIRE(Lerp(1.0f, p0, p1) == p1);

    Point3f mid = Lerp(0.5f, p0, p1);
    REQUIRE(mid.x == Approx(5.0f));
    REQUIRE(mid.y == Approx(10.0f));
    REQUIRE(mid.z == Approx(15.0f));
}

TEST_CASE("Distance and DistanceSquared", "[point3]") {
    Point3f p1(0.0f, 0.0f, 0.0f);
    Point3f p2(3.0f, 4.0f, 0.0f);
    REQUIRE(DistanceSquared(p1, p2) == Approx(25.0f));
    REQUIRE(Distance(p1, p2) == Approx(5.0f));
}

TEST_CASE("Min and Max produce componentwise results", "[point3]") {
    Point3f p1(1.0f, 5.0f, 3.0f);
    Point3f p2(4.0f, 2.0f, 6.0f);
    REQUIRE(Min(p1, p2) == Point3f(1.0f, 2.0f, 3.0f));
    REQUIRE(Max(p1, p2) == Point3f(4.0f, 5.0f, 6.0f));
}

static_assert(!std::is_invocable_v<std::plus<>, Point3f, Point3f>,
              "Point3 + Point3 should not be a valid operation (non-affine)");
