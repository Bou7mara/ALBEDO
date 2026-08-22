#include "rt/scene/showcase.h"
#include "rt/scene/mesh_generators.h"
#include "rt/shapes/sphere.h"
#include "rt/shapes/triangle.h"
#include "rt/shapes/quad.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/dielectric.h"
#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/emissive.h"
#include <numbers>
#include <cmath>
#include <memory>
#include <vector>

namespace rt {

// ===========================
// 1. OG Sphere Showcase Scene
// ===========================
ShowcaseSetup CreateSphereShowcaseScene(int width, int height, int spp) {
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

    auto matLambertian   = std::make_shared<Lambertian>(Vector3f(0.6f, 0.35f, 0.3f));
    auto matGlass        = std::make_shared<Dielectric>(1.5f);
    auto matFrostedGlass = std::make_shared<Microfacet>(Microfacet::MakeDielectricMicrofacet(0.15f, 1.5f));
    auto matMirrorMetal  = std::make_shared<Metal>(Vector3f(0.9f, 0.9f, 0.92f));
    auto matGold         = std::make_shared<Microfacet>(Microfacet::MakeConductorMicrofacet(0.04f, eta_gold, k_gold));
    auto matCopper       = std::make_shared<Microfacet>(Microfacet::MakeConductorMicrofacet(0.35f, eta_copper, k_copper));

    auto groundMaterial  = std::make_shared<Lambertian>(Vector3f(0.05f, 0.05f, 0.05f));
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, -100.0f, -1.0f)), 100.0f, groundMaterial));

    std::vector<std::shared_ptr<BSDF>> heroMaterials = {
        matLambertian, matGlass, matFrostedGlass, matMirrorMetal, matGold, matCopper
    };

    float arcRadius = 2.2f;
    float centerZ = -1.8f;
    float startAngle = -50.0f * (std::numbers::pi_v<float> / 180.0f);
    float endAngle   =  50.0f * (std::numbers::pi_v<float> / 180.0f);

    for (size_t i = 0; i < heroMaterials.size(); ++i) {
        float t = static_cast<float>(i) / static_cast<float>(heroMaterials.size() - 1);
        float angle = startAngle + t * (endAngle - startAngle);
        float x = arcRadius * std::sin(angle);
        float z = centerZ + arcRadius * std::cos(angle);

        scene.Add(std::make_shared<Sphere>(
            Transform::Translate(Vector3f(x, 0.35f, z)), 0.35f, heroMaterials[i]));
    }

    auto keyLightMat  = std::make_shared<Emissive>(Vector3f(14.4f, 10.4f, 6.4f));
    auto fillLightMat = std::make_shared<Emissive>(Vector3f(4.0f, 5.6f, 9.6f));
    auto rimLightMat  = std::make_shared<Emissive>(Vector3f(12.8f, 12.8f, 12.8f));

    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(-4.5f, 4.0f, 2.0f)), 0.30f, keyLightMat));
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(4.5f, 3.2f, 3.0f)), 0.48f, fillLightMat));
    scene.Add(std::make_shared<Sphere>(
        Transform::Translate(Vector3f(0.0f, 3.5f, -4.5f)), 0.25f, rimLightMat));

    scene.Build();

    return ShowcaseSetup(std::move(scene), camera, width, height, spp, 50);
}

// =================
// 2. "The Gem Room"
// =================
namespace {

    void AddTiledFloor(Scene& scene, float extent, int numTiles,
                       std::shared_ptr<BSDF> matLight, std::shared_ptr<BSDF> matDark) {
        float tileSize = (2.0f * extent) / numTiles;
        float startX = -extent;
        float startZ = -extent;

        for (int row = 0; row < numTiles; ++row) {
            for (int col = 0; col < numTiles; ++col) {
                float minX = startX + col * tileSize;
                float maxX = minX + tileSize;
                float minZ = startZ + row * tileSize;
                float maxZ = minZ + tileSize;

                auto mesh = std::make_shared<TriangleMesh>();
                mesh->positions = {
                    Point3f(minX, 0.0f, minZ),
                    Point3f(maxX, 0.0f, minZ),
                    Point3f(maxX, 0.0f, maxZ),
                    Point3f(minX, 0.0f, maxZ)
                };
                mesh->indices = { 0, 1, 2, 0, 2, 3 };

                auto material = ((row + col) % 2 == 0) ? matLight : matDark;
                auto faces = MakeTriangleMesh(mesh, material);
                for (auto& f : faces) scene.Add(f);
            }
        }
    }

