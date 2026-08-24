#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/textures/image2d.h"
#include "rt/core/vector3.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Image2D exact texel center sampling", "[texture][sampler]") {

    Image2D<float> img(2, 2, WrapMode::Clamp);
    img.Set(0, 0, 10.0f);
    img.Set(1, 0, 20.0f);
    img.Set(0, 1, 30.0f);
    img.Set(1, 1, 40.0f);

    REQUIRE(img.Sample(0.25f, 0.25f) == Approx(10.0f));
    REQUIRE(img.Sample(0.75f, 0.25f) == Approx(20.0f));
    REQUIRE(img.Sample(0.25f, 0.75f) == Approx(30.0f));
    REQUIRE(img.Sample(0.75f, 0.75f) == Approx(40.0f));
}

TEST_CASE("Image2D bilinear interpolation midpoints", "[texture][sampler]") {
    Image2D<float> img(2, 2, WrapMode::Clamp);
    img.Set(0, 0, 10.0f);
    img.Set(1, 0, 20.0f);
    img.Set(0, 1, 30.0f);
    img.Set(1, 1, 40.0f);

    REQUIRE(img.Sample(0.5f, 0.25f) == Approx(15.0f));

    REQUIRE(img.Sample(0.25f, 0.5f) == Approx(20.0f));

    REQUIRE(img.Sample(0.5f, 0.5f) == Approx(25.0f));
}

TEST_CASE("Image2D WrapMode::Clamp boundary handling", "[texture][sampler][clamp]") {
    Image2D<float> img(2, 2, WrapMode::Clamp);
    img.Set(0, 0, 10.0f);
    img.Set(1, 0, 20.0f);
    img.Set(0, 1, 30.0f);
    img.Set(1, 1, 40.0f);

    REQUIRE(img.Sample(-1.0f, -1.0f) == Approx(10.0f));
    REQUIRE(img.Sample(0.0f, 0.0f) == Approx(10.0f));

    REQUIRE(img.Sample(2.0f, 2.0f) == Approx(40.0f));
    REQUIRE(img.Sample(1.0f, 1.0f) == Approx(40.0f));
}

TEST_CASE("Image2D WrapMode::Repeat wrapping across boundaries", "[texture][sampler][repeat]") {
    Image2D<float> img(2, 2, WrapMode::Repeat);
    img.Set(0, 0, 10.0f);
    img.Set(1, 0, 20.0f);
    img.Set(0, 1, 30.0f);
    img.Set(1, 1, 40.0f);

    REQUIRE(img.Sample(1.25f, 1.25f) == Approx(10.0f));

    REQUIRE(img.Sample(-0.25f, -0.25f) == Approx(40.0f));

    REQUIRE(img.Sample(1.5f, 0.25f) == Approx(15.0f));
}

TEST_CASE("Image2D Vector3f color texture sampling", "[texture][vector]") {
    Image2D<Vector3f> img(2, 2, WrapMode::Clamp);
    img.Set(0, 0, Vector3f(1.0f, 0.0f, 0.0f));
    img.Set(1, 0, Vector3f(0.0f, 1.0f, 0.0f));
    img.Set(0, 1, Vector3f(0.0f, 0.0f, 1.0f));
    img.Set(1, 1, Vector3f(1.0f, 1.0f, 1.0f));

    Vector3f center = img.Sample(0.5f, 0.5f);
    REQUIRE(center.x == Approx(0.5f));
    REQUIRE(center.y == Approx(0.5f));
    REQUIRE(center.z == Approx(0.5f));
}
