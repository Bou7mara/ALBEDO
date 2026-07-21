#include "rt/scene/showcase.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/dielectric.h"
#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/emissive.h"
#include <numbers>
#include <cmath>

namespace rt {

ShowcaseSetup CreateShowcaseScene(int width, int height, int spp) {
    PerspectiveCamera camera(
        Point3f(0.0f, 1.2f, 4.0f),
        Point3f(0.0f, 0.3f, -1.0f),
        Vector3f(0.0f, 1.0f, 0.0f),
        35.0f,
        width, height
    );

    Scene scene;

    const Vector3f eta_gold(0.143f, 0.375f, 1.442f), k_gold(3.983f, 2.386f, 1.603f);
    const Vector3f eta_copper(0.200f, 0.924f, 1.102f), k_copper(3.907f, 2.618f, 2.239f);

    auto matLambertian = std::make_shared<Lambertian>(Vector3f(0.6f, 0.35f, 0.3f));
    auto matGlass = std::make_shared<Dielectric>(1.5f);
    auto matFrostedGlass = std::make_shared<Microfacet>(Microfacet::MakeDielectricMicrofacet(0.15f, 1.5f));
    auto matMirrorMetal = std::make_shared<Metal>(Vector3f(0.9f, 0.9f, 0.92f));
    auto matGold = std::make_shared<Microfacet>(Microfacet::MakeConductorMicrofacet(0.04f, eta_gold, k_gold));
    auto matCopper = std::make_shared<Microfacet>(Microfacet::MakeConductorMicrofacet(0.35f, eta_copper, k_copper));

    auto groundMaterial = std::make_shared<Lambertian>(Vector3f(0.05f, 0.05f, 0.05f));
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, -100.4f, -1.0f)), 100.0f, groundMaterial));

    std::vector<std::shared_ptr<BSDF>> heroMaterials = {
        matLambertian, matGlass, matFrostedGlass, matMirrorMetal, matGold, matCopper
    };

    float arcRadius = 2.2f;
    float centerZ = -1.8f;
    float startAngle = -50.0f * (std::numbers::pi_v<float> / 180.0f);
    float endAngle = 50.0f * (std::numbers::pi_v<float> / 180.0f);

    for (size_t i = 0; i < heroMaterials.size(); ++i) {
        float t = static_cast<float>(i) / static_cast<float>(heroMaterials.size() - 1);
        float angle = startAngle + t * (endAngle - startAngle);
        float x = arcRadius * std::sin(angle);
        float z = centerZ + arcRadius * std::cos(angle);
        
        scene.Add(std::make_shared<Sphere>(
            Transform::Translate(Vector3f(x, -0.05f, z)), 0.35f, heroMaterials[i]));
    }

    auto keyLightMat = std::make_shared<Emissive>(Vector3f(14.4f, 10.4f, 6.4f));
    auto fillLightMat = std::make_shared<Emissive>(Vector3f(4.0f, 5.6f, 9.6f));
    auto rimLightMat = std::make_shared<Emissive>(Vector3f(12.8f, 12.8f, 12.8f));

    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(-2.5f, 3.0f, 1.0f)), 0.30f, keyLightMat));
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(2.5f, 2.0f, 2.0f)), 0.48f, fillLightMat));
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, 2.5f, -3.0f)), 0.25f, rimLightMat));

    scene.Build();

    return ShowcaseSetup(std::move(scene), camera, width, height, spp, 50);
}

}
