#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/core/ray.h"
#include <cmath>

using namespace rt;
using Catch::Approx;

TEST_CASE("Ray default construction", "[ray]") {
    Ray r;
    REQUIRE(r.o == Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(r.d == Vector3f(0.0f, 0.0f, 0.0f));
    REQUIRE(r.time == Approx(0.0f));
    REQUIRE(std::isinf(r.tMax));
}

TEST_CASE("Ray construction with explicit origin and direction", "[ray]") {
    Point3f o(1.0f, 2.0f, 3.0f);
    Vector3f d(0.0f, 0.0f, 1.0f);
    Ray r(o, d);

    REQUIRE(r.o == o);
    REQUIRE(r.d == d);
}

TEST_CASE("Ray operator() evaluates point at parameter t", "[ray]") {
    Ray r(Point3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f));

    REQUIRE(r(0.0f) == Point3f(0.0f, 0.0f, 0.0f));
    REQUIRE(r(5.0f) == Point3f(5.0f, 0.0f, 0.0f));
}

TEST_CASE("Ray operator() with non-axis-aligned direction", "[ray]") {
    Ray r(Point3f(1.0f, 1.0f, 1.0f), Vector3f(1.0f, 2.0f, 3.0f));
    Point3f p = r(2.0f);

    REQUIRE(p.x == Approx(3.0f));   // 1 + 2*1
    REQUIRE(p.y == Approx(5.0f));   // 1 + 2*2
    REQUIRE(p.z == Approx(7.0f));   // 1 + 2*3
}

TEST_CASE("Ray tMax can be shrunk via mutable field", "[ray]") {
    const Ray r(Point3f(0,0,0), Vector3f(1,0,0));
    // Simulates what an intersection routine does: shrink tMax on a
    // const Ray& without needing a non-const reference.
    r.tMax = 4.0f;
    REQUIRE(r.tMax == Approx(4.0f));
}

TEST_CASE("Ray carries a time value for future motion blur support", "[ray]") {
    Ray r(Point3f(0,0,0), Vector3f(0,0,1), std::numeric_limits<float>::infinity(), 0.5f);
    REQUIRE(r.time == Approx(0.5f));
}
