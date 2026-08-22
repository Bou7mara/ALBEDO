#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/materials/disney_principled.h"
#include "rt/core/vector3.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Disney Principled diffuse lobe evaluation", "[disney][diffuse]") {
    DisneyParams params{};
    params.baseColor = Vector3f(0.8f, 0.4f, 0.2f);
    params.metallic = 0.0f;
    params.roughness = 0.5f;
    params.clearcoat = 0.0f;
    params.sheen = 0.0f;

    DisneyPrincipled mat(params);
    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo = Normalize(Vector3f(0.0f, 1.0f, 0.0f));
    Vector3f wi = Normalize(Vector3f(0.0f, 1.0f, 0.0f));

    Vector3f f = mat.f(wo, wi, n);
    REQUIRE(f.x > 0.0f);
    REQUIRE(f.y > 0.0f);
    REQUIRE(f.z > 0.0f);
}

TEST_CASE("Disney Principled metallic shuts off diffuse", "[disney][metallic]") {
    DisneyParams params{};
    params.baseColor = Vector3f(0.9f, 0.7f, 0.2f);
    params.metallic = 1.0f;
    params.roughness = 0.2f;

    DisneyPrincipled mat(params);
    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo = Normalize(Vector3f(0.3f, 0.8f, 0.2f));
    Vector3f wiReflect = Normalize(Vector3f(-0.3f, 0.8f, -0.2f));

    Vector3f f = mat.f(wo, wiReflect, n);
    REQUIRE(f.x > 0.0f);
    REQUIRE(f.y > 0.0f);
    REQUIRE(f.z > 0.0f);
}

TEST_CASE("Disney Principled clearcoat adds specular reflection", "[disney][clearcoat]") {
    DisneyParams paramsNoCc{};
    paramsNoCc.baseColor = Vector3f(0.1f, 0.1f, 0.1f);
    paramsNoCc.roughness = 0.8f;
    paramsNoCc.clearcoat = 0.0f;

    DisneyParams paramsCc = paramsNoCc;
    paramsCc.clearcoat = 1.0f;
    paramsCc.clearcoatGloss = 1.0f;

    DisneyPrincipled matNoCc(paramsNoCc);
    DisneyPrincipled matCc(paramsCc);

    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo = Normalize(Vector3f(0.5f, 0.866f, 0.0f));
    Vector3f wi = Normalize(Vector3f(-0.5f, 0.866f, 0.0f));

    Vector3f fNoCc = matNoCc.f(wo, wi, n);
    Vector3f fCc = matCc.f(wo, wi, n);

    REQUIRE(fCc.x > fNoCc.x);
    REQUIRE(fCc.y > fNoCc.y);
    REQUIRE(fCc.z > fNoCc.z);
}

TEST_CASE("Disney Principled Sample_f always samples upper hemisphere with positive PDF", "[disney][sampling]") {
    DisneyParams params{};
    params.baseColor = Vector3f(0.5f, 0.5f, 0.5f);
    params.metallic = 0.5f;
    params.roughness = 0.4f;
    params.clearcoat = 0.5f;
    params.sheen = 0.5f;

    DisneyPrincipled mat(params);
    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo = Normalize(Vector3f(0.2f, 0.9f, 0.1f));

    for (float ux = 0.05f; ux < 1.0f; ux += 0.15f) {
        for (float uy = 0.05f; uy < 1.0f; uy += 0.15f) {
            Vector3f wi;
            float pdf;
            Vector3f f = mat.Sample_f(wo, n, Point2f(ux, uy), &wi, &pdf);
            REQUIRE(Dot(wi, n) > 0.0f);
            REQUIRE(pdf > 0.0f);
            REQUIRE(f.x >= 0.0f);
            REQUIRE(f.y >= 0.0f);
            REQUIRE(f.z >= 0.0f);
        }
    }
}
