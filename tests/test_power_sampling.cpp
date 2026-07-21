#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "rt/scene/scene.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/emissive.h"
#include "rt/materials/lambertian.h"

using namespace rt;
using Catch::Matchers::WithinRel;

TEST_CASE("Power-Weighted Light Selection CDF & PMF", "[scene][light_sampling]") {
    Scene scene;
    
    auto lightMat1 = std::make_shared<Emissive>(Vector3f(10.0f, 10.0f, 10.0f));
    scene.Add(std::make_shared<Sphere>(Transform::Translate(Vector3f(0, 0, 0)), 0.5f, lightMat1));

    auto lightMat2 = std::make_shared<Emissive>(Vector3f(30.0f, 30.0f, 30.0f));
    scene.Add(std::make_shared<Sphere>(Transform::Translate(Vector3f(5, 0, 0)), 1.0f, lightMat2));

    scene.Build();

    REQUIRE(scene.Lights().size() == 2);

    int idx0 = -1, idx1 = -1;
    float pmf0 = 0.0f, pmf1 = 0.0f;

    scene.SampleLight(0.0f, &idx0, &pmf0);
    REQUIRE(idx0 == 0);

    scene.SampleLight(0.99f, &idx1, &pmf1);
    REQUIRE(idx1 == 1);

    REQUIRE_THAT(pmf0 + pmf1, WithinRel(1.0f, 1e-4f));
    REQUIRE(pmf1 > pmf0 * 10.0f);
}
