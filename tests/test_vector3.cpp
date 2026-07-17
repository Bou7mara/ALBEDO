#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/core/vector3.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Dot product", "[vector3]") {
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(4.0f, 5.0f, 6.0f);
    REQUIRE(Dot(a, b) == Approx(32.0f));
}

TEST_CASE("Dot product of orthogonal unit vectors is zero", "[vector3]") {
    REQUIRE(Dot(Vector3f(1, 0, 0), Vector3f(0, 1, 0)) == Approx(0.0f));
}

TEST_CASE("Cross product", "[vector3]") {
    Vector3f x(1.0f, 0.0f, 0.0f);
    Vector3f y(0.0f, 1.0f, 0.0f);
    Vector3f z = Cross(x, y);
    REQUIRE(z == Vector3f(0.0f, 0.0f, 1.0f));

    REQUIRE(Cross(y, x) == -z);
}

TEST_CASE("Length and LengthSquared", "[vector3]") {
    Vector3f v(3.0f, 4.0f, 0.0f);
    REQUIRE(LengthSquared(v) == Approx(25.0f));
    REQUIRE(Length(v) == Approx(5.0f));
}

TEST_CASE("Normalize produces a unit vector", "[vector3]") {
    Vector3f v(3.0f, 4.0f, 0.0f);
    Vector3f n = Normalize(v);
    REQUIRE(Length(n) == Approx(1.0f).margin(1e-6));
}

TEST_CASE("MinComponent and MaxComponent", "[vector3]") {
    Vector3f v(3.0f, -1.0f, 5.0f);
    REQUIRE(MinComponent(v) == Approx(-1.0f));
    REQUIRE(MaxComponent(v) == Approx(5.0f));
}

TEST_CASE("MaxDimension picks correct axis", "[vector3]") {
    REQUIRE(MaxDimension(Vector3f(5.0f, 1.0f, 2.0f)) == 0);
    REQUIRE(MaxDimension(Vector3f(1.0f, 5.0f, 2.0f)) == 1);
    REQUIRE(MaxDimension(Vector3f(1.0f, 2.0f, 5.0f)) == 2);
}

TEST_CASE("Permute reorders components", "[vector3]") {
    Vector3f v(10.0f, 20.0f, 30.0f);
    Vector3f p = Permute(v, 2, 0, 1);
    REQUIRE(p == Vector3f(30.0f, 10.0f, 20.0f));
}

TEST_CASE("Componentwise Vector3 multiplication", "[vector3]") {
    Vector3f a(2.0f, 3.0f, 4.0f);
    Vector3f b(0.5f, 2.0f, 0.25f);
    Vector3f c = a * b;
    REQUIRE(c.x == Catch::Approx(1.0f));
    REQUIRE(c.y == Catch::Approx(6.0f));
    REQUIRE(c.z == Catch::Approx(1.0f));
}
