#include <catch2/catch_test_macros.hpp>
#include "rt/scene/scene.h"
#include "rt/shapes/sphere.h"
#include "rt/core/transform.h"
#include <memory>

using namespace rt;

TEST_CASE("Scene with no shapes reports no hit", "[scene]") {
    Scene scene;
    scene.Build();
    Ray r(Point3f(0, 0, 0), Vector3f(0, 0, -1));
    SurfaceInteraction isect;
    REQUIRE_FALSE(scene.Intersect(r, &isect));
}

TEST_CASE("Scene finds a hit on a single sphere", "[scene]") {
    Scene scene;
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0, 0, -5)), 1.0f));
    scene.Build();

    Ray r(Point3f(0, 0, 0), Vector3f(0, 0, -1));
    SurfaceInteraction isect;
    REQUIRE(scene.Intersect(r, &isect));
}

TEST_CASE("Scene returns the CLOSEST hit, regardless of insertion order", "[scene][regression]") {
    Scene scene;
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0, 0, -10)), 1.0f));
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0, 0, -3)), 1.0f)); 
    scene.Build();

    Ray r(Point3f(0, 0, 0), Vector3f(0, 0, -1));
    SurfaceInteraction isect;
    REQUIRE(scene.Intersect(r, &isect));
    REQUIRE(isect.t < 5.0f);
}

TEST_CASE("Scene reports no hit when ray points away from all shapes", "[scene]") {
    Scene scene;
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0, 0, -5)), 1.0f));
    scene.Build();

    Ray r(Point3f(0, 0, 0), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE_FALSE(scene.Intersect(r, &isect));
}
