#include <fstream>
#include <thread>

#include "rt/core/point2.h"
#include "rt/core/rng.h"
#include "rt/cam/perspective_camera.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/emissive.h"
#include "rt/scene/scene.h"
#include "rt/scene/showcase.h"
#include "rt/io/ppm_writer.h"
#include "rt/io/png_writer.h"
#include "rt/core/progress.h"




using namespace rt;

namespace {
	constexpr int kRRStartDepth = 3;
	constexpr float kRRProbabilityMinThreshold = 0.5f;
    constexpr float kRRProbabilityMaximumThreshold = 0.95f;
}

[[nodiscard]] constexpr float MaxChannel(const Vector3f& v) {
	return std::max(v.x, std::max(v.y, v.z));
}

inline float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
    float f = nf * fPdf;
    float g = ng * gPdf;
    return (f * f) / (f * f + g * g);
}

Vector3f ALBEDO(Ray r, const Scene& scene, RNG& rng, int maxDepth) {
    Vector3f L(0.0f, 0.0f, 0.0f);
    Vector3f throughput(1.0f, 1.0f, 1.0f);
    bool specularBounce = true;
    float prevBsdfPdf = 0.0f;

    for (int depth = 0; depth < maxDepth; ++depth) {
        SurfaceInteraction isect;
        if (!scene.Intersect(r, &isect)) {
            Vector3f unitDir = Normalize(r.d);
            float t = 0.5f * (unitDir.y + 1.0f);
            Vector3f bg = 0.15f * ((1.0f - t) * Vector3f(0.8f, 0.8f, 0.9f) + t * Vector3f(0.4f, 0.5f, 0.7f));
            L += throughput * bg;
            break;
        }

        const BSDF* bsdf = isect.shape->GetBSDF();
        if (!bsdf) {
            Vector3f n = Vector3f(isect.n);
            L += throughput * Vector3f(0.5f * (n.x + 1.0f), 0.5f * (n.y + 1.0f), 0.5f * (n.z + 1.0f));
            break;
        }

        Vector3f emitted = bsdf->Le(isect.wo, Vector3f(isect.n));
        if (emitted.x > 0.0f || emitted.y > 0.0f || emitted.z > 0.0f) {
            if (specularBounce || scene.Lights().empty()) {
                L += throughput * emitted;
            } else {
                float pmf = scene.LightPmf(isect.shape);
                float lightPdf = isect.shape->Pdf(r.o, r.d) * pmf;
                float weight = PowerHeuristic(1, prevBsdfPdf, 1, lightPdf);
                L += throughput * emitted * weight;
            }
        }

        if (!scene.Lights().empty()) {
            int lightIdx = -1;
            float pmf = 0.0f;
            const Light* light = scene.SampleLight(rng.Uniform1D(), &lightIdx, &pmf);
            
            if (light && pmf > 0.0f) {
                Light::LiSample lightSample = light->Sample_Li(isect.p, rng.Uniform2D());
                if (lightSample.pdf > 0.0f) {
                    constexpr float kEpsilon = 1e-4f;
                    Vector3f offsetNormal = (Dot(lightSample.wi, isect.n) > 0.0f) ? Vector3f(isect.n) : -Vector3f(isect.n);
                    Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;
                    
                    Ray shadowRay(offsetOrigin, lightSample.wi, lightSample.dist - 2.0f * kEpsilon);
                    if (!scene.IntersectP(shadowRay)) {
                        Vector3f f = bsdf->f(isect.wo, lightSample.wi, Vector3f(isect.n));
                        if (f.x > 0.0f || f.y > 0.0f || f.z > 0.0f) {
                            float bsdfPdf = bsdf->Pdf(isect.wo, lightSample.wi, Vector3f(isect.n));
                            if (bsdfPdf > 0.0f) {
                                float lPdf = lightSample.pdf * pmf;
                                float weight = PowerHeuristic(1, lPdf, 1, bsdfPdf);
                                float cosTheta = AbsDot(lightSample.wi, isect.n);
                                L += throughput * f * lightSample.Li * cosTheta * weight / lPdf;
                            }
                        }
                    }
                }
            }
        }

        Vector3f wi;
        float pdf;
        Vector3f f = bsdf->Sample_f(isect.wo, Vector3f(isect.n), rng.Uniform2D(), &wi, &pdf);
        
        if (pdf <= 0.0f || (f.x == 0.0f && f.y == 0.0f && f.z == 0.0f)) break;

        float cosTheta = AbsDot(wi, isect.n);
        throughput = throughput * f * cosTheta / pdf;
        
        prevBsdfPdf = bsdf->Pdf(isect.wo, wi, Vector3f(isect.n));
        specularBounce = (prevBsdfPdf == 0.0f);

		if (depth >= kRRStartDepth) {
			const float q = std::clamp(MaxChannel(throughput), kRRProbabilityMinThreshold, kRRProbabilityMaximumThreshold);
			if (rng.Uniform1D() > q) break;
			throughput = throughput / q;
		}

        constexpr float kEpsilon = 1e-4f;
        Vector3f offsetNormal = (Dot(wi, isect.n) > 0.0f) ? Vector3f(isect.n) : -Vector3f(isect.n);
        Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;
        
        r = Ray(offsetOrigin, wi);
    }
    return L;
}

