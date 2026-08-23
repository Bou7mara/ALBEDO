#pragma once
#include "rt/accel/blas.h"
#include "rt/accel/bvh.h"
#include "rt/core/transform.h"
#include "rt/shapes/instance.h"
#include "rt/shapes/sphere.h"
#include "rt/shapes/triangle.h"
#include <cmath>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace rt::bench {

struct BenchmarkScene {
    std::string name;
    std::vector<std::shared_ptr<Shape>> shapes;
    size_t primitiveCount = 0;
    bool isInstanced = false;
};

// 1. Uniform random spheres in a bounding box
inline BenchmarkScene GenerateUniformScene(int count = 20000, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> posDist(-20.0f, 20.0f);
    std::uniform_real_distribution<float> radDist(0.1f, 0.4f);

    std::vector<std::shared_ptr<Shape>> shapes;
    shapes.reserve(count);
    for (int i = 0; i < count; ++i) {
        shapes.push_back(std::make_shared<Sphere>(
            Transform::Translate(Vector3f(posDist(rng), posDist(rng), posDist(rng) - 30.0f)),
            radDist(rng)));
    }
    return {"Uniform Spheres", shapes, static_cast<size_t>(count), false};
}

// 2. Problem case: Long thin overlapping diagonal triangles mixed with tight clusters
inline BenchmarkScene GenerateProblemCaseScene(int numSlices = 2000, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> xDist(-15.0f, 15.0f);
    std::uniform_real_distribution<float> yDist(-15.0f, 15.0f);
    std::uniform_real_distribution<float> zDist(-40.0f, -10.0f);

    auto mesh = std::make_shared<TriangleMesh>();
    mesh->positions.reserve(numSlices * 3);
    mesh->indices.reserve(numSlices * 3);

    for (int i = 0; i < numSlices; ++i) {
        float x = xDist(rng);
        float y = yDist(rng);
        float z = zDist(rng);
        int baseIdx = static_cast<int>(mesh->positions.size());
        mesh->positions.push_back(Point3f(x - 10.0f, y - 10.0f, z));
        mesh->positions.push_back(Point3f(x + 10.0f, y + 10.0f, z + 2.0f));
        mesh->positions.push_back(Point3f(x, y + 0.5f, z + 1.0f));
        mesh->indices.push_back(baseIdx + 0);
        mesh->indices.push_back(baseIdx + 1);
        mesh->indices.push_back(baseIdx + 2);
    }

    std::vector<std::shared_ptr<Shape>> shapes = MakeTriangleMesh(mesh);

    // Tight clusters of small spheres inside the bounding box of the long triangles
    for (int i = 0; i < numSlices * 2; ++i) {
        shapes.push_back(std::make_shared<Sphere>(
            Transform::Translate(Vector3f(xDist(rng) * 0.4f, yDist(rng) * 0.4f, zDist(rng))),
            0.15f));
    }

    return {"Problem Case (Overlapping Diagonals)", shapes, shapes.size(), false};
}

// 3. Instanced scene: 1,000 instances of a shared base mesh
inline BenchmarkScene GenerateInstancedScene(int numInstances = 1000, int trianglesPerMesh = 200, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> posDist(-30.0f, 30.0f);
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> scaleDist(0.5f, 1.5f);

    auto baseMesh = std::make_shared<TriangleMesh>();
    baseMesh->positions.reserve(trianglesPerMesh * 3);
    baseMesh->indices.reserve(trianglesPerMesh * 3);

    for (int i = 0; i < trianglesPerMesh; ++i) {
        float theta0 = 2.0f * 3.14159265f * (i / static_cast<float>(trianglesPerMesh));
        float theta1 = 2.0f * 3.14159265f * ((i + 1) / static_cast<float>(trianglesPerMesh));
        int baseIdx = static_cast<int>(baseMesh->positions.size());
        baseMesh->positions.push_back(Point3f(0.0f, 1.0f, 0.0f));
        baseMesh->positions.push_back(Point3f(std::cos(theta0), 0.0f, std::sin(theta0)));
        baseMesh->positions.push_back(Point3f(std::cos(theta1), 0.0f, std::sin(theta1)));
        baseMesh->indices.push_back(baseIdx + 0);
        baseMesh->indices.push_back(baseIdx + 1);
        baseMesh->indices.push_back(baseIdx + 2);
    }

    auto baseShapes = MakeTriangleMesh(baseMesh);
    auto sharedBLAS = std::make_shared<BVH>(baseShapes, 4, BVH::SplitMethod::SAH);

    std::vector<std::shared_ptr<Shape>> instances;
    instances.reserve(numInstances);
    for (int i = 0; i < numInstances; ++i) {
        float s = scaleDist(rng);
        Transform t = Transform::Translate(Vector3f(posDist(rng), posDist(rng), posDist(rng) - 40.0f))
                    * Transform::RotateY(rotDist(rng))
                    * Transform::Scale(s, s, s);
        instances.push_back(std::make_shared<Instance>(sharedBLAS, t));
    }

    return {"Instanced Geometry (1K Instances)", instances, static_cast<size_t>(numInstances * trianglesPerMesh), true};
}

// 4. Large primitive scaling scene: 100,000+ primitives
inline BenchmarkScene GenerateLargeScene(int count = 100000, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> posDist(-35.0f, 35.0f);
    std::uniform_real_distribution<float> radDist(0.05f, 0.25f);

    std::vector<std::shared_ptr<Shape>> shapes;
    shapes.reserve(count);
    for (int i = 0; i < count; ++i) {
        shapes.push_back(std::make_shared<Sphere>(
            Transform::Translate(Vector3f(posDist(rng), posDist(rng), posDist(rng) - 50.0f)),
            radDist(rng)));
    }
    return {"Large Scale (100K Primitives)", shapes, static_cast<size_t>(count), false};
}

} // namespace rt::bench
