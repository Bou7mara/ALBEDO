#include <catch2/catch_test_macros.hpp>
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"

using namespace rt;

TEST_CASE("Shape with no BSDF reports GetBSDF() == nullptr", "[shape]") {
    Sphere s(Transform::Identity(), 1.0f);
    REQUIRE(s.GetBSDF() == nullptr);
}

TEST_CASE("Shape constructed with a BSDF reports it via GetBSDF()", "[shape]") {
    auto mat = std::make_shared<Lambertian>(Vector3f(0.5f, 0.5f, 0.5f));
    Sphere s(Transform::Identity(), 1.0f, mat);
    REQUIRE(s.GetBSDF() == mat.get());
}
