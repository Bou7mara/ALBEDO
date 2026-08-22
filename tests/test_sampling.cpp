#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/core/sampling.h"
#include <numbers>

using namespace rt;
using Catch::Approx;

TEST_CASE("Cosine-sampled hemisphere directions are unit length and face +z", "[sampling]") {
    for (float ux = 0.05f; ux < 1.0f; ux += 0.1f) {
        for (float uy = 0.05f; uy < 1.0f; uy += 0.1f) {
            Vector3f d = CosineSampleHemisphere(Point2f(ux, uy));
            REQUIRE(Length(d) == Approx(1.0f).margin(1e-4));
            REQUIRE(d.z >= 0.0f);
        }
    }
}

TEST_CASE("CosineHemispherePdf(1.0) equals 1/pi", "[sampling]") {
    REQUIRE(CosineHemispherePdf(1.0f) == Approx(std::numbers::inv_pi_v<float>));
}

TEST_CASE("ConcentricSampleDisk never leaves the unit disk", "[sampling]") {
    for (float ux = 0.0f; ux <= 1.0f; ux += 0.1f) {
        for (float uy = 0.0f; uy <= 1.0f; uy += 0.1f) {
            Point2f d = ConcentricSampleDisk(Point2f(ux, uy));
            REQUIRE(d.x * d.x + d.y * d.y <= 1.0f + 1e-4f);
        }
    }
}