int main(int argc, char* argv[]) {
    int width = 2560;
    int height = 1600;
    int spp = 5000;
    std::string sceneChoice = "gem";

    if (argc > 1) width = std::atoi(argv[1]);
    if (argc > 2) height = std::atoi(argv[2]);
    if (argc > 3) spp = std::atoi(argv[3]);
    if (argc > 4) sceneChoice = argv[4];

    ShowcaseSetup setup = (sceneChoice == "sphere") ? CreateSphereShowcaseScene(width, height, spp) :
                          (sceneChoice == "gem")    ? CreateGemRoomShowcaseScene(width, height, spp) :
                                                      CreateCornellBoxShowcaseScene(width, height, spp);

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::cout << "Rendering Scene (" << sceneChoice << "): " << setup.imageWidth << "x" << setup.imageHeight
              << " image at " << setup.samplesPerPixel << " spp using " << numThreads << " threads...\n";

    constexpr int kTileSize = 16;
    int numTilesX = (setup.imageWidth + kTileSize - 1) / kTileSize;
    int numTilesY = (setup.imageHeight + kTileSize - 1) / kTileSize;
    int totalTiles = numTilesX * numTilesY;

    std::vector<Vector3f> framebuffer(setup.imageWidth * setup.imageHeight);
    std::atomic<int> nextTile{0};
    ProgressReporter progress(totalTiles);

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (unsigned int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            RNG rng(1337 + t * 997);
            int tileIdx = 0;
            while ((tileIdx = nextTile.fetch_add(1, std::memory_order_relaxed)) < totalTiles) {
                int tileY = tileIdx / numTilesX;
                int tileX = tileIdx % numTilesX;

                int startX = tileX * kTileSize;
                int endX = std::min(startX + kTileSize, setup.imageWidth);
                int startY = tileY * kTileSize;
                int endY = std::min(startY + kTileSize, setup.imageHeight);

                for (int y = startY; y < endY; ++y) {
                    for (int x = startX; x < endX; ++x) {
                        Vector3f colorSum(0.0f, 0.0f, 0.0f);
                        for (int s = 0; s < setup.samplesPerPixel; ++s) {
                            Point2f jitter = rng.Uniform2D();
                            CameraSample sample{Point2f(x + jitter.x, y + jitter.y)};
                            Ray ray = setup.camera.GenerateRay(sample);
                            colorSum += ALBEDO(ray, setup.scene, rng, setup.maxDepth);
                        }
                        Vector3f avgColor = colorSum / static_cast<float>(setup.samplesPerPixel);
                        framebuffer[y * setup.imageWidth + x] = avgColor;
                    }
                }
                progress.Advance();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    progress.Finish();

    std::string ppmPath = NextImagePath("images", ".ppm");
    std::ofstream out(ppmPath);
    if (!out) {
        std::cerr << "Failed to open " << ppmPath << " for writing!\n";
        return 1;
    }

    WritePPMHeader(out, setup.imageWidth, setup.imageHeight);
    for (int y = 0; y < setup.imageHeight; ++y) {
        for (int x = 0; x < setup.imageWidth; ++x) {
            const Vector3f& color = framebuffer[y * setup.imageWidth + x];
            WritePixel(out, color.x, color.y, color.z);
        }
    }
    std::cout << "PPM output written to " << ppmPath << "\n";

    std::string pngPath = NextImagePath("images_png", ".png");
    if (WritePNG(pngPath, setup.imageWidth, setup.imageHeight, framebuffer)) {
        std::cout << "PNG output written to " << pngPath << "\n";
    } else {
        std::cerr << "Failed to write PNG output to " << pngPath << "!\n";
    }

    std::cout << "Rendering completed.\n";
    return 0;
}