    void AddDiamond(Scene& scene, const Point3f& center, float radius, float height,
                    std::shared_ptr<BSDF> glass) {
        auto mesh = std::make_shared<TriangleMesh>();

        float girdleRadius = radius;
        float tableRadius  = radius * 0.72f;

        float deltaR   = girdleRadius - tableRadius;
        float yGirdle  = 0.0f;
        float yTable   = yGirdle + deltaR * 1.15f;
        float yCulet   = yGirdle - (height - (yTable - yGirdle));

        constexpr int numFacets = 16;

        for (int i = 0; i < numFacets; ++i) {
            float angle = i * (2.0f * std::numbers::pi_v<float> / numFacets);
            mesh->positions.emplace_back(tableRadius * std::cos(angle), yTable, tableRadius * std::sin(angle));
        }

        for (int i = 0; i < numFacets; ++i) {
            float angle = (i + 0.5f) * (2.0f * std::numbers::pi_v<float> / numFacets);
            mesh->positions.emplace_back(girdleRadius * std::cos(angle), yGirdle, girdleRadius * std::sin(angle));
        }

        mesh->positions.emplace_back(0.0f, yCulet, 0.0f);
        int culetIdx = static_cast<int>(mesh->positions.size()) - 1;

        // Winding order
        for (int i = 1; i < numFacets - 1; ++i) {
            mesh->indices.push_back(0);
            mesh->indices.push_back(i + 1);
            mesh->indices.push_back(i);
        }

        for (int i = 0; i < numFacets; ++i) {
            int t1 = i;
            int t2 = (i + 1) % numFacets;
            int g1 = numFacets + i;
            int g2 = numFacets + ((i == 0) ? numFacets - 1 : i - 1);

            mesh->indices.push_back(t1);
            mesh->indices.push_back(t2);
            mesh->indices.push_back(g1);

            mesh->indices.push_back(t1);
            mesh->indices.push_back(g1);
            mesh->indices.push_back(g2);
        }

        for (int i = 0; i < numFacets; ++i) {
            int g1 = numFacets + i;
            int g2 = numFacets + (i + 1) % numFacets;

            mesh->indices.push_back(g1);
            mesh->indices.push_back(g2);
            mesh->indices.push_back(culetIdx);
        }

        float culetOffsetY = -yCulet;
        for (auto& p : mesh->positions) {
            p = Point3f(p.x + center.x, p.y + center.y + culetOffsetY, p.z + center.z);
        }

        auto faces = MakeTriangleMesh(mesh, glass);
        for (auto& f : faces) scene.Add(f);
    }

    void AddFacetedMirror(Scene& scene, const Point3f& center, float radius, float height,
                          std::shared_ptr<BSDF> mirror) {
        auto mesh = std::make_shared<TriangleMesh>();
        float halfH = height * 0.5f;
        float cos30 = std::cos(std::numbers::pi_v<float> / 6.0f);
        float sin30 = 0.5f;

        mesh->positions = {
            Point3f( 0.0f,            -halfH,  radius),
            Point3f( radius * cos30,  -halfH, -radius * sin30),
            Point3f(-radius * cos30,  -halfH, -radius * sin30),

            Point3f( 0.0f,             halfH,  radius),
            Point3f( radius * cos30,   halfH, -radius * sin30),
            Point3f(-radius * cos30,   halfH, -radius * sin30),
        };

        mesh->indices = {
            0, 2, 1,
            3, 4, 5,

            0, 1, 4,   0, 4, 3,
            1, 2, 5,   1, 5, 4,
            2, 0, 3,   2, 3, 5,
        };

        for (auto& p : mesh->positions) {
            p = Point3f(p.x + center.x, p.y + center.y + halfH, p.z + center.z);
        }

        auto faces = MakeTriangleMesh(mesh, mirror);
        for (auto& f : faces) scene.Add(f);
    }

