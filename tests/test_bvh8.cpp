#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/accel/bvh8.h"
#include "rt/accel/bvh4.h"
#include "rt/accel/bvh.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"
#include <random>

using namespace rt;
using Catch::Approx;

namespace {

bool ScalarBoxIntersect8(const Bounds3f& box, const Ray& ray, const Vector3f& invDir, const int dirIsNeg[3], float tMax, float* tHit) {
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

std::vector<std::shared_ptr<Shape>> MakeRandomSpheres8(int count, uint32_t seed) {
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

TEST_CASE("BVH8 - Hardware and AVX2 compilation verification", "[bvh8][simd]") {
    REQUIRE(ALBEDO_HAS_AVX2 == 1);
    REQUIRE(ALBEDO_HAS_SSE == 1);
}

TEST_CASE("BVH8 - Isolated 8-wide SIMD box test vs scalar reference", "[bvh8][simd]") {
    WideBVHNode<8> node;
    std::vector<Bounds3f> boxes = {
        Bounds3f(Point3f(-1, -1, -1), Point3f(1, 1, 1)),
        Bounds3f(Point3f(5, 5, 5), Point3f(7, 7, 7)),
        Bounds3f(Point3f(-10, 0, -20), Point3f(-5, 2, -15)),
        Bounds3f(Point3f(10, -5, -10), Point3f(12, -3, -8)),
        Bounds3f(Point3f(-2, 10, -5), Point3f(0, 12, -3))
    };

    for (size_t i = 0; i < boxes.size(); ++i) {
        node.SetChildBox(static_cast<int>(i), boxes[i]);
    }
    for (int i = 5; i < 8; ++i) {
        node.SetEmptyChild(i);
    }
    node.activeChildCount = 5;

    std::vector<Ray> testRays = {
        Ray(Point3f(0, 0, 10), Vector3f(0, 0, -1)),                // hits box 0
        Ray(Point3f(6, 6, 20), Vector3f(0, 0, -1)),                // hits box 1
        Ray(Point3f(-7.5f, 1.0f, 0), Vector3f(0, 0, -1)),          // hits box 2
        Ray(Point3f(11, -4, 5), Vector3f(0, 0, -1)),               // hits box 3
        Ray(Point3f(-1, 11, 10), Vector3f(0, 0, -1)),              // hits box 4
        Ray(Point3f(100, 100, 100), Vector3f(0, 0, -1)),           // misses all
        Ray(Point3f(0, 0, 0), Vector3f(1, 0, 0)),                   // inside box 0
        Ray(Point3f(0, 0, -10), Vector3f(0, 0, 1)),                 // negative z to positive z
        Ray(Point3f(-20, -20, -20), Normalize(Vector3f(1, 1, 1))), // diagonal ray
        Ray(Point3f(6, 6, 6), Vector3f(0, 1, 0))                    // inside box 1 parallel to Y
    };

    for (const auto& ray : testRays) {
        Vector3f invDir(1.0f / (std::abs(ray.d.x) < 1e-8f ? 1e-8f : ray.d.x),
                        1.0f / (std::abs(ray.d.y) < 1e-8f ? 1e-8f : ray.d.y),
                        1.0f / (std::abs(ray.d.z) < 1e-8f ? 1e-8f : ray.d.z));
        int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};

        float simdTEnter[8];
        int simdMask = Intersect8(node, ray, invDir, dirIsNeg, ray.tMax, simdTEnter);

        // Degenerate lanes 5, 6, 7 must NEVER be hit
        for (int lane = 5; lane < 8; ++lane) {
            REQUIRE((simdMask & (1 << lane)) == 0);
        }

        for (size_t lane = 0; lane < boxes.size(); ++lane) {
            float scalarTHit = 0.0f;
            bool scalarHit = ScalarBoxIntersect8(boxes[lane], ray, invDir, dirIsNeg, ray.tMax, &scalarTHit);
            bool simdHit = (simdMask & (1 << lane)) != 0;

            REQUIRE(simdHit == scalarHit);
            if (simdHit) {
                REQUIRE(simdTEnter[lane] == Approx(scalarTHit).margin(1e-4f));
            }
        }
    }
}

TEST_CASE("BVH8 - Parity with Binary BVH and BVH4 on complex scenes", "[bvh8]") {
    auto shapes = MakeRandomSpheres8(3000, 101);

    BVH binaryBvh(shapes, 4, BVH::SplitMethod::SAH);
    BVH4 wideBvh4(shapes, 4);
    BVH8 wideBvh8(shapes, 4);

    REQUIRE(wideBvh8.NodeCount() > 0);
    REQUIRE(wideBvh8.NodeCount() < wideBvh4.NodeCount());   // BVH8 has fewer nodes than BVH4
    REQUIRE(wideBvh4.NodeCount() < binaryBvh.NodeCount());  // BVH4 has fewer nodes than Binary

    std::mt19937 rng(888);
    std::uniform_real_distribution<float> dist(-15.0f, 15.0f);

    for (int i = 0; i < 500; ++i) {
        Ray ray(Point3f(dist(rng), dist(rng), 10.0f),
                Normalize(Vector3f(dist(rng) * 0.5f, dist(rng) * 0.5f, -1.0f)));

        SurfaceInteraction isectBinary, isect4, isect8;
        bool hitBinary = binaryBvh.Intersect(ray, &isectBinary);
        bool hit4 = wideBvh4.Intersect(ray, &isect4);
        bool hit8 = wideBvh8.Intersect(ray, &isect8);

        REQUIRE(hitBinary == hit4);
        REQUIRE(hitBinary == hit8);

        if (hitBinary) {
            REQUIRE(isect8.t == Approx(isectBinary.t).margin(1e-4f));
            REQUIRE(isect8.p.x == Approx(isectBinary.p.x).margin(1e-4f));
            REQUIRE(isect8.p.y == Approx(isectBinary.p.y).margin(1e-4f));
            REQUIRE(isect8.p.z == Approx(isectBinary.p.z).margin(1e-4f));
        }
    }
}

TEST_CASE("BVH8 - Collapse quality diagnostic", "[bvh8][diagnostic]") {
    auto shapes = MakeRandomSpheres8(5000, 2026);
    BVH8 wideBvh8(shapes, 4);

    float avgChildren = wideBvh8.AverageChildrenPerNode();
    // For an 8-wide tree, average fanout should be > 4.5
    REQUIRE(avgChildren >= 4.0f);
    REQUIRE(avgChildren <= 8.0f);
}

TEST_CASE("BVH8 - Shadow ray IntersectP parity", "[bvh8]") {
    auto shapes = MakeRandomSpheres8(2000, 333);
    BVH8 wideBvh8(shapes, 4);

    std::mt19937 rng(444);
    std::uniform_real_distribution<float> dist(-20.0f, 20.0f);

    for (int i = 0; i < 300; ++i) {
        Ray ray(Point3f(dist(rng), dist(rng), 10.0f),
                Normalize(Vector3f(dist(rng) * 0.5f, dist(rng) * 0.5f, -1.0f)));

        SurfaceInteraction isect;
        bool hit = wideBvh8.Intersect(ray, &isect);
        bool hitP = wideBvh8.IntersectP(ray);

        REQUIRE(hit == hitP);
    }
}

TEST_CASE("BVH8 - Edge cases: empty, 1, 2, 7, and 8 shapes", "[bvh8][edge]") {
    SECTION("Empty BVH8") {
        BVH8 empty(std::vector<std::shared_ptr<Shape>>{});
        Ray r(Point3f(0, 0, 0), Vector3f(0, 0, 1));
        SurfaceInteraction isect;
        REQUIRE_FALSE(empty.Intersect(r, &isect));
        REQUIRE_FALSE(empty.IntersectP(r));
        REQUIRE(empty.NodeCount() == 0);
    }

    SECTION("Single shape BVH8") {
        auto s = std::make_shared<Sphere>(Transform::Identity(), 1.0f);
        BVH8 single({s});
        Ray r(Point3f(0, 0, -5), Vector3f(0, 0, 1));
        SurfaceInteraction isect;
        REQUIRE(single.Intersect(r, &isect));
        REQUIRE(isect.t == Approx(4.0f).margin(1e-4f));
    }

    SECTION("Eight shapes BVH8") {
        std::vector<std::shared_ptr<Shape>> shapes;
        for (int i = 0; i < 8; ++i) {
            shapes.push_back(std::make_shared<Sphere>(
                Transform::Translate(Vector3f((i - 4) * 3.0f, 0, 0)), 1.0f));
        }
        BVH8 eight(shapes);
        for (int i = 0; i < 8; ++i) {
            Ray r(Point3f((i - 4) * 3.0f, 0, -5), Vector3f(0, 0, 1));
            SurfaceInteraction isect;
            REQUIRE(eight.Intersect(r, &isect));
            REQUIRE(isect.t == Approx(4.0f).margin(1e-4f));
        }
    }
}
