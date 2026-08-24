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

TEST_CASE("Parallel BVH construction produces bit-for-bit identical tree to serial build (SAH)", "[bvh][parallel]") {
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> posDist(-20.0f, 20.0f);
    std::uniform_real_distribution<float> radDist(0.1f, 0.5f);

    const int kNumSpheres = 3000;
    std::vector<std::shared_ptr<Shape>> shapes;
    shapes.reserve(kNumSpheres);
    for (int i = 0; i < kNumSpheres; ++i) {
        Point3f center(posDist(rng), posDist(rng), posDist(rng) - 30.0f);
        shapes.push_back(std::make_shared<Sphere>(Transform::Translate(Vector3f(center.x, center.y, center.z)), radDist(rng)));
    }

    BVH bvhSerial(shapes, 4, BVH::SplitMethod::SAH, 1);
    BVH bvhParallel(shapes, 4, BVH::SplitMethod::SAH, 0);

    REQUIRE(bvhSerial.NodeCount() > 0);
    REQUIRE(bvhSerial.NodeCount() == bvhParallel.NodeCount());
    REQUIRE(bvhSerial.OrderedShapes().size() == bvhParallel.OrderedShapes().size());

    const auto& nodesSerial = bvhSerial.Nodes();
    const auto& nodesParallel = bvhParallel.Nodes();
    for (size_t i = 0; i < nodesSerial.size(); ++i) {
        REQUIRE(nodesSerial[i].bounds.minPt.x == nodesParallel[i].bounds.minPt.x);
        REQUIRE(nodesSerial[i].bounds.minPt.y == nodesParallel[i].bounds.minPt.y);
        REQUIRE(nodesSerial[i].bounds.minPt.z == nodesParallel[i].bounds.minPt.z);
        REQUIRE(nodesSerial[i].bounds.maxPt.x == nodesParallel[i].bounds.maxPt.x);
        REQUIRE(nodesSerial[i].bounds.maxPt.y == nodesParallel[i].bounds.maxPt.y);
        REQUIRE(nodesSerial[i].bounds.maxPt.z == nodesParallel[i].bounds.maxPt.z);
        REQUIRE(nodesSerial[i].nPrimitives == nodesParallel[i].nPrimitives);
        REQUIRE(nodesSerial[i].axis == nodesParallel[i].axis);
        if (nodesSerial[i].nPrimitives > 0) {
            REQUIRE(nodesSerial[i].primitivesOffset == nodesParallel[i].primitivesOffset);
        } else {
            REQUIRE(nodesSerial[i].secondChildOffset == nodesParallel[i].secondChildOffset);
        }
    }

    const auto& shapesSerial = bvhSerial.OrderedShapes();
    const auto& shapesParallel = bvhParallel.OrderedShapes();
    for (size_t i = 0; i < shapesSerial.size(); ++i) {
        REQUIRE(shapesSerial[i].get() == shapesParallel[i].get());
    }

    for (int i = 0; i < 500; ++i) {
        Ray ray(Point3f(posDist(rng) * 0.5f, posDist(rng) * 0.5f, 10.0f),
                Normalize(Vector3f(posDist(rng), posDist(rng), -1.0f)));
        SurfaceInteraction isectSerial, isectParallel;
        bool hitSerial = bvhSerial.Intersect(ray, &isectSerial);
        bool hitParallel = bvhParallel.Intersect(ray, &isectParallel);
        REQUIRE(hitSerial == hitParallel);
        if (hitSerial) {
            REQUIRE(isectSerial.t == Catch::Approx(isectParallel.t).margin(1e-5f));
            REQUIRE(isectSerial.p.x == Catch::Approx(isectParallel.p.x).margin(1e-5f));
            REQUIRE(isectSerial.p.y == Catch::Approx(isectParallel.p.y).margin(1e-5f));
            REQUIRE(isectSerial.p.z == Catch::Approx(isectParallel.p.z).margin(1e-5f));
        }
    }
}

TEST_CASE("Parallel BVH handles small scenes below parallel cutoff", "[bvh][parallel]") {
    auto shapes = MakeGridOfSpheres();
    BVH bvhSerial(shapes, 4, BVH::SplitMethod::SAH, 1);
    BVH bvhParallel(shapes, 4, BVH::SplitMethod::SAH, 0);

    REQUIRE(bvhSerial.NodeCount() == bvhParallel.NodeCount());
    for (size_t i = 0; i < bvhSerial.NodeCount(); ++i) {
        REQUIRE(bvhSerial.Nodes()[i].nPrimitives == bvhParallel.Nodes()[i].nPrimitives);
        REQUIRE(bvhSerial.Nodes()[i].axis == bvhParallel.Nodes()[i].axis);
    }
}

TEST_CASE("Parallel BVH stress and repeated construction stability", "[bvh][parallel][stress]") {
    std::mt19937 rng(4242);
    std::uniform_real_distribution<float> dist(-15.0f, 15.0f);

    const int kNumSpheres = 5000;
    std::vector<std::shared_ptr<Shape>> shapes;
    shapes.reserve(kNumSpheres);
    for (int i = 0; i < kNumSpheres; ++i) {
        shapes.push_back(std::make_shared<Sphere>(
            Transform::Translate(Vector3f(dist(rng), dist(rng), dist(rng) - 25.0f)), 0.25f));
    }

    BVH ref(shapes, 4, BVH::SplitMethod::SAH, 0);
    size_t expectedNodes = ref.NodeCount();

    for (int run = 0; run < 5; ++run) {
        BVH bvh(shapes, 4, BVH::SplitMethod::SAH, 0);
        REQUIRE(bvh.NodeCount() == expectedNodes);
        REQUIRE(bvh.WorldBound().minPt.x == Catch::Approx(ref.WorldBound().minPt.x));
        REQUIRE(bvh.WorldBound().maxPt.x == Catch::Approx(ref.WorldBound().maxPt.x));
    }
}

