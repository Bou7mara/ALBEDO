#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/accel/bvh4.h"
#include "rt/accel/bvh.h"
#include "rt/shapes/sphere.h"
#include "rt/shapes/triangle.h"
#include "rt/materials/lambertian.h"
#include <random>

using namespace rt;
using Catch::Approx;

namespace {

bool ScalarBoxIntersect(const Bounds3f& box, const Ray& ray, const Vector3f& invDir, const int dirIsNeg[3], float tMax, float* tHit) {
    float t0x = ((dirIsNeg[0] ? box.maxPt.x : box.minPt.x) - ray.o.x) * invDir.x;
    float t1x = ((dirIsNeg[0] ? box.minPt.x : box.maxPt.x) - ray.o.x) * invDir.x;
    float t0y = ((dirIsNeg[1] ? box.maxPt.y : box.minPt.y) - ray.o.y) * invDir.y;
    float t1y = ((dirIsNeg[1] ? box.minPt.y : box.maxPt.y) - ray.o.y) * invDir.y;
    float t0z = ((dirIsNeg[2] ? box.maxPt.z : box.minPt.z) - ray.o.z) * invDir.z;
    float t1z = ((dirIsNeg[2] ? box.minPt.z : box.maxPt.z) - ray.o.z) * invDir.z;

    float tNear = std::max({0.0f, std::min(t0x, t1x), std::min(t0y, t1y), std::min(t0z, t1z)});
    float tFar  = std::min({tMax, std::max(t0x, t1x), std::max(t0y, t1y), std::max(t0z, t1z)});

    if (tNear <= tFar) {
        if (tHit) *tHit = tNear;
        return true;
    }
    return false;
}

std::vector<std::shared_ptr<Shape>> MakeRandomSpheres(int count, uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> posDist(-20.0f, 20.0f);
    std::uniform_real_distribution<float> radDist(0.1f, 0.4f);

    std::vector<std::shared_ptr<Shape>> shapes;
    shapes.reserve(count);
    for (int i = 0; i < count; ++i) {
        shapes.push_back(std::make_shared<Sphere>(
            Transform::Translate(Vector3f(posDist(rng), posDist(rng), posDist(rng) - 25.0f)),
            radDist(rng)));
    }
    return shapes;
}

}

TEST_CASE("BVH4 - Isolated SIMD ray-vs-4-boxes test vs scalar reference", "[bvh4][simd]") {
    WideBVHNode<4> node;
    Bounds3f b0(Point3f(-1, -1, -1), Point3f(1, 1, 1));
    Bounds3f b1(Point3f(5, 5, 5), Point3f(7, 7, 7));
    Bounds3f b2(Point3f(-10, 0, -20), Point3f(-5, 2, -15));

    node.SetChildBox(0, b0);
    node.SetChildBox(1, b1);
    node.SetChildBox(2, b2);
    node.SetEmptyChild(3);
    node.activeChildCount = 3;

    std::vector<Ray> testRays = {
        Ray(Point3f(0, 0, 10), Vector3f(0, 0, -1)),
        Ray(Point3f(6, 6, 20), Vector3f(0, 0, -1)),
        Ray(Point3f(-7.5f, 1.0f, 0), Vector3f(0, 0, -1)),
        Ray(Point3f(100, 100, 100), Vector3f(0, 0, -1)),
        Ray(Point3f(0, 0, 0), Vector3f(1, 0, 0)),
        Ray(Point3f(0, 0, -10), Vector3f(0, 0, 1)),
        Ray(Point3f(-20, -20, -20), Normalize(Vector3f(1, 1, 1))),
        Ray(Point3f(6, 6, 6), Vector3f(0, 1, 0))
    };

    for (const auto& ray : testRays) {
        Vector3f invDir(1.0f / (std::abs(ray.d.x) < 1e-8f ? 1e-8f : ray.d.x),
                        1.0f / (std::abs(ray.d.y) < 1e-8f ? 1e-8f : ray.d.y),
                        1.0f / (std::abs(ray.d.z) < 1e-8f ? 1e-8f : ray.d.z));
        int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};

        float simdTEnter[4];
        int simdMask = Intersect4(node, ray, invDir, dirIsNeg, ray.tMax, simdTEnter);

        REQUIRE((simdMask & (1 << 3)) == 0);

        for (int lane = 0; lane < 3; ++lane) {
            Bounds3f box = (lane == 0) ? b0 : (lane == 1) ? b1 : b2;
            float scalarTHit = 0.0f;
            bool scalarHit = ScalarBoxIntersect(box, ray, invDir, dirIsNeg, ray.tMax, &scalarTHit);
            bool simdHit = (simdMask & (1 << lane)) != 0;

            REQUIRE(simdHit == scalarHit);
            if (simdHit) {
                REQUIRE(simdTEnter[lane] == Approx(scalarTHit).margin(1e-4f));
            }
        }
    }
}

