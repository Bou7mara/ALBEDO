#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/textures/image2d.h"
#include "rt/core/vector3.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Image2D exact texel center sampling", "[texture][sampler]") {
    // 2x2 scalar texture
    // [0,0]=10.0f  [1,0]=20.0f
    // [0,1]=30.0f  [1,1]=40.0f
    Image2D<float> img(2, 2, WrapMode::Clamp);
    img.Set(0, 0, 10.0f);
    img.Set(1, 0, 20.0f);
    img.Set(0, 1, 30.0f);
    img.Set(1, 1, 40.0f);

    // Texel centers for 2x2 are (0.25, 0.25), (0.75, 0.25), (0.25, 0.75), (0.75, 0.75)
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

    // Horizontal midpoint between (0.25, 0.25) and (0.75, 0.25) is (0.5, 0.25) -> 15.0f
    REQUIRE(img.Sample(0.5f, 0.25f) == Approx(15.0f));

    // Vertical midpoint between (0.25, 0.25) and (0.25, 0.75) is (0.25, 0.5) -> 20.0f
    REQUIRE(img.Sample(0.25f, 0.5f) == Approx(20.0f));

    // Center of all 4 texels at (0.5, 0.5) -> (10+20+30+40)/4 = 25.0f
    REQUIRE(img.Sample(0.5f, 0.5f) == Approx(25.0f));
}

TEST_CASE("Image2D WrapMode::Clamp boundary handling", "[texture][sampler][clamp]") {
    Image2D<float> img(2, 2, WrapMode::Clamp);
    img.Set(0, 0, 10.0f);
    img.Set(1, 0, 20.0f);
    img.Set(0, 1, 30.0f);
    img.Set(1, 1, 40.0f);

    // Out-of-bounds negative coordinates should clamp to top-left texel (10.0f)
    REQUIRE(img.Sample(-1.0f, -1.0f) == Approx(10.0f));
    REQUIRE(img.Sample(0.0f, 0.0f) == Approx(10.0f));

    // Out-of-bounds positive coordinates should clamp to bottom-right texel (40.0f)
    REQUIRE(img.Sample(2.0f, 2.0f) == Approx(40.0f));
    REQUIRE(img.Sample(1.0f, 1.0f) == Approx(40.0f));
}

TEST_CASE("Image2D WrapMode::Repeat wrapping across boundaries", "[texture][sampler][repeat]") {
    Image2D<float> img(2, 2, WrapMode::Repeat);
    img.Set(0, 0, 10.0f);
    img.Set(1, 0, 20.0f);
    img.Set(0, 1, 30.0f);
    img.Set(1, 1, 40.0f);

    // UV = (1.25, 1.25) wraps to (0.25, 0.25) -> top-left texel (10.0f)
    REQUIRE(img.Sample(1.25f, 1.25f) == Approx(10.0f));

    // UV = (-0.25, -0.25) wraps to (0.75, 0.75) -> bottom-right texel (40.0f)
    REQUIRE(img.Sample(-0.25f, -0.25f) == Approx(40.0f));

    // Midpoint wrap at u = 1.5, v = 0.25 wraps to (0.5, 0.25) -> 15.0f
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
