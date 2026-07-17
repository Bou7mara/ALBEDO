
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/core/transform.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Identity transform leaves points and vectors unchanged", "[transform]") {
    Transform t = Transform::Identity();
    Point3f p(1.0f, 2.0f, 3.0f);
    Vector3f v(4.0f, 5.0f, 6.0f);

    REQUIRE(t(p) == p);
    REQUIRE(t(v) == v);
}

TEST_CASE("Translate affects points but not vectors", "[transform]") {
    Transform t = Transform::Translate(Vector3f(10.0f, 0.0f, 0.0f));
    Point3f p(1.0f, 1.0f, 1.0f);
    Vector3f v(1.0f, 1.0f, 1.0f);

    REQUIRE(t(p) == Point3f(11.0f, 1.0f, 1.0f));
    REQUIRE(t(v) == v);
}

TEST_CASE("Scale affects points and vectors identically", "[transform]") {
    Transform t = Transform::Scale(2.0f, 3.0f, 4.0f);
    Point3f p(1.0f, 1.0f, 1.0f);
    Vector3f v(1.0f, 1.0f, 1.0f);

    REQUIRE(t(p) == Point3f(2.0f, 3.0f, 4.0f));
    REQUIRE(t(v) == Vector3f(2.0f, 3.0f, 4.0f));
}

TEST_CASE("Non-uniform scale transforms normals differently than vectors", "[transform][regression]") {
    Transform t = Transform::Scale(1.0f, 1.0f, 2.0f);

    Normal3f n(0.0f, 0.0f, 1.0f);
    Normal3f transformed = t(n);

    REQUIRE(transformed.z == Approx(0.5f));
}

TEST_CASE("Inverse transform undoes the original", "[transform]") {
    Transform t = Transform::Translate(Vector3f(5.0f, -3.0f, 2.0f));
    Transform tInv = t.Inverse();
    Point3f p(1.0f, 1.0f, 1.0f);

    Point3f roundTrip = tInv(t(p));
    REQUIRE(roundTrip.x == Approx(p.x).margin(1e-5));
    REQUIRE(roundTrip.y == Approx(p.y).margin(1e-5));
    REQUIRE(roundTrip.z == Approx(p.z).margin(1e-5));
}

TEST_CASE("Composition applies right-to-left, matching function composition", "[transform]") {
    Transform translate = Transform::Translate(Vector3f(1.0f, 0.0f, 0.0f));
    Transform scale = Transform::Scale(2.0f, 2.0f, 2.0f);
    Transform composed = translate * scale;

    Point3f p(1.0f, 1.0f, 1.0f);
    Point3f expected = translate(scale(p));

    REQUIRE(composed(p) == expected);
}

TEST_CASE("SwapsHandedness detects negative-determinant transforms", "[transform]") {
    Transform normalScale = Transform::Scale(1.0f, 1.0f, 1.0f);
    Transform mirrorX = Transform::Scale(-1.0f, 1.0f, 1.0f);

    REQUIRE_FALSE(normalScale.SwapsHandedness());
    REQUIRE(mirrorX.SwapsHandedness());
}

TEST_CASE("Ray transform moves origin as a point and direction as a vector", "[transform]") {
    Transform t = Transform::Translate(Vector3f(10.0f, 0.0f, 0.0f));
    Ray r(Point3f(0,0,0), Vector3f(0,0,1));
    Ray transformed = t(r);

    REQUIRE(transformed.o == Point3f(10.0f, 0.0f, 0.0f));
    REQUIRE(transformed.d == Vector3f(0.0f, 0.0f, 1.0f));
}

TEST_CASE("RotateZ by 90 degrees maps +X to +Y", "[transform]") {
    Transform t = Transform::RotateZ(90.0f);
    Vector3f v = t(Vector3f(1.0f, 0.0f, 0.0f));
    REQUIRE(v.x == Approx(0.0f).margin(1e-5));
    REQUIRE(v.y == Approx(1.0f).margin(1e-5));
}

TEST_CASE("Rotation inverse equals transpose (orthogonality)", "[transform]") {
    Transform t = Transform::RotateX(37.0f);
    Transform inv = t.Inverse();
    Vector3f v(1.0f, 2.0f, 3.0f);
    Vector3f roundTrip = inv(t(v));
    REQUIRE(roundTrip.x == Approx(v.x).margin(1e-5));
    REQUIRE(roundTrip.y == Approx(v.y).margin(1e-5));
    REQUIRE(roundTrip.z == Approx(v.z).margin(1e-5));
}

TEST_CASE("LookAt places eye at origin in camera space", "[transform]") {
    Transform worldToCamera = Transform::LookAt(
        Point3f(0.0f, 0.0f, 5.0f), Point3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f));
    Point3f eyeInCameraSpace = worldToCamera(Point3f(0.0f, 0.0f, 5.0f));
    REQUIRE(eyeInCameraSpace.x == Approx(0.0f).margin(1e-5));
    REQUIRE(eyeInCameraSpace.y == Approx(0.0f).margin(1e-5));
    REQUIRE(eyeInCameraSpace.z == Approx(0.0f).margin(1e-5));
}

TEST_CASE("LookAt camera-to-world inverse round-trips a point", "[transform]") {
    Transform worldToCamera = Transform::LookAt(
        Point3f(3.0f, 0.0f, 0.0f), Point3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f));
    Transform cameraToWorld = worldToCamera.Inverse();
    Point3f p(1.0f, 2.0f, 3.0f);
    Point3f roundTrip = cameraToWorld(worldToCamera(p));
    REQUIRE(roundTrip.x == Approx(p.x).margin(1e-4));
    REQUIRE(roundTrip.y == Approx(p.y).margin(1e-4));
    REQUIRE(roundTrip.z == Approx(p.z).margin(1e-4));
}
