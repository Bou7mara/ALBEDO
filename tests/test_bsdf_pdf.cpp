#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/dielectric.h"
#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/emissive.h"
#include "rt/core/rng.h"
#include "rt/core/onb.h"

using namespace rt;
using Catch::Matchers::WithinAbs;

TEST_CASE("BSDF Pdf vs Sample_f matches", "[bsdf][pdf]") {
    RNG rng;
    Vector3f n(0, 0, 1);
    Vector3f wo = Normalize(Vector3f(0.5f, 0.5f, 1.0f));

    auto testBsdf = [&](const BSDF& bsdf) {
        for (int i = 0; i < 100; ++i) {
            Vector3f wi;
            float pdf;
            [[maybe_unused]] Vector3f f = bsdf.Sample_f(wo, n, rng.Uniform2D(), &wi, &pdf);
            
            float expectedPdf = bsdf.Pdf(wo, wi, n);
            REQUIRE_THAT(pdf, WithinAbs(expectedPdf, 1e-4f));
        }
    };

    SECTION("Lambertian") {
        Lambertian lambertian(Vector3f(1, 1, 1));
        testBsdf(lambertian);
    }

    SECTION("Metal") {
        Metal metal(Vector3f(1, 1, 1));
        Vector3f wi;
        float pdf;
        metal.Sample_f(wo, n, rng.Uniform2D(), &wi, &pdf);
        REQUIRE_THAT(metal.Pdf(wo, wi, n), WithinAbs(0.0f, 1e-6f));
    }

    SECTION("Dielectric") {
        Dielectric dielectric(1.5f);
        Vector3f wi;
        float pdf;
        dielectric.Sample_f(wo, n, rng.Uniform2D(), &wi, &pdf);
        REQUIRE_THAT(dielectric.Pdf(wo, wi, n), WithinAbs(0.0f, 1e-6f));
    }

    SECTION("Emissive") {
        Emissive emissive(Vector3f(1, 1, 1));
        testBsdf(emissive);
    }

    SECTION("Microfacet") {
        Microfacet micro = Microfacet::MakeDielectricMicrofacet(0.5f, 1.5f);
        testBsdf(micro);
    }
}
