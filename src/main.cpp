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
#include "rt/core/progress.h"

#include <fstream>
#include <memory>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

using namespace rt;

// Calculates Multiple Importance Sampling (MIS) power heuristic weight (with exponent 2)
// Combines sampling techniques (e.g. BSDF sampling vs direct light sampling) to reduce render variance
inline float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
    float f = nf * fPdf;
    float g = ng * gPdf;
    return (f * f) / (f * f + g * g);
}

// Monte Carlo path tracer ray evaluation function
// Traces a camera or bounce ray through the scene up to maxDepth bounces
Vector3f RayColor(Ray r, const Scene& scene, RNG& rng, int maxDepth) {
    Vector3f L(0.0f, 0.0f, 0.0f);            // Accumulated radiance color
    Vector3f throughput(1.0f, 1.0f, 1.0f);   // Path throughput attenuation color
    bool specularBounce = true;              // Flag tracking if previous bounce was ideal specular
    float prevBsdfPdf = 0.0f;                // BSDF PDF from previous bounce step

    for (int depth = 0; depth < maxDepth; ++depth) {
        SurfaceInteraction isect;
        // If ray misses scene geometry, add background sky gradient color and break loop
        if (!scene.Intersect(r, &isect)) {
            Vector3f unitDir = Normalize(r.d);
            float t = 0.5f * (unitDir.y + 1.0f);
            Vector3f bg = 0.15f * ((1.0f - t) * Vector3f(0.8f, 0.8f, 0.9f) + t * Vector3f(0.4f, 0.5f, 0.7f));
            L += throughput * bg;
            break;
        }

        const BSDF* bsdf = isect.shape->GetBSDF();
        // Default fallback normal visualization if shape has no material BSDF attached
        if (!bsdf) {
            Vector3f n = Vector3f(isect.n);
            L += throughput * Vector3f(0.5f * (n.x + 1.0f), 0.5f * (n.y + 1.0f), 0.5f * (n.z + 1.0f));
            break;
        }

        // Add direct emission from light surfaces hit by ray
        Vector3f emitted = bsdf->Le(isect.wo, Vector3f(isect.n));
        if (emitted.x > 0.0f || emitted.y > 0.0f || emitted.z > 0.0f) {
            if (specularBounce || scene.Lights().empty()) {
                L += throughput * emitted;
            } else {
                // Calculate MIS weight for light hit using previous BSDF sampling PDF
                float pmf = scene.LightPmf(isect.shape);
                float lightPdf = isect.shape->Pdf(r.o, r.d) * pmf;
                float weight = PowerHeuristic(1, prevBsdfPdf, 1, lightPdf);
                L += throughput * emitted * weight;
            }
        }

        // Direct light sampling (Next Event Estimation with MIS)
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
                    
                    // Trace shadow ray to verify light source is unoccluded
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

        // Sample next bounce direction from material BSDF
        Vector3f wi;
        float pdf;
        Vector3f f = bsdf->Sample_f(isect.wo, Vector3f(isect.n), rng.Uniform2D(), &wi, &pdf);
        
        if (pdf <= 0.0f || (f.x == 0.0f && f.y == 0.0f && f.z == 0.0f)) break;

        // Update path throughput color
        float cosTheta = AbsDot(wi, isect.n);
        throughput = throughput * f * cosTheta / pdf;
        
        prevBsdfPdf = bsdf->Pdf(isect.wo, wi, Vector3f(isect.n));
        specularBounce = (prevBsdfPdf == 0.0f);

        // Offset ray origin slightly along normal to prevent self-intersection self-shadowing acne
        constexpr float kEpsilon = 1e-4f;
        Vector3f offsetNormal = (Dot(wi, isect.n) > 0.0f) ? Vector3f(isect.n) : -Vector3f(isect.n);
        Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;
        
        r = Ray(offsetOrigin, wi);
    }
    return L;
}

// Program main entry point: Prepares showcase scene, launches rendering worker threads, writes output image
int main() {
    ShowcaseSetup setup = CreateShowcaseScene(1600, 1000, 500);

    // Determine number of CPU core worker threads available
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::cout << "Rendering Showcase Scene: " << setup.imageWidth << "x" << setup.imageHeight
              << " image at " << setup.samplesPerPixel << " spp using " << numThreads << " threads...\n";

    std::vector<Vector3f> framebuffer(setup.imageWidth * setup.imageHeight);
    std::atomic<int> nextRow{0};
    ProgressReporter progress(setup.imageHeight);

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    // Launch multithreaded rendering worker pool
    for (unsigned int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            RNG rng(1337 + t * 997);
            int y = 0;
            // Lock-free row fetching loop
            while ((y = nextRow.fetch_add(1, std::memory_order_relaxed)) < setup.imageHeight) {
                for (int x = 0; x < setup.imageWidth; ++x) {
                    Vector3f colorSum(0.0f, 0.0f, 0.0f);
                    // Accumulate pixel samples
                    for (int s = 0; s < setup.samplesPerPixel; ++s) {
                        Point2f jitter = rng.Uniform2D();
                        CameraSample sample{Point2f(x + jitter.x, y + jitter.y)};
                        Ray ray = setup.camera.GenerateRay(sample);
                        colorSum += RayColor(ray, setup.scene, rng, setup.maxDepth);
                    }
                    Vector3f avgColor = colorSum / static_cast<float>(setup.samplesPerPixel);
                    framebuffer[y * setup.imageWidth + x] = avgColor;
                }
                progress.Advance();
            }
        });
    }

    // Wait for all worker threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    progress.Finish();

    // Write rendered image buffer to PPM file
    std::string outputPath = NextImagePath();
    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "Failed to open " << outputPath << " for writing!\n";
        return 1;
    }

    WritePPMHeader(out, setup.imageWidth, setup.imageHeight);
    for (int y = 0; y < setup.imageHeight; ++y) {
        for (int x = 0; x < setup.imageWidth; ++x) {
            const Vector3f& color = framebuffer[y * setup.imageWidth + x];
            WritePixel(out, color.x, color.y, color.z);
        }
    }

    std::cout << "Rendering completed. Output written to " << outputPath << "\n";
    return 0;
}

