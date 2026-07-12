#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/cameras/perspective_camera.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("PerspectiveCamera generates ray through the center of pixel raster space", "[camera]") {
    // Width = 100, Height = 100, fovY = 90.
    // Camera sits at (0, 0, 0) looking down +Z, up is +Y.
    PerspectiveCamera camera(Point3f(0, 0, 0), Point3f(0, 0, 1), Vector3f(0, 1, 0), 90.0f, 100, 100);

    // Raster coordinate for exact center is (50, 50).
    CameraSample sample;
    sample.pFilm = Point2f(50.0f, 50.0f);

    Ray r = camera.GenerateRay(sample);
    REQUIRE(r.o == Point3f(0.0f, 0.0f, 0.0f));
    // The ray direction should point straight along +Z
    REQUIRE(r.d.x == Approx(0.0f).margin(1e-5));
    REQUIRE(r.d.y == Approx(0.0f).margin(1e-5));
    REQUIRE(r.d.z == Approx(1.0f).margin(1e-5));
}

TEST_CASE("PerspectiveCamera ray directions are always normalized", "[camera]") {
    PerspectiveCamera camera(Point3f(3.0f, 4.0f, 5.0f), Point3f(0, 0, 0), Vector3f(0, 1, 0), 60.0f, 200, 100);
    
    CameraSample sample;
    sample.pFilm = Point2f(12.5f, 80.3f);
    
    Ray r = camera.GenerateRay(sample);
    REQUIRE(Length(r.d) == Approx(1.0f).margin(1e-5));
}
