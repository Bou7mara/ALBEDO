#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/materials/microfacet_brdf.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Microfacet f() is non-negative everywhere", "[microfacet_brdf]") {
    Microfacet mf = Microfacet::MakeDielectricMicrofacet(0.3f, 1.5f);
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(0.2f, 0.9f, 0.1f));

    for (float x = -0.8f; x <= 0.8f; x += 0.2f) {
        for (float z = -0.8f; z <= 0.8f; z += 0.2f) {
            Vector3f wi = Normalize(Vector3f(x, 0.5f, z));
            Vector3f f = mf.f(wo, wi, n);
            REQUIRE(f.x >= 0.0f);
            REQUIRE(f.y >= 0.0f);
            REQUIRE(f.z >= 0.0f);
        }
    }
}

TEST_CASE("Microfacet f() obeys Helmholtz reciprocity", "[microfacet_brdf][regression]") {
    // f(wo, wi) must equal f(wi, wo) -- swapping which direction is
    // "outgoing" and which is "incident" cannot change the BRDF value.
    // This is exactly the check that would catch an accidental NdotV/NdotL
    // mixup inside the D*G*F assembly.
    Microfacet mf = Microfacet::MakeConductorMicrofacet(
        0.25f, Vector3f(0.2f, 0.2f, 0.2f), Vector3f(3.0f, 3.0f, 3.0f));
    Vector3f n(0, 1, 0);
    Vector3f a = Normalize(Vector3f(0.4f, 0.7f, 0.1f));
    Vector3f b = Normalize(Vector3f(-0.3f, 0.6f, 0.5f));

    Vector3f fab = mf.f(a, b, n);
    Vector3f fba = mf.f(b, a, n);
    REQUIRE(fab.x == Approx(fba.x).margin(1e-4));
    REQUIRE(fab.y == Approx(fba.y).margin(1e-4));
    REQUIRE(fab.z == Approx(fba.z).margin(1e-4));
}

TEST_CASE("Microfacet Sample_f produces a pdf consistent with its own returned wi", "[microfacet_brdf][regression]") {
    // Sample_f must return f(wo, wi) for the SAME wi it just picked, and a
    // strictly positive pdf for any non-degenerate sample -- catches the
    // "paired 1:1, not a general evaluator" contract from GgxVndfPdf being
    // violated by passing in stale or mismatched intermediate values.
    Microfacet mf = Microfacet::MakeDielectricMicrofacet(0.4f, 1.5f);
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(0.1f, 0.95f, 0.05f));

    Vector3f wi;
    float pdf;
    Vector3f sampled = mf.Sample_f(wo, n, Point2f(0.4f, 0.6f), &wi, &pdf);

    REQUIRE(pdf > 0.0f);
    Vector3f direct = mf.f(wo, wi, n);
    REQUIRE(sampled.x == Approx(direct.x).margin(1e-5));
    REQUIRE(sampled.y == Approx(direct.y).margin(1e-5));
    REQUIRE(sampled.z == Approx(direct.z).margin(1e-5));
}

TEST_CASE("Microfacet at very low roughness concentrates energy near the mirror direction", "[microfacet_brdf][regression]") {
    // Same near-delta intuition as GgxD/SampleGgxVndf's own tests, now
    // checked at the assembled-BRDF level: f() evaluated far from the
    // mirror-reflection direction should be dramatically smaller than f()
    // evaluated at it.
    Microfacet mf = Microfacet::MakeConductorMicrofacet(
        0.02f, Vector3f(0.2f, 0.2f, 0.2f), Vector3f(3.0f, 3.0f, 3.0f));
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(0.0f, 1.0f, 0.0f));
    Vector3f mirrorWi = Reflect(wo, n);   // (0,1,0), normal incidence

    Vector3f atMirror = mf.f(wo, mirrorWi, n);
    Vector3f offMirror = mf.f(wo, Normalize(Vector3f(0.5f, 0.7f, 0.2f)), n);

    REQUIRE(atMirror.x > offMirror.x * 10.0f);
}

TEST_CASE("Microfacet Sample_f direction is always unit length and on the correct side", "[microfacet_brdf]") {
    Microfacet mf = Microfacet::MakeDielectricMicrofacet(0.5f, 1.5f);
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(0.3f, 0.8f, 0.2f));
    Vector3f wi;
    float pdf;
    for (float ux : {0.1f, 0.5f, 0.9f}) {
        for (float uy : {0.1f, 0.5f, 0.9f}) {
            mf.Sample_f(wo, n, Point2f(ux, uy), &wi, &pdf);
            if (pdf > 0.0f) {
                REQUIRE(Length(wi) == Approx(1.0f).margin(1e-3));
                REQUIRE(Dot(wi, n) > -1e-4f);   // guard from Step 4 should
                                                  // reject the wrong side
            }
        }
    }
}
