#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/cam/perspective_camera.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Center-of-image ray points straight down the look direction", "[camera]") {
    PerspectiveCamera cam(Point3f(0, 0, 0), Point3f(0, 0, 1), Vector3f(0, 1, 0),
                          90.0f, 200, 100);

    Ray r = cam.GenerateRay(CameraSample{Point2f(100.0f, 50.0f)});

    REQUIRE(r.o == Point3f(0, 0, 0));
    REQUIRE(r.d.x == Approx(0.0f).margin(1e-4));
    REQUIRE(r.d.y == Approx(0.0f).margin(1e-4));
    REQUIRE(r.d.z == Approx(1.0f).margin(1e-4));
}

TEST_CASE("Camera positioned away from origin generates rays from its own eye point", "[camera]") {
    PerspectiveCamera cam(Point3f(5, 0, 0), Point3f(5, 0, 1), Vector3f(0, 1, 0),
                          90.0f, 200, 100);
    Ray r = cam.GenerateRay(CameraSample{Point2f(100.0f, 50.0f)});

    REQUIRE(r.o == Point3f(5, 0, 0));
}

TEST_CASE("Left edge of image points to negative-x side of the view direction", "[camera]") {
    PerspectiveCamera cam(Point3f(0, 0, 0), Point3f(0, 0, 1), Vector3f(0, 1, 0),
                          90.0f, 200, 100);

    Ray left = cam.GenerateRay(CameraSample{Point2f(0.0f, 50.0f)});
    Ray right = cam.GenerateRay(CameraSample{Point2f(200.0f, 50.0f)});

    REQUIRE(left.d.x < 0.0f);
    REQUIRE(right.d.x > 0.0f);
}

TEST_CASE("Top edge of image points to positive-y side of the view direction", "[camera]") {
    PerspectiveCamera cam(Point3f(0, 0, 0), Point3f(0, 0, 1), Vector3f(0, 1, 0),
                          90.0f, 200, 100);

    Ray top = cam.GenerateRay(CameraSample{Point2f(100.0f, 0.0f)});
    Ray bottom = cam.GenerateRay(CameraSample{Point2f(100.0f, 100.0f)});

    REQUIRE(top.d.y > 0.0f);
    REQUIRE(bottom.d.y < 0.0f);
}

TEST_CASE("Wider FOV produces a wider spread of ray directions at the image edges", "[camera][regression]") {
    PerspectiveCamera narrow(Point3f(0, 0, 0), Point3f(0, 0, 1), Vector3f(0, 1, 0),
                             30.0f, 200, 100);
    PerspectiveCamera wide(Point3f(0, 0, 0), Point3f(0, 0, 1), Vector3f(0, 1, 0),
                           120.0f, 200, 100);

    Ray narrowEdge = narrow.GenerateRay(CameraSample{Point2f(200.0f, 50.0f)});
    Ray wideEdge = wide.GenerateRay(CameraSample{Point2f(200.0f, 50.0f)});

    REQUIRE(wideEdge.d.x > narrowEdge.d.x);
}

TEST_CASE("Generated ray direction is always unit length", "[camera]") {
    PerspectiveCamera cam(Point3f(1, 2, 3), Point3f(4, 5, 6), Vector3f(0, 1, 0),
                          60.0f, 320, 240);
    Ray r = cam.GenerateRay(CameraSample{Point2f(37.0f, 111.0f)});

    REQUIRE(Length(r.d) == Approx(1.0f).margin(1e-5));
}
