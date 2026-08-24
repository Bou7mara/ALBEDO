#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/core/point2.h"
#include <type_traits>

using namespace rt;
using Catch::Approx;

TEST_CASE("Point2 construction and explicit Vector2 conversion", "[point2]") {
    Point2f p(1.5f, 2.5f);
    REQUIRE(p.x == Approx(1.5f));
    REQUIRE(p.y == Approx(2.5f));

    Vector2f v(4.0f, 5.0f);
    Point2f pFromV(v);
    REQUIRE(pFromV.x == Approx(4.0f));
    REQUIRE(pFromV.y == Approx(5.0f));
}

TEST_CASE("Point2-Vector2 operations (affine spaces)", "[point2]") {
    Point2f p(1.0f, 2.0f);
    Vector2f v(3.0f, 4.0f);

    SECTION("Point + Vector -> Point") {
        Point2f r = p + v;
        REQUIRE(r == Point2f(4.0f, 6.0f));

        Point2f rCommutative = v + p;
        REQUIRE(rCommutative == Point2f(4.0f, 6.0f));
    }

    SECTION("Point - Vector -> Point") {
        Point2f r = p - v;
        REQUIRE(r == Point2f(-2.0f, -2.0f));
    }

    SECTION("Point - Point -> Vector") {
        Point2f p2(5.0f, 7.0f);
        Vector2f r = p2 - p;
        REQUIRE(r == Vector2f(4.0f, 5.0f));
    }
}

TEST_CASE("Point2 geometric free functions", "[point2]") {
    Point2f p1(1.0f, 2.0f);
    Point2f p2(4.0f, 6.0f);

    SECTION("DistanceSquared") {
        REQUIRE(DistanceSquared(p1, p2) == Approx(25.0f));
    }

    SECTION("Distance") {
        REQUIRE(Distance(p1, p2) == Approx(5.0f));
    }

    SECTION("Lerp") {
        Point2f r = Lerp(0.5f, p1, p2);
        REQUIRE(r.x == Approx(2.5f));
        REQUIRE(r.y == Approx(4.0f));
    }

    SECTION("Min / Max") {
        Point2f a(1.0f, 5.0f);
        Point2f b(2.0f, 4.0f);
        REQUIRE(Min(a, b) == Point2f(1.0f, 4.0f));
        REQUIRE(Max(a, b) == Point2f(2.0f, 5.0f));
    }

    SECTION("Floor / Ceil") {
        Point2f p(1.7f, 2.3f);
        REQUIRE(Floor(p) == Point2f(1.0f, 2.0f));
        REQUIRE(Ceil(p) == Point2f(2.0f, 3.0f));
    }
}

template <typename T, typename = void>
struct HasAddition : std::false_type {};

template <typename T>
struct HasAddition<T, std::void_t<decltype(std::declval<T>() + std::declval<T>())>> : std::true_type {};

static_assert(!HasAddition<Point2f>::value, "Point2 + Point2 addition must be disabled at compile-time");
