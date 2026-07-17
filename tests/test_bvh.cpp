#include <catch2/catch_test_macros.hpp>
#include "rt/accel/bvh.h"
#include "rt/shapes/sphere.h"
#include <random>
#include <catch2/catch_approx.hpp>

using namespace rt;

namespace {
bool LinearIntersect(const std::vector<std::shared_ptr<Shape>>& shapes,
                      const Ray& ray, SurfaceInteraction* isect) {
    bool hit = false;
    for (const auto& s : shapes) if (s->Intersect(ray, isect)) hit = true;
    return hit;
}

std::vector<std::shared_ptr<Shape>> MakeGridOfSpheres() {
    std::vector<std::shared_ptr<Shape>> shapes;
    for (int x = -2; x <= 2; ++x)
        for (int z = -2; z <= 2; ++z)
            shapes.push_back(std::make_shared<Sphere>(
                Transform::Translate(Vector3f(x * 2.0f, 0.0f, z * 2.0f - 10.0f)), 0.4f));
    return shapes;
}
}

TEST_CASE("BVH (SAH) matches brute-force linear scan across random rays", "[bvh]") {
    auto shapes = MakeGridOfSpheres();
    BVH bvh(shapes, 4, BVH::SplitMethod::SAH);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-3.0f, 3.0f);

    for (int i = 0; i < 200; ++i) {
        Ray rBvh(Point3f(dist(rng), dist(rng), -20.0f),
                  Normalize(Vector3f(dist(rng) * 0.1f, dist(rng) * 0.1f, 1.0f)));
        Ray rLinear = rBvh;

        SurfaceInteraction isectBvh, isectLinear;
        bool hitBvh = bvh.Intersect(rBvh, &isectBvh);
        bool hitLinear = LinearIntersect(shapes, rLinear, &isectLinear);

        REQUIRE(hitBvh == hitLinear);
        if (hitBvh) {
            REQUIRE(isectBvh.t == Catch::Approx(isectLinear.t).margin(1e-4));
        }
    }
}

TEST_CASE("BVH (Midpoint) also matches brute-force linear scan", "[bvh]") {
    auto shapes = MakeGridOfSpheres();
    BVH bvh(shapes, 4, BVH::SplitMethod::Midpoint);

    Ray r(Point3f(0, 0, -20), Vector3f(0, 0, 1));
    SurfaceInteraction isectBvh, isectLinear;
    bool hitBvh = bvh.Intersect(r, &isectBvh);
    Ray r2 = r;
    bool hitLinear = LinearIntersect(shapes, r2, &isectLinear);

    REQUIRE(hitBvh == hitLinear);
    REQUIRE(isectBvh.t == Catch::Approx(isectLinear.t).margin(1e-4));
}

TEST_CASE("BVH correctly reports a miss when a ray clears every primitive", "[bvh]") {
    auto shapes = MakeGridOfSpheres();
    BVH bvh(shapes);
    Ray r(Point3f(1000, 1000, -20), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE_FALSE(bvh.Intersect(r, &isect));
}

TEST_CASE("BVH with zero shapes never crashes and always reports a miss", "[bvh][regression]") {
    BVH bvh({});
    Ray r(Point3f(0, 0, 0), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE_FALSE(bvh.Intersect(r, &isect));
}

TEST_CASE("BVH with a single shape still works", "[bvh][regression]") {
    std::vector<std::shared_ptr<Shape>> shapes{
        std::make_shared<Sphere>(Transform::Identity(), 1.0f)};
    BVH bvh(shapes);
    Ray r(Point3f(0, 0, -5), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE(bvh.Intersect(r, &isect));
}
