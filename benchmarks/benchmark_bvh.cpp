#include "rt/accel/bvh.h"
#include "rt/accel/bvh4.h"
#include "rt/accel/bvh8.h"
#include "rt/cam/perspective_camera.h"
#include "rt/shapes/sphere.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

using namespace rt;

namespace {

constexpr int kImageWidth = 400;
constexpr int kImageHeight = 200;

std::vector<std::shared_ptr<Shape>> GenerateRandomSpheres(int n, uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> xDist(-10.0f, 10.0f);
    std::uniform_real_distribution<float> yDist(-5.0f, 5.0f);
    std::uniform_real_distribution<float> zDist(-60.0f, -5.0f);
    std::uniform_real_distribution<float> rDist(0.1f, 0.3f);

    std::vector<std::shared_ptr<Shape>> shapes;
    shapes.reserve(n);
    for (int i = 0; i < n; ++i) {
        Vector3f center(xDist(rng), yDist(rng), zDist(rng));
        shapes.push_back(std::make_shared<Sphere>(Transform::Translate(center), rDist(rng)));
    }
    return shapes;
}

bool LinearIntersect(const std::vector<std::shared_ptr<Shape>>& shapes,
                      const Ray& ray, SurfaceInteraction* isect) {
    bool hit = false;
    for (const auto& s : shapes) if (s->Intersect(ray, isect)) hit = true;
    return hit;
}

template <typename Func>
double TimeMs(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

PerspectiveCamera MakeBenchmarkCamera() {
    return PerspectiveCamera(Point3f(0, 0, 0), Point3f(0, 0, -1), Vector3f(0, 1, 0),
                              90.0f, kImageWidth, kImageHeight);
}

std::vector<Ray> GenerateFrameRays(const PerspectiveCamera& camera) {
    std::vector<Ray> rays;
    rays.reserve(kImageWidth * kImageHeight);
    for (int y = 0; y < kImageHeight; ++y) {
        for (int x = 0; x < kImageWidth; ++x) {
            rays.push_back(camera.GenerateRay(
                CameraSample{Point2f(x + 0.5f, y + 0.5f)}));
        }
    }
    return rays;
}

template <typename IntersectFn>
std::pair<double, int> RunFramePass(const std::vector<Ray>& rays, IntersectFn&& intersectFn) {
    int hitCount = 0;
    double ms = TimeMs([&] {
        for (const Ray& templateRay : rays) {
            Ray r = templateRay;
            SurfaceInteraction isect;
            if (intersectFn(r, &isect)) ++hitCount;
        }
    });
    return {ms, hitCount};
}

std::string NextResultsPath(const std::string& directory = "benchmark_results") {
    std::filesystem::create_directories(directory);
    int n = 1;
    std::string path;
    do {
        path = directory + "/bvh_benchmark" + std::to_string(n) + ".csv";
        ++n;
    } while (std::filesystem::exists(path));
    return path;
}

}

int main() {
    const std::vector<int> smallSphereCounts = {1, 10, 100, 1000};
    const std::vector<int> largeSphereCounts = {10000, 50000, 100000, 200000};

    PerspectiveCamera camera = MakeBenchmarkCamera();
    std::vector<Ray> frameRays = GenerateFrameRays(camera);

    std::string resultsPath = NextResultsPath();
    std::ofstream csv(resultsPath);
    csv << "section,spheres,linear_ms,build_serial_ms,build_parallel_ms,build_speedup,query_ms\n";

    std::cout << "========================================================================================\n";
    std::cout << " Part 1: Traversal Performance vs Linear Scan (400x200 = 80,000 rays)\n";
    std::cout << "========================================================================================\n";
    std::cout << "N spheres | Linear (ms) | BVH Query (ms) | Query Speedup\n";

    for (int n : smallSphereCounts) {
        auto shapes = GenerateRandomSpheres(n, 12345);

        auto [linearMs, linearHits] = RunFramePass(frameRays,
            [&](const Ray& r, SurfaceInteraction* isect) { return LinearIntersect(shapes, r, isect); });

        std::unique_ptr<BVH> bvh;
        bvh = std::make_unique<BVH>(shapes, 4, BVH::SplitMethod::SAH, 0);
        auto [queryMs, bvhHits] = RunFramePass(frameRays,
            [&](const Ray& r, SurfaceInteraction* isect) { return bvh->Intersect(r, isect); });

        if (linearHits != bvhHits) {
            std::cerr << "*** CORRECTNESS MISMATCH at N=" << n << " ***\n";
            continue;
        }

        double speedup = linearMs / queryMs;
        std::cout << n << " | " << linearMs << " ms | " << queryMs << " ms | " << speedup << "x\n";
        csv << "linear_vs_bvh," << n << "," << linearMs << ",0,0,0," << queryMs << "\n";
    }

    std::cout << "\n========================================================================================\n";
    std::cout << " Part 2: Parallel BVH Construction Scaling (Serial vs Parallel SAH Build)\n";
    std::cout << "========================================================================================\n";
    std::cout << "N spheres | Serial Build (ms) | Parallel Build (ms) | Build Speedup | BVH Query (ms)\n";

    for (int n : largeSphereCounts) {
        auto shapes = GenerateRandomSpheres(n, 54321 + n);

        std::unique_ptr<BVH> bvhSerial;
        double serialBuildMs = TimeMs([&] {
            bvhSerial = std::make_unique<BVH>(shapes, 4, BVH::SplitMethod::SAH, 1);
        });

        std::unique_ptr<BVH> bvhParallel;
        double parallelBuildMs = TimeMs([&] {
            bvhParallel = std::make_unique<BVH>(shapes, 4, BVH::SplitMethod::SAH, 0);
        });

        auto [queryMs, hits] = RunFramePass(frameRays,
            [&](const Ray& r, SurfaceInteraction* isect) { return bvhParallel->Intersect(r, isect); });

        double buildSpeedup = serialBuildMs / parallelBuildMs;
        std::cout << n << " | " << serialBuildMs << " ms | " << parallelBuildMs << " ms | "
                  << buildSpeedup << "x | " << queryMs << " ms\n";

        csv << "parallel_build," << n << ",0," << serialBuildMs << "," << parallelBuildMs << ","
            << buildSpeedup << "," << queryMs << "\n";
    }

    std::cout << "\n========================================================================================\n";
    std::cout << " Part 3: 3-Way Wide BVH Traversal (Binary vs. BVH4 vs. BVH8, 80,000 rays)\n";
    std::cout << "========================================================================================\n";
    std::cout << "N spheres | Binary Query | BVH4 Query (Speedup) | BVH8 Query (Speedup) | Nodes (Bin / 4 / 8)\n";

    for (int n : largeSphereCounts) {
        auto shapes = GenerateRandomSpheres(n, 9999 + n);

        BVH binaryBvh(shapes, 4, BVH::SplitMethod::SAH);
        BVH4 wideBvh4(shapes, 4);
        BVH8 wideBvh8(shapes, 4);

        auto [binaryQueryMs, binaryHits] = RunFramePass(frameRays,
            [&](const Ray& r, SurfaceInteraction* isect) { return binaryBvh.Intersect(r, isect); });

        auto [bvh4QueryMs, bvh4Hits] = RunFramePass(frameRays,
            [&](const Ray& r, SurfaceInteraction* isect) { return wideBvh4.Intersect(r, isect); });

        auto [bvh8QueryMs, bvh8Hits] = RunFramePass(frameRays,
            [&](const Ray& r, SurfaceInteraction* isect) { return wideBvh8.Intersect(r, isect); });

        if (binaryHits != bvh4Hits || binaryHits != bvh8Hits) {
            std::cerr << "*** CORRECTNESS MISMATCH in Wide BVH at N=" << n << " ***\n";
            continue;
        }

        double speedup4 = binaryQueryMs / bvh4QueryMs;
        double speedup8 = binaryQueryMs / bvh8QueryMs;

        std::cout << n << " | " << binaryQueryMs << " ms | "
                  << bvh4QueryMs << " ms (" << speedup4 << "x) | "
                  << bvh8QueryMs << " ms (" << speedup8 << "x) | "
                  << binaryBvh.NodeCount() << " / " << wideBvh4.NodeCount() << " / " << wideBvh8.NodeCount() << "\n";

        csv << "bvh_3way," << n << ",0," << binaryQueryMs << "," << bvh4QueryMs << "," << speedup4 << "," << bvh8QueryMs << "\n";
    }

    std::cout << "\nResults written to " << resultsPath << "\n";
    return 0;
}

