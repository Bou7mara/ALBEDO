#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/core/vector2.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Vector2 construction and element access", "[vector2]") {
    Vector2f v(3.0f, 4.0f);
    REQUIRE(v.x == Approx(3.0f));
    REQUIRE(v.y == Approx(4.0f));
    REQUIRE(v[0] == Approx(3.0f));
    REQUIRE(v[1] == Approx(4.0f));
}

TEST_CASE("Vector2 default construction is zero", "[vector2]") {
    Vector2f v;
    REQUIRE(v.x == Approx(0.0f));
    REQUIRE(v.y == Approx(0.0f));
}

TEST_CASE("Vector2 geometric free functions", "[vector2]") {
    Vector2f v(3.0f, 4.0f);

    SECTION("LengthSquared") {
        REQUIRE(LengthSquared(v) == Approx(25.0f));
    }

    SECTION("Length") {
        REQUIRE(Length(v) == Approx(5.0f));
    }

    SECTION("Normalize") {
        Vector2f n = Normalize(v);
        REQUIRE(n.x == Approx(0.6f));
        REQUIRE(n.y == Approx(0.8f));
        REQUIRE(Length(n) == Approx(1.0f));
    }

    SECTION("Dot") {
        Vector2f other(2.0f, -1.0f);
        REQUIRE(Dot(v, other) == Approx(2.0f));
    }

    SECTION("AbsDot") {
        Vector2f other(-2.0f, 1.0f);
        REQUIRE(AbsDot(v, other) == Approx(2.0f));
    }

    SECTION("Abs") {
        Vector2f neg(-1.5f, -2.5f);
        Vector2f a = Abs(neg);
        REQUIRE(a.x == Approx(1.5f));
        REQUIRE(a.y == Approx(2.5f));
    }
}

TEST_CASE("Vector2 arithmetic operators", "[vector2]") {
    Vector2f a(1.0f, 2.0f);
    Vector2f b(3.0f, 4.0f);

    SECTION("addition") {
        REQUIRE(a + b == Vector2f(4.0f, 6.0f));
    }

    SECTION("subtraction") {
        REQUIRE(a - b == Vector2f(-2.0f, -2.0f));
    }

    SECTION("scalar multiplication") {
        REQUIRE(a * 3.0f == Vector2f(3.0f, 6.0f));
        REQUIRE(3.0f * a == Vector2f(3.0f, 6.0f));
    }

    SECTION("scalar division") {
        REQUIRE(b / 2.0f == Vector2f(1.5f, 2.0f));
    }
}
