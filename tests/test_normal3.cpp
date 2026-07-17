#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <type_traits>
#include "rt/core/normal3.h"
#include "rt/core/vector3.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Normal3 explicit construction from Vector3", "[normal3]") {
    Vector3f v(1.0f, 2.0f, 3.0f);
    Normal3f n(v);
    REQUIRE(n.x == Approx(1.0f));
    REQUIRE(n.y == Approx(2.0f));
    REQUIRE(n.z == Approx(3.0f));
}

TEST_CASE("Normal3 explicit conversion back to Vector3", "[normal3]") {
    Normal3f n(1.0f, 2.0f, 3.0f);
    Vector3f v(n);
    REQUIRE(v == Vector3f(1.0f, 2.0f, 3.0f));
}

TEST_CASE("Dot between Normal3 and Vector3, both orders", "[normal3]") {
    Normal3f n(0.0f, 1.0f, 0.0f);
    Vector3f v(0.0f, 5.0f, 0.0f);
    REQUIRE(Dot(n, v) == Approx(5.0f));
    REQUIRE(Dot(v, n) == Approx(5.0f));
}

TEST_CASE("FaceForward flips normal away from reference direction", "[normal3]") {
    Normal3f n(0.0f, 1.0f, 0.0f);
    Vector3f towardSameSide(0.0f, 1.0f, 0.0f);
    Vector3f towardOppositeSide(0.0f, -1.0f, 0.0f);

    REQUIRE(FaceForward(n, towardSameSide) == n);
    REQUIRE(FaceForward(n, towardOppositeSide) == -n);
}

static_assert(!std::is_convertible_v<Vector3f, Normal3f>,
              "Vector3 -> Normal3 conversion must be explicit, not implicit");
static_assert(!std::is_convertible_v<Normal3f, Vector3f>,
              "Normal3 -> Vector3 conversion must be explicit, not implicit");