    void AddQuadLight(Scene& scene, const Point3f& center, float halfWidth, float halfDepth,
                      std::shared_ptr<BSDF> emissive) {
        Point3f p0(center.x - halfWidth, center.y, center.z - halfDepth);
        Vector3f e1(2.0f * halfWidth, 0.0f, 0.0f);
        Vector3f e2(0.0f, 0.0f, 2.0f * halfDepth);
        scene.Add(std::make_shared<Quad>(p0, e1, e2, emissive));
    }

}

ShowcaseSetup CreateCornellBoxShowcaseScene(int width, int height, int spp) {
    PerspectiveCamera camera(
        Point3f(0.0f, 1.0f, 3.4f),
        Point3f(0.0f, 1.0f, 0.0f),
        Vector3f(0.0f, 1.0f, 0.0f),
        40.0f,
        width, height
    );

    Scene scene;

    auto whiteMat = std::make_shared<Lambertian>(Vector3f(0.73f, 0.73f, 0.73f));
    auto redMat   = std::make_shared<Lambertian>(Vector3f(0.65f, 0.05f, 0.05f));
    auto greenMat = std::make_shared<Lambertian>(Vector3f(0.12f, 0.45f, 0.15f));
    auto lightMat = std::make_shared<Emissive>(Vector3f(15.0f, 15.0f, 15.0f));
    auto glassMat = std::make_shared<Dielectric>(1.5f);
    auto metalMat = std::make_shared<Metal>(Vector3f(0.95f, 0.93f, 0.88f));

    // 5-quad box walls
    // Floor (y = 0)
    scene.Add(std::make_shared<Quad>(Point3f(-1.0f, 0.0f, 0.0f), Vector3f(2.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, -2.0f), whiteMat));
    // Ceiling (y = 2)
    scene.Add(std::make_shared<Quad>(Point3f(-1.0f, 2.0f, -2.0f), Vector3f(2.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 2.0f), whiteMat));
    // Back wall (z = -2)
    scene.Add(std::make_shared<Quad>(Point3f(-1.0f, 0.0f, -2.0f), Vector3f(2.0f, 0.0f, 0.0f), Vector3f(0.0f, 2.0f, 0.0f), whiteMat));
    // Left wall (x = -1, red)
    scene.Add(std::make_shared<Quad>(Point3f(-1.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, -2.0f), Vector3f(0.0f, 2.0f, 0.0f), redMat));
    // Right wall (x = 1, green)
    scene.Add(std::make_shared<Quad>(Point3f(1.0f, 0.0f, -2.0f), Vector3f(0.0f, 0.0f, 2.0f), Vector3f(0.0f, 2.0f, 0.0f), greenMat));

    // Ceiling light quad
    scene.Add(std::make_shared<Quad>(Point3f(-0.3f, 1.99f, -1.3f), Vector3f(0.6f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.6f), lightMat));

    // Inner hero objects
    scene.Add(std::make_shared<Sphere>(Transform::Translate(Vector3f(-0.4f, 0.4f, -1.2f)), 0.4f, glassMat));
    scene.Add(std::make_shared<Sphere>(Transform::Translate(Vector3f(0.4f, 0.4f, -0.7f)), 0.4f, metalMat));

    scene.Build();

    return ShowcaseSetup(std::move(scene), camera, width, height, spp, 50);
}

