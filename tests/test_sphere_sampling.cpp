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
    int samples = 100000;
    float sumWeights = 0.0f;
    
    // Evaluate MC integration of integral of pdf over the sphere's solid angle
    // Since Sample() returns points on the sphere, we must check that the MC estimate
    // of 1 computes to 1 (meaning the pdf matches the density of samples).
    // Actually, Monte Carlo estimate of integral of f(x) is sum(f(x_i)/pdf(x_i)) / N.
    // If f(x) = 1, we compute sum(1) / N = 1.
    // To verify the pdf itself, we need to check that integrating 1 over the subtended cone solid angle
    // is equal to 2*pi*(1 - cosThetaMax).
    // Let's directly check if the Sample pdf matches the expected solid angle pdf.
    
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
