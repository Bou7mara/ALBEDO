#include "rt/core/point2.h"
#include "rt/core/rng.h"
#include "rt/cam/perspective_camera.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/emissive.h"
#include "rt/scene/scene.h"
#include "rt/io/ppm_writer.h"

#include <fstream>
#include <memory>
#include <iostream>

using namespace rt;

inline float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
    float f = nf * fPdf;
    float g = ng * gPdf;
    return (f * f) / (f * f + g * g);
}

Vector3f RayColor(Ray r, const Scene& scene, RNG& rng, int maxDepth) {
    Vector3f L(0.0f, 0.0f, 0.0f);
    Vector3f throughput(1.0f, 1.0f, 1.0f);
    bool specularBounce = true;
    float prevBsdfPdf = 0.0f;

    for (int depth = 0; depth < maxDepth; ++depth) {
        SurfaceInteraction isect;
        if (!scene.Intersect(r, &isect)) {
            Vector3f unitDir = Normalize(r.d);
            float t = 0.5f * (unitDir.y + 1.0f);
            Vector3f bg = (1.0f - t) * Vector3f(1.0f, 1.0f, 1.0f) + t * Vector3f(0.5f, 0.7f, 1.0f);
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
                float lightPdf = isect.shape->Pdf(r.o, r.d) / scene.Lights().size();
                float weight = PowerHeuristic(1, prevBsdfPdf, 1, lightPdf);
                L += throughput * emitted * weight;
            }
        }

        if (!scene.Lights().empty()) {
            int numLights = scene.Lights().size();
            int lightIdx = std::min(static_cast<int>(rng.Uniform1D() * numLights), numLights - 1);
            const auto& light = scene.Lights()[lightIdx];
            
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
                            float lPdf = lightSample.pdf / numLights;
                            float weight = PowerHeuristic(1, lPdf, 1, bsdfPdf);
                            float cosTheta = AbsDot(lightSample.wi, isect.n);
                            L += throughput * f * lightSample.Li * cosTheta * weight / lPdf;
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

        constexpr float kEpsilon = 1e-4f;
        Vector3f offsetNormal = (Dot(wi, isect.n) > 0.0f) ? Vector3f(isect.n) : -Vector3f(isect.n);
        Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;
        
        r = Ray(offsetOrigin, wi);
    }
    return L;
}

int main() {
    const int imageWidth = 800;
    const int imageHeight = 400;
    const int samplesPerPixel = 100; // Lowered to 100 to show off MIS!
    const int maxDepth = 50;

    std::cout << "Rendering a " << imageWidth << "x" << imageHeight
              << " image at " << samplesPerPixel << " spp...\n";

    PerspectiveCamera camera(
        Point3f(0.0f, 0.0f, 0.0f),
        Point3f(0.0f, 0.0f, -1.0f),
        Vector3f(0.0f, 1.0f, 0.0f),
        90.0f,
        imageWidth, imageHeight
    );

    auto sphereMaterial = std::make_shared<Lambertian>(Vector3f(0.7f, 0.7f, 0.7f));
    auto groundMaterial = std::make_shared<Lambertian>(Vector3f(0.2f, 0.15f, 0.1f));
    auto metalMaterial = std::make_shared<Metal>(Vector3f(0.8f, 0.8f, 0.9f));
    auto lightMaterial = std::make_shared<Emissive>(Vector3f(8.0f, 7.2f, 5.6f));
    auto lightMaterial2 = std::make_shared<Emissive>(Vector3f(2.0f, 5.0f, 10.0f));

    Scene scene;
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, 0.0f, -1.0f)), 0.5f, sphereMaterial));

    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(1.1f, 0.0f, -1.0f)), 0.5f, metalMaterial));

    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, -100.5f, -1.0f)), 100.0f, groundMaterial));

    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(-0.6f, 0.8f, -1.2f)), 0.3f, lightMaterial));

    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(1.5f, 1.2f, -2.0f)), 0.2f, lightMaterial2));

    scene.Build();

    std::string outputPath = NextImagePath();
    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "Failed to open " << outputPath << " for writing!\n";
        return 1;
    }

    WritePPMHeader(out, imageWidth, imageHeight);
    RNG rng;

    for (int y = 0; y < imageHeight; ++y) {
        for (int x = 0; x < imageWidth; ++x) {
            Vector3f colorSum(0.0f, 0.0f, 0.0f);
            for (int s = 0; s < samplesPerPixel; ++s) {
                Point2f jitter = rng.Uniform2D();
                CameraSample sample{Point2f(x + jitter.x, y + jitter.y)};
                Ray ray = camera.GenerateRay(sample);
                colorSum += RayColor(ray, scene, rng, maxDepth);
            }
            Vector3f avgColor = colorSum / static_cast<float>(samplesPerPixel);
            WritePixel(out, avgColor.x, avgColor.y, avgColor.z);
        }
    }

    std::cout << "Rendering completed. Output written to " << outputPath << "\n";
    return 0;
}
