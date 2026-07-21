#include <catch2/catch_test_macros.hpp>
#include "rt/scene/scene.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"
#include "rt/core/rng.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace rt;

TEST_CASE("Concurrent Ray Intersection & Scene Access", "[multithreading]") {
    Scene scene;
    auto mat = std::make_shared<Lambertian>(Vector3f(0.5f, 0.5f, 0.5f));
    scene.Add(std::make_shared<Sphere>(Transform::Translate(Vector3f(0, 0, -2)), 1.0f, mat));
    scene.Build();

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::atomic<int> hits{0};
    std::vector<std::thread> threads;

    for (unsigned int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&scene, &hits, t]() {
            RNG rng(t * 12345);
            for (int i = 0; i < 1000; ++i) {
                Point2f u = rng.Uniform2D();
                Ray r(Point3f(0, 0, 0), Normalize(Vector3f(u.x - 0.5f, u.y - 0.5f, -1.0f)));
                SurfaceInteraction isect;
                if (scene.Intersect(r, &isect)) {
                    hits.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    REQUIRE(hits.load() > 0);
}
