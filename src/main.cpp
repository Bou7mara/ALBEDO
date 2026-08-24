#include <fstream>
#include <thread>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <atomic>

#include "rt/core/point2.h"
#include "rt/core/rng.h"
#include "rt/cam/perspective_camera.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/emissive.h"
#include "rt/scene/scene.h"
#include "rt/scene/showcase.h"
#include "rt/integrator_constants.h"
#include "rt/io/ppm_writer.h"
#include "rt/io/png_writer.h"
#include "rt/core/progress.h"

#ifdef ALBEDO_ENABLE_GPU
#include "gpu_renderer.h"
#endif

using namespace rt;

#include "rt/materials/dielectric.h"
#include "rt/spectral/spectrum.h"

Vector3f ALBEDO(Ray r, const Scene& scene, RNG& rng, int maxDepth) {
    Vector3f L(0.0f, 0.0f, 0.0f);
    Vector3f throughput(1.0f, 1.0f, 1.0f);
    float spectralThroughput[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    HeroWavelengths hw;
    bool isSpectral = false;
    bool specularBounce = true;
    float prevBsdfPdf = 0.0f;
    bool spectralCompanionsCollapsed = false;

    for (int depth = 0; depth < maxDepth; ++depth) {
        SurfaceInteraction isect;
        if (!scene.Intersect(r, &isect)) {
            Vector3f unitDir = Normalize(r.d);
            float t = 0.5f * (unitDir.y + 1.0f);
            Vector3f bg = 0.15f * ((1.0f - t) * Vector3f(0.8f, 0.8f, 0.9f) + t * Vector3f(0.4f, 0.5f, 0.7f));
            if (isSpectral) {
                float spectralL[4];
                for (int i = 0; i < 4; ++i) {
                    spectralL[i] = spectralThroughput[i] * RgbToSpectrum(bg, hw.lambda[i]);
                }
                Vector3f contrib = SpectralToRgb(hw, spectralL);
                constexpr float kMaxSampleContribution = 10.0f;
                float maxC = MaxChannel(contrib);
                if (maxC > kMaxSampleContribution) {
                    contrib *= (kMaxSampleContribution / maxC);
                }
                L += contrib;
            } else {
                L += throughput * bg;
            }
            break;
        }

        const BSDF* bsdf = isect.shape->GetBSDF();
        if (!bsdf) {
            Vector3f n = Vector3f(isect.n);
            Vector3f normalCol = Vector3f(0.5f * (n.x + 1.0f), 0.5f * (n.y + 1.0f), 0.5f * (n.z + 1.0f));
            if (isSpectral) {
                float spectralL[4];
                for (int i = 0; i < 4; ++i) {
                    spectralL[i] = spectralThroughput[i] * RgbToSpectrum(normalCol, hw.lambda[i]);
                }
                Vector3f contrib = SpectralToRgb(hw, spectralL);
                constexpr float kMaxSampleContribution = 10.0f;
                float maxC = MaxChannel(contrib);
                if (maxC > kMaxSampleContribution) {
                    contrib *= (kMaxSampleContribution / maxC);
                }
                L += contrib;
            } else {
                L += throughput * normalCol;
            }
            break;
        }

        Vector3f emitted = bsdf->Le(isect.wo, Vector3f(isect.n));
        if (emitted.x > 0.0f || emitted.y > 0.0f || emitted.z > 0.0f) {
            float weight = 1.0f;
            if (!specularBounce && !scene.Lights().empty()) {
                float pmf = scene.LightPmf(isect.shape);
                float lightPdf = isect.shape->Pdf(r.o, r.d) * pmf;
                weight = PowerHeuristic(1, prevBsdfPdf, 1, lightPdf);
            }
            Vector3f contrib = emitted * weight;
            if (isSpectral) {
                float spectralL[4];
                for (int i = 0; i < 4; ++i) {
                    spectralL[i] = spectralThroughput[i] * RgbToSpectrum(contrib, hw.lambda[i]);
                }
                Vector3f finalContrib = SpectralToRgb(hw, spectralL);
                constexpr float kMaxSampleContribution = 10.0f;
                float maxC = MaxChannel(finalContrib);
                if (maxC > kMaxSampleContribution) {
                    finalContrib *= (kMaxSampleContribution / maxC);
                }
                L += finalContrib;
            } else {
                L += throughput * contrib;
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
                        Vector3f f = bsdf->f(isect.wo, lightSample.wi, Vector3f(isect.n), isect.uv);
                        if (f.x > 0.0f || f.y > 0.0f || f.z > 0.0f) {
                            float bsdfPdf = bsdf->Pdf(isect.wo, lightSample.wi, Vector3f(isect.n), isect.uv);
                            if (bsdfPdf > 0.0f) {
                                float lPdf = lightSample.pdf * pmf;
                                float weight = PowerHeuristic(1, lPdf, 1, bsdfPdf);
                                float cosTheta = AbsDot(lightSample.wi, isect.n);
                                float factor = (cosTheta * weight) / lPdf;

                                if (isSpectral) {
                                    float spectralNEE[4];
                                    for (int i = 0; i < 4; ++i) {
                                        float sF = RgbToSpectrum(f, hw.lambda[i]);
                                        float sLi = RgbToSpectrum(lightSample.Li, hw.lambda[i]);
                                        spectralNEE[i] = spectralThroughput[i] * sF * sLi * factor;
                                    }
                                    Vector3f neeContrib = SpectralToRgb(hw, spectralNEE);
                                    constexpr float kMaxSampleContribution = 10.0f;
                                    float neeMax = MaxChannel(neeContrib);
                                    if (neeMax > kMaxSampleContribution) {
                                        neeContrib = neeContrib * (kMaxSampleContribution / neeMax);
                                    }
                                    L += neeContrib;
                                } else {
                                    constexpr float kMaxSampleContribution = 10.0f;
                                    Vector3f neeContribution = throughput * f * lightSample.Li * factor;
                                    float neeMax = MaxChannel(neeContribution);
                                    if (neeMax > kMaxSampleContribution) {
                                        neeContribution = neeContribution * (kMaxSampleContribution / neeMax);
                                    }
                                    L += neeContribution;
                                }
                            }
                        }
                    }
                }
            }
        }

        Vector3f wi;
        float pdf = 0.0f;

        const Dielectric* dielectric = dynamic_cast<const Dielectric*>(bsdf);
        if (dielectric && dielectric->HasDispersion()) {
            if (!isSpectral) {
                isSpectral = true;
                hw = SampleHeroWavelengths(rng.Uniform1D());
                for (int i = 0; i < 4; ++i) {
                    spectralThroughput[i] = RgbToSpectrum(throughput, hw.lambda[i]);
                }
            }

            float throughputWeights[4];
            if (!dielectric->Sample_HeroWavelengths(isect.wo, Vector3f(isect.n), rng.Uniform2D(), hw, &wi, &pdf, throughputWeights)) {
                break;
            }

            bool collapsesThisEvent = (throughputWeights[1] == 0.0f && throughputWeights[2] == 0.0f && throughputWeights[3] == 0.0f);
            if (collapsesThisEvent && !spectralCompanionsCollapsed) {
                throughputWeights[0] *= 4.0f;
                spectralCompanionsCollapsed = true;
            }

            for (int i = 0; i < 4; ++i) {
                spectralThroughput[i] *= throughputWeights[i];
            }
            prevBsdfPdf = 0.0f;
            specularBounce = true;
        } else if (isSpectral) {
            Vector3f f = bsdf->Sample_f(isect.wo, Vector3f(isect.n), rng.Uniform2D(), &wi, &pdf, isect.uv);
            if (pdf <= 0.0f || (f.x == 0.0f && f.y == 0.0f && f.z == 0.0f)) break;

            float cosTheta = AbsDot(wi, isect.n);
            float weight = cosTheta / pdf;
            for (int i = 0; i < 4; ++i) {
                spectralThroughput[i] *= RgbToSpectrum(f, hw.lambda[i]) * weight;
            }
            prevBsdfPdf = bsdf->Pdf(isect.wo, wi, Vector3f(isect.n), isect.uv);
            specularBounce = (prevBsdfPdf == 0.0f);
        } else {
            Vector3f f = bsdf->Sample_f(isect.wo, Vector3f(isect.n), rng.Uniform2D(), &wi, &pdf, isect.uv);
            if (pdf <= 0.0f || (f.x == 0.0f && f.y == 0.0f && f.z == 0.0f)) break;

            float cosTheta = AbsDot(wi, isect.n);
            throughput = throughput * f * (cosTheta / pdf);
            constexpr float kMaxSampleContribution = 10.0f;
            float maxComponent = MaxChannel(throughput);
            if (maxComponent > kMaxSampleContribution) {
                throughput = throughput * (kMaxSampleContribution / maxComponent);
            }
            prevBsdfPdf = bsdf->Pdf(isect.wo, wi, Vector3f(isect.n), isect.uv);
            specularBounce = (prevBsdfPdf == 0.0f);
        }

        if (depth >= kRRStartDepth) {
            float maxVal = isSpectral ? std::max({spectralThroughput[0], spectralThroughput[1], spectralThroughput[2], spectralThroughput[3]})
                                      : MaxChannel(throughput);
            const float q = std::clamp(maxVal, kRRProbabilityMinimumThreshold, kRRProbabilityMaximumThreshold);
            if (rng.Uniform1D() > q) break;
            if (isSpectral) {
                for (int i = 0; i < 4; ++i) spectralThroughput[i] /= q;
            } else {
                throughput = throughput / q;
            }
        }

        constexpr float kEpsilon = 1e-4f;
        Vector3f offsetNormal = (Dot(wi, isect.n) > 0.0f) ? Vector3f(isect.n) : -Vector3f(isect.n);
        Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;

        r = Ray(offsetOrigin, wi);
    }
    return L;
}

