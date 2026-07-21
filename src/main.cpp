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

Vector3f RayColor(const Ray& r, const Scene& scene, RNG& rng, int depth) {
    if (depth <= 0) return Vector3f(0.0f, 0.0f, 0.0f);

    SurfaceInteraction isect;
    if (scene.Intersect(r, &isect)) {
        const BSDF* bsdf = isect.shape->GetBSDF();
        if (bsdf) {
            Vector3f emitted = bsdf->Le(isect.wo, Vector3f(isect.n));

            Vector3f wi;
            float pdf;
            Vector3f f = bsdf->Sample_f(isect.wo, Vector3f(isect.n),
                                         rng.Uniform2D(), &wi, &pdf);
            if (pdf <= 0.0f) return emitted;

            float cosTheta = AbsDot(wi, isect.n);

            constexpr float kEpsilon = 1e-4f;

            Vector3f offsetNormal = (Dot(wi, isect.n) > 0.0f) ? Vector3f(isect.n) : -Vector3f(isect.n);
            Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;
            Ray scattered(offsetOrigin, wi);

            Vector3f Li = RayColor(scattered, scene, rng, depth - 1);
            return emitted + f * cosTheta * Li / pdf;
        }
        Vector3f n = Vector3f(isect.n);
        return Vector3f(0.5f * (n.x + 1.0f), 0.5f * (n.y + 1.0f),
                         0.5f * (n.z + 1.0f));
    }

    Vector3f unitDir = Normalize(r.d);
    float t = 0.5f * (unitDir.y + 1.0f);
    return (1.0f - t) * Vector3f(1.0f, 1.0f, 1.0f)
         + t * Vector3f(0.5f, 0.7f, 1.0f);
}

int main() {
    const int imageWidth = 800;
    const int imageHeight = 400;
    const int samplesPerPixel = 500;
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

    Scene scene;
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, 0.0f, -1.0f)), 0.5f, sphereMaterial));

    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(1.1f, 0.0f, -1.0f)), 0.5f, metalMaterial));

    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, -100.5f, -1.0f)), 100.0f, groundMaterial));

    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(-0.6f, 0.8f, -1.2f)), 0.3f, lightMaterial));

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
