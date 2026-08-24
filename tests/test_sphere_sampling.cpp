#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "rt/shapes/sphere.h"
#include "rt/core/rng.h"
#include <numbers>

using namespace rt;
using Catch::Matchers::WithinRel;

TEST_CASE("Sphere Cone Sampling integrates to 1", "[sphere][sampling]") {
    Sphere sphere(Transform::Translate(Vector3f(0, 0, -5)), 1.0f);
    Point3f ref(0, 0, 0);

    RNG rng;

    float d2 = 25.0f;
    float r2 = 1.0f;
    float sinThetaMax2 = r2 / d2;
    float cosThetaMax = std::sqrt(std::max(0.0f, 1.0f - sinThetaMax2));
    float expectedPdf = 1.0f / (2.0f * std::numbers::pi_v<float> * (1.0f - cosThetaMax));

    for (int i = 0; i < 10; ++i) {
        ShapeSample s = sphere.Sample(ref, rng.Uniform2D());
        REQUIRE_THAT(s.pdf, WithinRel(expectedPdf, 1e-4f));

        Vector3f wi = Normalize(s.p - ref);
        float pdf = sphere.Pdf(ref, wi);
        REQUIRE_THAT(pdf, WithinRel(expectedPdf, 1e-4f));
    }
}
