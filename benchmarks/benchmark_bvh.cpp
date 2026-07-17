#include "rt/accel/bvh.h"
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
    const std::vector<int> sphereCounts = {10, 50, 100, 500, 1000, 5000};
    PerspectiveCamera camera = MakeBenchmarkCamera();
    std::vector<Ray> frameRays = GenerateFrameRays(camera);

    std::string resultsPath = NextResultsPath();
    std::ofstream csv(resultsPath);
    csv << "spheres,linear_ms,bvh_build_mid_ms,bvh_query_mid_ms,speedup_mid,"
           "bvh_build_sah_ms,bvh_query_sah_ms,speedup_sah\n";

    std::cout << "N spheres | Linear (ms) | Mid build/query (ms) | Mid speedup | "
                 "SAH build/query (ms) | SAH speedup\n";

    for (int n : sphereCounts) {
        auto shapes = GenerateRandomSpheres(n, /*seed=*/12345);

        auto [linearMs, linearHits] = RunFramePass(frameRays,
            [&](const Ray& r, SurfaceInteraction* isect) { return LinearIntersect(shapes, r, isect); });

        double buildMidMs = 0.0;
        std::unique_ptr<BVH> bvhMid;
        buildMidMs = TimeMs([&] { bvhMid = std::make_unique<BVH>(shapes, 4, BVH::SplitMethod::Midpoint); });
        auto [queryMidMs, midHits] = RunFramePass(frameRays,
            [&](const Ray& r, SurfaceInteraction* isect) { return bvhMid->Intersect(r, isect); });

        double buildSahMs = 0.0;
        std::unique_ptr<BVH> bvhSah;
        buildSahMs = TimeMs([&] { bvhSah = std::make_unique<BVH>(shapes, 4, BVH::SplitMethod::SAH); });
        auto [querySahMs, sahHits] = RunFramePass(frameRays,
            [&](const Ray& r, SurfaceInteraction* isect) { return bvhSah->Intersect(r, isect); });

        if (linearHits != midHits || linearHits != sahHits) {
            std::cerr << "*** CORRECTNESS MISMATCH at N=" << n
                      << " (linear=" << linearHits << ", mid=" << midHits
                      << ", sah=" << sahHits << ") -- DO NOT TRUST THESE NUMBERS ***\n";
            continue;
        }

        double speedupMid = linearMs / queryMidMs;
        double speedupSah = linearMs / querySahMs;

        std::cout << n << " | " << linearMs << " | "
                  << buildMidMs << "/" << queryMidMs << " | " << speedupMid << "x | "
                  << buildSahMs << "/" << querySahMs << " | " << speedupSah << "x\n";

        csv << n << "," << linearMs << "," << buildMidMs << "," << queryMidMs << "," << speedupMid << ","
            << buildSahMs << "," << querySahMs << "," << speedupSah << "\n";
    }

    std::cout << "Results written to " << resultsPath << "\n";
    return 0;
}