ShowcaseSetup CreateGemRoomShowcaseScene(int width, int height, int spp) {
    PerspectiveCamera camera(
        Point3f(0.0f, 1.6f, 4.6f),
        Point3f(0.0f, 0.35f, -1.0f),
        Vector3f(0.0f, 1.0f, 0.0f),
        35.0f,
        width, height
    );

    Scene scene;

    const Vector3f eta_gold(0.143f, 0.375f, 1.442f), k_gold(3.983f, 2.386f, 1.603f);
    const Vector3f eta_copper(0.200f, 0.924f, 1.102f), k_copper(3.907f, 2.618f, 2.239f);

    auto matLambertian   = std::make_shared<Lambertian>(Vector3f(0.6f, 0.35f, 0.3f));
    auto matGlass        = std::make_shared<Dielectric>(1.5f);
    auto matDiamond      = std::make_shared<Dielectric>(2.42f, Vector3f(1.0f, 1.0f, 1.0f), 0.005f);
    auto matRuby         = std::make_shared<Dielectric>(1.76f, Vector3f(0.85f, 0.05f, 0.12f));
    auto matSapphire     = std::make_shared<Dielectric>(2.75f, Vector3f(0.12f, 0.38f, 0.88f));
    auto matFrostedGlass = std::make_shared<Microfacet>(Microfacet::MakeDielectricMicrofacet(0.15f, 1.5f));
    auto matGold         = std::make_shared<Microfacet>(Microfacet::MakeConductorMicrofacet(0.04f, eta_gold, k_gold));
    auto matCopper       = std::make_shared<Microfacet>(Microfacet::MakeConductorMicrofacet(0.35f, eta_copper, k_copper));
    auto matMirrorMetal  = std::make_shared<Metal>(Vector3f(0.9f, 0.9f, 0.92f));

    auto tileDark  = std::make_shared<Lambertian>(Vector3f(0.04f, 0.04f, 0.05f));
    auto tileLight = std::make_shared<Lambertian>(Vector3f(0.22f, 0.22f, 0.24f));
    AddTiledFloor(scene, 12.0f, 24, tileLight, tileDark);

    std::vector<std::shared_ptr<BSDF>> heroMaterials = {
        matLambertian, matGlass, matFrostedGlass, matGold, matCopper
    };

    float arcRadius = 2.2f;
    float centerZ = -1.8f;
    float startAngle = -50.0f * (std::numbers::pi_v<float> / 180.0f);
    float endAngle   =  50.0f * (std::numbers::pi_v<float> / 180.0f);
    int arcSlots = 6;

    for (int i = 0; i < arcSlots; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(arcSlots - 1);
        float angle = startAngle + t * (endAngle - startAngle);
        float x = arcRadius * std::sin(angle);
        float z = centerZ + arcRadius * std::cos(angle);

        if (i == 3) {
            AddFacetedMirror(scene, Point3f(x, 0.0f, z), 0.35f, 0.60f, matMirrorMetal);
        }
        else {
            int heroIdx = i < 3 ? i : i - 1;
            scene.Add(std::make_shared<Sphere>(
                Transform::Translate(Vector3f(x, 0.35f, z)), 0.35f, heroMaterials[heroIdx]));
        }
    }

    AddDiamond(scene, Point3f(0.0f, 0.0f, -2.5f), 0.6f, 0.9f, matDiamond);

    auto keyLightMat   = std::make_shared<Emissive>(Vector3f(8.5f, 6.0f, 3.8f));
    auto fillLightMat  = std::make_shared<Emissive>(Vector3f(2.8f, 3.8f, 6.2f));
    auto rimLightMat   = std::make_shared<Emissive>(Vector3f(7.0f, 7.0f, 7.0f));
    auto topSoftboxMat = std::make_shared<Emissive>(Vector3f(4.2f, 4.2f, 4.5f));

    //AddQuadLight(scene, Point3f(-4.5f, 4.2f, 2.0f), 0.6f, 0.6f, keyLightMat);
    //AddQuadLight(scene, Point3f(4.5f, 3.5f, 2.5f), 0.7f, 0.7f, fillLightMat);
    AddQuadLight(scene, Point3f(0.0f, 3.8f, -4.5f), 0.5f, 0.5f, rimLightMat);
    AddQuadLight(scene, Point3f(0.0f, 5.5f, -1.2f), 1.5f, 0.9f, topSoftboxMat);

    scene.Build();

    return ShowcaseSetup(std::move(scene), camera, width, height, spp, 50);
}

ShowcaseSetup CreateShowcaseScene(int width, int height, int spp) {
    return CreateGemRoomShowcaseScene(width, height, spp);
}
}