void RenderCpu(const ShowcaseSetup& setup, std::vector<Vector3f>& framebuffer) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::cout << "Rendering on CPU using " << numThreads << " threads...\n";

    constexpr int kTileSize = 16;
    int numTilesX = (setup.imageWidth + kTileSize - 1) / kTileSize;
    int numTilesY = (setup.imageHeight + kTileSize - 1) / kTileSize;
    int totalTiles = numTilesX * numTilesY;

    framebuffer.resize(setup.imageWidth * setup.imageHeight);
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
}

int main(int argc, char* argv[]) {
    int width = 1600;
    int height = 2560;
    int spp = 500;
    std::string sceneChoice = "gem";
#ifdef ALBEDO_ENABLE_GPU
    std::string backend = "gpu";
#else
    std::string backend = "cpu";
#endif
    bool denoise = false;

    int positionalIdx = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--backend=gpu" || arg == "gpu") backend = "gpu";
        else if (arg == "--backend=cpu" || arg == "cpu") backend = "cpu";
        else if (arg == "--denoise" || arg == "denoise") denoise = true;
        else if (arg.rfind("--scene=", 0) == 0) sceneChoice = arg.substr(8);
        else if (arg.rfind("--width=", 0) == 0) width = std::stoi(arg.substr(8));
        else if (arg.rfind("--height=", 0) == 0) height = std::stoi(arg.substr(9));
        else if (arg.rfind("--spp=", 0) == 0) spp = std::stoi(arg.substr(6));
        else if (!arg.empty() && (std::isdigit(arg[0]) || arg == "gem" || arg == "sphere" || arg == "cornell")) {
            if (positionalIdx == 0 && std::isdigit(arg[0])) width = std::stoi(arg);
            else if (positionalIdx == 1 && std::isdigit(arg[0])) height = std::stoi(arg);
            else if (positionalIdx == 2 && std::isdigit(arg[0])) spp = std::stoi(arg);
            else if (positionalIdx == 3 || arg == "gem" || arg == "sphere" || arg == "cornell") sceneChoice = arg;
            ++positionalIdx;
        }
    }

    ShowcaseSetup setup = (sceneChoice == "sphere") ? CreateSphereShowcaseScene(width, height, spp) :
                          (sceneChoice == "gem")    ? CreateGemRoomShowcaseScene(width, height, spp) :
                                                      CreateCornellBoxShowcaseScene(width, height, spp);

    std::cout << "Rendering Scene (" << sceneChoice << "): " << setup.imageWidth << "x" << setup.imageHeight
              << " image at " << setup.samplesPerPixel << " spp [Backend: " << backend
              << (denoise ? ", Denoise: ON" : "") << "]...\n";

    std::vector<Vector3f> framebuffer;

    if (backend == "gpu") {
#ifdef ALBEDO_ENABLE_GPU
        try {
            rtx::RenderGpu(setup, framebuffer, denoise);
        } catch (const std::exception& e) {
            std::cerr << "GPU Render Error: " << e.what() << "\n";
            return 1;
        }
#else
        std::cerr << "Error: --backend=gpu requested but project built without ALBEDO_ENABLE_GPU.\n";
        return 1;
#endif
    } else {
        if (denoise) {
            std::cerr << "Error: --denoise is only supported with GPU backend (ALBEDO_ENABLE_GPU).\n";
            return 1;
        }
        RenderCpu(setup, framebuffer);
    }

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