TEST_CASE("BVH4 - Parity with binary BVH on random scenes", "[bvh4]") {
    auto shapes = MakeRandomSpheres(3000, 42);

    BVH binaryBvh(shapes, 4, BVH::SplitMethod::SAH);
    BVH4 wideBvh(shapes, 4);

    REQUIRE(wideBvh.NodeCount() > 0);
    REQUIRE(wideBvh.NodeCount() < binaryBvh.NodeCount());

    std::mt19937 rng(777);
    std::uniform_real_distribution<float> dist(-15.0f, 15.0f);

    for (int i = 0; i < 500; ++i) {
        Ray ray(Point3f(dist(rng), dist(rng), 10.0f),
                Normalize(Vector3f(dist(rng) * 0.5f, dist(rng) * 0.5f, -1.0f)));

        SurfaceInteraction isectBinary, isectWide;
        bool hitBinary = binaryBvh.Intersect(ray, &isectBinary);
        bool hitWide = wideBvh.Intersect(ray, &isectWide);

        REQUIRE(hitBinary == hitWide);
        if (hitBinary) {
            REQUIRE(isectWide.t == Approx(isectBinary.t).margin(1e-4f));
            REQUIRE(isectWide.p.x == Approx(isectBinary.p.x).margin(1e-4f));
            REQUIRE(isectWide.p.y == Approx(isectBinary.p.y).margin(1e-4f));
            REQUIRE(isectWide.p.z == Approx(isectBinary.p.z).margin(1e-4f));
        }
    }
}

TEST_CASE("BVH4 - Collapse quality diagnostic", "[bvh4][diagnostic]") {
    auto shapes = MakeRandomSpheres(5000, 1234);
    BVH4 wideBvh(shapes, 4);

    float avgChildren = wideBvh.AverageChildrenPerNode();

    REQUIRE(avgChildren >= 2.5f);
    REQUIRE(avgChildren <= 4.0f);
}

TEST_CASE("BVH4 - Shadow ray IntersectP parity", "[bvh4]") {
    auto shapes = MakeRandomSpheres(2000, 9876);
    BVH4 wideBvh(shapes, 4);

    std::mt19937 rng(555);
    std::uniform_real_distribution<float> dist(-20.0f, 20.0f);

    for (int i = 0; i < 300; ++i) {
        Ray ray(Point3f(dist(rng), dist(rng), 10.0f),
                Normalize(Vector3f(dist(rng), dist(rng), -1.0f)));

        SurfaceInteraction isect;
        bool hit = wideBvh.Intersect(ray, &isect);
        bool hitP = wideBvh.IntersectP(ray);

        REQUIRE(hit == hitP);
    }
}

TEST_CASE("BVH4 - Edge cases: empty, 1 shape, and small scenes", "[bvh4][edge]") {
    SECTION("Empty BVH4") {
        BVH4 empty(std::vector<std::shared_ptr<Shape>>{});
        Ray r(Point3f(0, 0, 0), Vector3f(0, 0, 1));
        SurfaceInteraction isect;
        REQUIRE_FALSE(empty.Intersect(r, &isect));
        REQUIRE_FALSE(empty.IntersectP(r));
        REQUIRE(empty.NodeCount() == 0);
    }

    SECTION("Single shape BVH4") {
        auto s = std::make_shared<Sphere>(Transform::Identity(), 1.0f);
        BVH4 single({s});
        Ray r(Point3f(0, 0, -5), Vector3f(0, 0, 1));
        SurfaceInteraction isect;
        REQUIRE(single.Intersect(r, &isect));
        REQUIRE(isect.t == Approx(4.0f).margin(1e-4f));
    }

    SECTION("Two shapes BVH4") {
        auto s1 = std::make_shared<Sphere>(Transform::Translate(Vector3f(-5, 0, 0)), 1.0f);
        auto s2 = std::make_shared<Sphere>(Transform::Translate(Vector3f(5, 0, 0)), 1.0f);
        BVH4 two({s1, s2});

        Ray r1(Point3f(-5, 0, -5), Vector3f(0, 0, 1));
        Ray r2(Point3f(5, 0, -5), Vector3f(0, 0, 1));
        SurfaceInteraction isect;
        REQUIRE(two.Intersect(r1, &isect));
        REQUIRE(two.Intersect(r2, &isect));
    }
}
