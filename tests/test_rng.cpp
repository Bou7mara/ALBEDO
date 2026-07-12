#include <catch2/catch_test_macros.hpp>
#include "rt/core/rng.h"

using namespace rt;

TEST_CASE("RNG outputs are within bounds [0, 1)", "[rng]") {
    RNG rng;
    for (int i = 0; i < 1000; ++i) {
        float val1d = rng.Uniform1D();
        REQUIRE(val1d >= 0.0f);
        REQUIRE(val1d < 1.0f);

        Point2f val2d = rng.Uniform2D();
        REQUIRE(val2d.x >= 0.0f);
        REQUIRE(val2d.x < 1.0f);
        REQUIRE(val2d.y >= 0.0f);
        REQUIRE(val2d.y < 1.0f);
    }
}

TEST_CASE("RNG is reproducible with fixed seed", "[rng]") {
    RNG rng1(42);
    RNG rng2(42);

    for (int i = 0; i < 100; ++i) {
        REQUIRE(rng1.Uniform1D() == rng2.Uniform1D());
        REQUIRE(rng1.Uniform2D() == rng2.Uniform2D());
    }
}
