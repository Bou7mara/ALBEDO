#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "rt/core/bounds3.h"
#include "rt/core/transform.h"

using namespace rt;
using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;

TEST_CASE("Bounds3f Transform - Identity", "[bounds3][transform]") {
    Bounds3f box(Point3f(-1.0f, -2.0f, -3.0f), Point3f(4.0f, 5.0f, 6.0f));
    Transform id = Transform::Identity();
    Bounds3f result = id(box);

    REQUIRE_THAT(result.minPt.x, WithinAbs(box.minPt.x, 1e-5f));
    REQUIRE_THAT(result.minPt.y, WithinAbs(box.minPt.y, 1e-5f));
    REQUIRE_THAT(result.minPt.z, WithinAbs(box.minPt.z, 1e-5f));
    REQUIRE_THAT(result.maxPt.x, WithinAbs(box.maxPt.x, 1e-5f));
    REQUIRE_THAT(result.maxPt.y, WithinAbs(box.maxPt.y, 1e-5f));
    REQUIRE_THAT(result.maxPt.z, WithinAbs(box.maxPt.z, 1e-5f));
}

TEST_CASE("Bounds3f Transform - Translation", "[bounds3][transform]") {
    Bounds3f box(Point3f(1.0f, 2.0f, 3.0f), Point3f(4.0f, 6.0f, 8.0f));
    Vector3f delta(5.0f, -3.0f, 2.0f);
    Transform t = Transform::Translate(delta);
    Bounds3f result = t(box);

    REQUIRE_THAT(result.minPt.x, WithinAbs(6.0f, 1e-5f));
    REQUIRE_THAT(result.minPt.y, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(result.minPt.z, WithinAbs(5.0f, 1e-5f));
    REQUIRE_THAT(result.maxPt.x, WithinAbs(9.0f, 1e-5f));
    REQUIRE_THAT(result.maxPt.y, WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(result.maxPt.z, WithinAbs(10.0f, 1e-5f));

    REQUIRE_THAT(result.Diagonal().x, WithinAbs(box.Diagonal().x, 1e-5f));
    REQUIRE_THAT(result.Diagonal().y, WithinAbs(box.Diagonal().y, 1e-5f));
    REQUIRE_THAT(result.Diagonal().z, WithinAbs(box.Diagonal().z, 1e-5f));
}

TEST_CASE("Bounds3f Transform - 90 deg Rotation", "[bounds3][transform]") {
    // Non-cubic box: X extent [0, 2], Y extent [0, 4], Z extent [0, 8]
    Bounds3f box(Point3f(0.0f, 0.0f, 0.0f), Point3f(2.0f, 4.0f, 8.0f));
    Transform rotY = Transform::RotateY(90.0f);
    Bounds3f result = rotY(box);

    // RotateY(90): x' = z, z' = -x
    // Box X goes to [0, 8], Z goes to [-2, 0]
    REQUIRE_THAT(result.minPt.x, WithinAbs(0.0f, 1e-4f));
    REQUIRE_THAT(result.maxPt.x, WithinAbs(8.0f, 1e-4f));
    REQUIRE_THAT(result.minPt.y, WithinAbs(0.0f, 1e-4f));
    REQUIRE_THAT(result.maxPt.y, WithinAbs(4.0f, 1e-4f));
    REQUIRE_THAT(result.minPt.z, WithinAbs(-2.0f, 1e-4f));
    REQUIRE_THAT(result.maxPt.z, WithinAbs(0.0f, 1e-4f));
}

TEST_CASE("Bounds3f Transform - Scale and 45 deg Rotation", "[bounds3][transform]") {
    Bounds3f box(Point3f(-1.0f, -2.0f, -3.0f), Point3f(1.0f, 2.0f, 3.0f));
    Transform s = Transform::Scale(2.0f, 1.0f, 0.5f);
    Transform r = Transform::RotateY(45.0f);
    Transform comp = r * s;

    Bounds3f result = comp(box);

    // Scaled bounds: X in [-2, 2], Y in [-2, 2], Z in [-1.5, 1.5]
    // 45 deg Y rot max x' = 3.5 * cos(45 deg) = 2.4748737f
    float expectedExt = 3.5f * std::cos(45.0f * 3.1415926535f / 180.0f);

    REQUIRE_THAT(result.minPt.x, WithinAbs(-expectedExt, 1e-4f));
    REQUIRE_THAT(result.maxPt.x, WithinAbs(expectedExt, 1e-4f));
    REQUIRE_THAT(result.minPt.y, WithinAbs(-2.0f, 1e-4f));
    REQUIRE_THAT(result.maxPt.y, WithinAbs(2.0f, 1e-4f));
    REQUIRE_THAT(result.minPt.z, WithinAbs(-expectedExt, 1e-4f));
    REQUIRE_THAT(result.maxPt.z, WithinAbs(expectedExt, 1e-4f));
}
