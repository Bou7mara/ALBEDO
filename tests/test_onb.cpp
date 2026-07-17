#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/core/onb.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("ONB axes are mutually orthogonal and unit length", "[onb]") {
    ONB onb(Normalize(Vector3f(0.3f, 0.7f, 0.2f)));
    REQUIRE(Length(onb.u) == Approx(1.0f).margin(1e-4));
    REQUIRE(Length(onb.v) == Approx(1.0f).margin(1e-4));
    REQUIRE(Length(onb.w) == Approx(1.0f).margin(1e-4));
    REQUIRE(Dot(onb.u, onb.v) == Approx(0.0f).margin(1e-4));
    REQUIRE(Dot(onb.u, onb.w) == Approx(0.0f).margin(1e-4));
    REQUIRE(Dot(onb.v, onb.w) == Approx(0.0f).margin(1e-4));
}

TEST_CASE("ONB w axis matches the input normal exactly", "[onb]") {
    Vector3f n = Normalize(Vector3f(1.0f, 2.0f, 3.0f));
    ONB onb(n);
    REQUIRE(onb.w.x == Approx(n.x));
    REQUIRE(onb.w.y == Approx(n.y));
    REQUIRE(onb.w.z == Approx(n.z));
}

TEST_CASE("ONB is well-defined for a normal pointing straight down -Z", "[onb][regression]") {
    ONB onb(Vector3f(0.0f, 0.0f, -1.0f));
    REQUIRE(Length(onb.u) == Approx(1.0f).margin(1e-4));
    REQUIRE(Length(onb.v) == Approx(1.0f).margin(1e-4));
    REQUIRE(Dot(onb.u, onb.v) == Approx(0.0f).margin(1e-4));
}
