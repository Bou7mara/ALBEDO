#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/disney_principled.h"
#include "rt/textures/image2d.h"

#include <numbers>

using namespace rt;
using Catch::Approx;

TEST_CASE("Untextured material regression with arbitrary UVs", "[material][texture][regression]") {
    Vector3f albedo(0.7f, 0.2f, 0.3f);
    Lambertian lamb(albedo);
    Vector3f wo(0.0f, 0.0f, 1.0f);
    Vector3f wi(0.0f, 0.0f, 1.0f);
    Vector3f n(0.0f, 0.0f, 1.0f);

    Vector3f f0 = lamb.f(wo, wi, n, Point2f(0.0f, 0.0f));
    Vector3f f1 = lamb.f(wo, wi, n, Point2f(0.42f, 0.88f));

    REQUIRE(f0.x == Approx(f1.x));
    REQUIRE(f0.y == Approx(f1.y));
    REQUIRE(f0.z == Approx(f1.z));
    REQUIRE(f0.x == Approx(albedo.x * std::numbers::inv_pi_v<float>));
}

TEST_CASE("Checkerboard textured Lambertian evaluation", "[material][texture][lambertian]") {
    auto tex = std::make_shared<Image2D<Vector3f>>(2, 2, WrapMode::Clamp);
    Vector3f white(1.0f, 1.0f, 1.0f);
    Vector3f black(0.0f, 0.0f, 0.0f);
    tex->Set(0, 0, white);
    tex->Set(1, 0, black);
    tex->Set(0, 1, black);
    tex->Set(1, 1, white);

    Lambertian lamb(tex);
    Vector3f wo(0.0f, 0.0f, 1.0f);
    Vector3f wi(0.0f, 0.0f, 1.0f);
    Vector3f n(0.0f, 0.0f, 1.0f);

    // Sample top-left (white tile center at (0.25, 0.25))
    Vector3f fWhite = lamb.f(wo, wi, n, Point2f(0.25f, 0.25f));
    REQUIRE(fWhite.x == Approx(std::numbers::inv_pi_v<float>));
    REQUIRE(fWhite.y == Approx(std::numbers::inv_pi_v<float>));
    REQUIRE(fWhite.z == Approx(std::numbers::inv_pi_v<float>));

    // Sample top-right (black tile center at (0.75, 0.25))
    Vector3f fBlack = lamb.f(wo, wi, n, Point2f(0.75f, 0.25f));
    REQUIRE(fBlack.x == Approx(0.0f));
    REQUIRE(fBlack.y == Approx(0.0f));
    REQUIRE(fBlack.z == Approx(0.0f));
}

TEST_CASE("Textured Microfacet Conductor evaluation", "[material][texture][microfacet]") {
    auto roughTex = std::make_shared<Image2D<float>>(2, 1, WrapMode::Clamp);
    roughTex->Set(0, 0, 0.2f);
    roughTex->Set(1, 0, 0.8f);

    Vector3f eta(0.2f, 0.9f, 1.1f);
    Vector3f k(3.5f, 2.5f, 1.8f);
    Microfacet mf = Microfacet::MakeConductorMicrofacetTextured(roughTex, eta, k);

    Vector3f wo = Normalize(Vector3f(0.0f, 1.0f, 1.0f));
    Vector3f wi = Normalize(Vector3f(0.0f, -1.0f, 1.0f));
    Vector3f n(0.0f, 0.0f, 1.0f);

    Vector3f fSmooth = mf.f(wo, wi, n, Point2f(0.25f, 0.5f));
    Vector3f fRough  = mf.f(wo, wi, n, Point2f(0.75f, 0.5f));

    REQUIRE(fSmooth.x > 0.0f);
    REQUIRE(fRough.x > 0.0f);
    REQUIRE(fSmooth.x != Approx(fRough.x));
}

TEST_CASE("Textured Disney Principled evaluation", "[material][texture][disney]") {
    auto colorTex = std::make_shared<Image2D<Vector3f>>(2, 1, WrapMode::Clamp);
    colorTex->Set(0, 0, Vector3f(1.0f, 0.0f, 0.0f));
    colorTex->Set(1, 0, Vector3f(0.0f, 0.0f, 1.0f));

    DisneyParams params{};
    params.baseColorTexture = colorTex;
    params.roughness = 0.5f;
    DisneyPrincipled disney(params);

    Vector3f wo(0.0f, 0.0f, 1.0f);
    Vector3f wi(0.0f, 0.0f, 1.0f);
    Vector3f n(0.0f, 0.0f, 1.0f);

    Vector3f fRed  = disney.f(wo, wi, n, Point2f(0.25f, 0.5f));
    Vector3f fBlue = disney.f(wo, wi, n, Point2f(0.75f, 0.5f));

    REQUIRE(fRed.x > fRed.z);
    REQUIRE(fBlue.z > fBlue.x);
}
