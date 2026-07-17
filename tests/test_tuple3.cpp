#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/core/vector3.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Vector3 construction and element access", "[tuple3]") {
    Vector3f v(1.0f, 2.0f, 3.0f);
    REQUIRE(v.x == Approx(1.0f));
    REQUIRE(v.y == Approx(2.0f));
    REQUIRE(v.z == Approx(3.0f));
    REQUIRE(v[0] == Approx(1.0f));
    REQUIRE(v[1] == Approx(2.0f));
    REQUIRE(v[2] == Approx(3.0f));
}

TEST_CASE("Vector3 default construction is zero", "[tuple3]") {
    Vector3f v;
    REQUIRE(v.x == Approx(0.0f));
    REQUIRE(v.y == Approx(0.0f));
    REQUIRE(v.z == Approx(0.0f));
}

TEST_CASE("Vector3 arithmetic operators", "[tuple3]") {
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(4.0f, 5.0f, 6.0f);

    SECTION("addition") {
        Vector3f r = a + b;
        REQUIRE(r == Vector3f(5.0f, 7.0f, 9.0f));
    }

    SECTION("subtraction") {
        Vector3f r = a - b;
        REQUIRE(r == Vector3f(-3.0f, -3.0f, -3.0f));
    }

    SECTION("unary negation") {
        Vector3f r = -a;
        REQUIRE(r == Vector3f(-1.0f, -2.0f, -3.0f));
    }

    SECTION("scalar multiplication, both directions") {
        REQUIRE(a * 2.0f == Vector3f(2.0f, 4.0f, 6.0f));
        REQUIRE(2.0f * a == Vector3f(2.0f, 4.0f, 6.0f));
    }

    SECTION("scalar division") {
        Vector3f r = Vector3f(2.0f, 4.0f, 6.0f) / 2.0f;
        REQUIRE(r == Vector3f(1.0f, 2.0f, 3.0f));
    }

    SECTION("compound assignment") {
        Vector3f c = a;
        c += b;
        REQUIRE(c == Vector3f(5.0f, 7.0f, 9.0f));

        c = a;
        c -= b;
        REQUIRE(c == Vector3f(-3.0f, -3.0f, -3.0f));

        c = a;
        c *= 2.0f;
        REQUIRE(c == Vector3f(2.0f, 4.0f, 6.0f));

        c = Vector3f(2.0f, 4.0f, 6.0f);
        c /= 2.0f;
        REQUIRE(c == Vector3f(1.0f, 2.0f, 3.0f));
    }
}

TEST_CASE("Vector3i integer division does not truncate to zero", "[tuple3][regression]") {
    Vector3i v(4, 6, 8);
    Vector3i r = v / 2;
    REQUIRE(r == Vector3i(2, 3, 4));

    Vector3i v2(9, 10, 11);
    Vector3i r2 = v2 / 3;
    REQUIRE(r2 == Vector3i(3, 3, 3));
}

TEST_CASE("Vector3 equality and inequality", "[tuple3]") {
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(1.0f, 2.0f, 3.0f);
    Vector3f c(1.0f, 2.0f, 3.1f);

    REQUIRE(a == b);
    REQUIRE(a != c);
}

TEST_CASE("HasNaN detects NaN components", "[tuple3]") {
    Vector3f v(1.0f, 2.0f, 3.0f);
    REQUIRE_FALSE(v.HasNaN());
}
