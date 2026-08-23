#include "benchmarks/as_variants/as_registry.h"
#include "benchmarks/fixtures/fixtures.h"
#include "benchmarks/ray_sets/ray_generators.h"
#include "benchmarks/report/console_reporter.h"
#include "benchmarks/report/csv_reporter.h"
#include "rt/cam/perspective_camera.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>

using namespace rt;
using namespace rt::bench;

namespace {

constexpr int kImageWidth = 400;
constexpr int kImageHeight = 200;
constexpr int kNumRuns = 5;

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

void WarmUp(const BLAS& as, const std::vector<Ray>& rays) {
    size_t count = std::min<size_t>(rays.size(), 10000);
    for (size_t i = 0; i < count; ++i) {
        Ray r = rays[i];
        SurfaceInteraction isect;
        as.Intersect(r, &isect);
    }
}

int RunRayPass(const BLAS& as, const std::vector<Ray>& rays) {
    int hits = 0;
    for (const auto& templateRay : rays) {
        Ray r = templateRay;
        SurfaceInteraction isect;
        if (as.Intersect(r, &isect)) {
            ++hits;
        }
    }
    return hits;
}

} // namespace

int main() {
    std::cout << "=================================================================================================================\n";
    std::cout << "                              ALBEDO ACCELERATION STRUCTURE BENCHMARK SUITE                                      \n";
    std::cout << "=================================================================================================================\n";
    std::cout << " Runs per benchmark: " << kNumRuns << " (reporting median, min, max)\n";
    std::cout << " Cache warm-up: Enabled (10,000 pre-pass rays)\n";
    std::cout << " Ray workloads: Primary (Coherent) & Secondary (Incoherent diffuse)\n";

    CSVReporter csv;
    PerspectiveCamera camera = MakeBenchmarkCamera();
    std::vector<Ray> primaryRays = GeneratePrimaryRays(kImageWidth, kImageHeight, camera);

    std::vector<BenchmarkScene> scenes = {
        GenerateUniformScene(20000, 1001),
        GenerateProblemCaseScene(2000, 2002),
        GenerateInstancedScene(1000, 200, 3003),
        GenerateLargeScene(100000, 4004)
    };

    for (const auto& scene : scenes) {
        auto registry = GetStandardASRegistry(scene.isInstanced);

        // First build a baseline AS to generate secondary rays if needed
        auto referenceAS = registry[0].buildFn(scene.shapes);
        std::vector<Ray> secondaryRays = GenerateSecondaryRays(primaryRays, *referenceAS, 2, 777);

        std::vector<std::pair<RaySetType, const std::vector<Ray>*>> workloads = {
            {RaySetType::Primary, &primaryRays},
            {RaySetType::SecondaryIncoherent, &secondaryRays}
        };

        for (const auto& [rayType, raysPtr] : workloads) {
            const auto& rays = *raysPtr;
            ConsoleReporter::PrintHeader(scene.name, scene.primitiveCount,
                                        RaySetTypeName(rayType), rays.size());

            double baselineQueryMs = 0.0;
            int referenceHits = -1;

            for (size_t asIdx = 0; asIdx < registry.size(); ++asIdx) {
                const auto& asDesc = registry[asIdx];

                // 1. Measure Build Time (5 runs)
                std::vector<double> buildTimes;
                buildTimes.reserve(kNumRuns);
                std::unique_ptr<BLAS> builtAS;

                for (int r = 0; r < kNumRuns; ++r) {
                    std::unique_ptr<BLAS> tempAS;
                    double ms = TimeMs([&] {
                        tempAS = asDesc.buildFn(scene.shapes);
                    });
                    buildTimes.push_back(ms);
                    if (r == 0) builtAS = std::move(tempAS);
                }
                std::sort(buildTimes.begin(), buildTimes.end());
                double buildMedianMs = buildTimes[kNumRuns / 2];
                double buildMinMs = buildTimes.front();
                double buildMaxMs = buildTimes.back();

                // 2. Warm up CPU caches
                WarmUp(*builtAS, rays);

                // 3. Measure Query Time (5 runs)
                std::vector<double> queryTimes;
                queryTimes.reserve(kNumRuns);
                int hitCount = 0;

                for (int r = 0; r < kNumRuns; ++r) {
                    int hits = 0;
                    double ms = TimeMs([&] {
                        hits = RunRayPass(*builtAS, rays);
                    });
                    queryTimes.push_back(ms);
                    hitCount = hits;
                }
                std::sort(queryTimes.begin(), queryTimes.end());
                double queryMedianMs = queryTimes[kNumRuns / 2];
                double queryMinMs = queryTimes.front();
                double queryMaxMs = queryTimes.back();

                if (asIdx == 0) {
                    baselineQueryMs = queryMedianMs;
                    referenceHits = hitCount;
                } else if (referenceHits != hitCount && !scene.isInstanced) {
                    std::cerr << "*** WARNING: Correctness hit mismatch in " << asDesc.name
                              << ": expected " << referenceHits << ", got " << hitCount << " ***\n";
                }

                double mraysPerSec = (rays.size() / (queryMedianMs * 1e-3)) * 1e-6;

                // 4. Log per-repetition records for statistical analysis
                for (int r = 0; r < kNumRuns; ++r) {
                    double repMrayPerSec = (rays.size() / (queryTimes[r] * 1e-3)) * 1e-6;
                    RepetitionRecord repRecord{
                        scene.name,
                        asDesc.name,
                        RaySetTypeName(rayType),
                        r + 1,
                        scene.primitiveCount,
                        rays.size(),
                        buildTimes[r],
                        queryTimes[r],
                        repMrayPerSec,
                        asDesc.memoryFn(*builtAS),
                        asDesc.nodeCountFn(*builtAS),
                        asDesc.fanoutFn(*builtAS),
                        hitCount
                    };
                    csv.AddRepetitionRecord(repRecord);
                }

                BenchmarkRecord record{
                    scene.name,
                    asDesc.name,
                    RaySetTypeName(rayType),
                    scene.primitiveCount,
                    rays.size(),
                    buildMedianMs,
                    buildMinMs,
                    buildMaxMs,
                    queryMedianMs,
                    queryMinMs,
                    queryMaxMs,
                    mraysPerSec,
                    asDesc.memoryFn(*builtAS),
                    asDesc.nodeCountFn(*builtAS),
                    asDesc.fanoutFn(*builtAS),
                    hitCount
                };

                ConsoleReporter::PrintRow(record, baselineQueryMs);
                csv.AddSummaryRecord(record);
            }

            ConsoleReporter::PrintFooter();
        }
    }

    std::cout << "\nBenchmark complete.\n"
              << " Summary CSV output saved to: " << csv.SummaryFilePath() << "\n"
              << " Per-repetition CSV saved to: " << csv.RepsFilePath() << "\n\n";
    return 0;
}
