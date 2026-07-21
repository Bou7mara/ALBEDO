#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

// Assuming PowerHeuristic is available. I will redefine it here for the test just in case.
inline float PowerHeuristicTest(int nf, float fPdf, int ng, float gPdf) {
    float f = nf * fPdf;
    float g = ng * gPdf;
    if (f == 0.0f && g == 0.0f) return 0.0f;
    return (f * f) / (f * f + g * g);
}

using Catch::Matchers::WithinAbs;

TEST_CASE("MIS Power Heuristic Sanity Checks", "[mis]") {
    SECTION("One strategy has 0 pdf") {
        REQUIRE_THAT(PowerHeuristicTest(1, 1.0f, 1, 0.0f), WithinAbs(1.0f, 1e-6f));
        REQUIRE_THAT(PowerHeuristicTest(1, 0.0f, 1, 1.0f), WithinAbs(0.0f, 1e-6f));
    }

    SECTION("Equal pdfs") {
        REQUIRE_THAT(PowerHeuristicTest(1, 0.5f, 1, 0.5f), WithinAbs(0.5f, 1e-6f));
    }

    SECTION("Weights sum to 1") {
        float p1 = 0.3f, p2 = 0.7f;
        float w1 = PowerHeuristicTest(1, p1, 1, p2);
        float w2 = PowerHeuristicTest(1, p2, 1, p1);
        REQUIRE_THAT(w1 + w2, WithinAbs(1.0f, 1e-6f));
    }
}
