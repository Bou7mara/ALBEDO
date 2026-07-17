#include "rt/core/point2.h"
#include "rt/core/rng.h"
#include "rt/cam/perspective_camera.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
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
            Vector3f wi;
            float pdf;
            Vector3f f = bsdf->Sample_f(isect.wo, Vector3f(isect.n),
                                         rng.Uniform2D(), &wi, &pdf);
            if (pdf <= 0.0f) return Vector3f(0.0f, 0.0f, 0.0f);

            float cosTheta = AbsDot(wi, isect.n);

            // Sample_f() has just produced `wi` here — this offset can only be computed
            // once the new ray direction is known, since (as of Dielectric) the correct
            // offset side depends on it.
            constexpr float kEpsilon = 1e-4f;

            // -- wi on the outward side (reflect, diffuse scatter): nudge outward,
            //    same behavior every existing material already relies on.
            // -- wi on the inward side (Dielectric's refract branch): nudge inward,
            //    so the new ray starts on the far side of the surface it just
            //    crossed, instead of re-landing inside the shape it came from.
            Vector3f offsetNormal = (Dot(wi, isect.n) > 0.0f) ? Vector3f(isect.n) : -Vector3f(isect.n);
            Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;
            Ray scattered(offsetOrigin, wi);

            Vector3f Li = RayColor(scattered, scene, rng, depth - 1);
            return f * cosTheta * Li / pdf;
        }
        // No BSDF attached: fall back to normal-visualization debug
        // shading, so any future un-textured shape stays diagnosable
        // rather than silently rendering black.
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
    const int imageWidth = 400;
    const int imageHeight = 200;
    const int samplesPerPixel = 100;
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

    Scene scene;
    // Middle sphere (bottom edge sits at y = -0.5)
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, 0.0f, -1.0f)), 0.5f, sphereMaterial));

    // Metal sphere (bottom edge sits at y = -0.5)
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(1.1f, 0.0f, -1.0f)), 0.5f, metalMaterial));

    // Huge ground sphere (top edge sits at y = -0.5, touching the middle sphere tangentially)
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, -100.5f, -1.0f)), 100.0f, groundMaterial));

    scene.Build();

    std::string outputPath = NextImagePath();
    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "Failed to open " << outputPath << " for writing!\n";
        return 1;
    }

    WritePPMHeader(out, imageWidth, imageHeight);
    RNG rng;   // single-threaded for now -- see rng.h note on per-thread RNGs

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
