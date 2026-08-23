#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/accel/tlas.h"
#include "rt/accel/bvh.h"
#include "rt/shapes/triangle.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"
#include <random>

using namespace rt;
using Catch::Approx;

namespace {

std::shared_ptr<BVH> MakeUnitSphereBLAS() {
    std::vector<std::shared_ptr<Shape>> shapes;
    shapes.push_back(std::make_shared<Sphere>(Transform::Identity(), 1.0f));
    return std::make_shared<BVH>(shapes);
}

std::shared_ptr<BVH> MakeUnitTriangleBLAS() {
    auto mesh = std::make_shared<TriangleMesh>();
    mesh->positions = { Point3f(0, 0, 0), Point3f(1, 0, 0), Point3f(0, 1, 0) };
    mesh->indices = { 0, 1, 2 };
    auto shapes = MakeTriangleMesh(mesh, nullptr);
    return std::make_shared<BVH>(shapes);
}

}

TEST_CASE("TLAS - Single instance with identity transform matches direct BLAS traversal", "[tlas]") {
    auto blas = MakeUnitSphereBLAS();
    auto inst = std::make_shared<Instance>(blas, Transform::Identity());

    TLAS tlas;
    tlas.Add(inst);
    tlas.Build();

    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(-0.8f, 0.8f);

    for (int i = 0; i < 100; ++i) {
        Ray rDirect(Point3f(dist(rng), dist(rng), 5.0f), Vector3f(0, 0, -1.0f));
        Ray rTLAS = rDirect;

        SurfaceInteraction isectDirect, isectTLAS;
        bool hitDirect = blas->Intersect(rDirect, &isectDirect);
        bool hitTLAS = tlas.Intersect(rTLAS, &isectTLAS);

        REQUIRE(hitDirect == hitTLAS);
        if (hitDirect) {
            REQUIRE(isectTLAS.t == Approx(isectDirect.t).margin(1e-4f));
            REQUIRE(isectTLAS.p.x == Approx(isectDirect.p.x).margin(1e-4f));
            REQUIRE(isectTLAS.p.y == Approx(isectDirect.p.y).margin(1e-4f));
            REQUIRE(isectTLAS.p.z == Approx(isectDirect.p.z).margin(1e-4f));
            REQUIRE(isectTLAS.n.x == Approx(isectDirect.n.x).margin(1e-4f));
            REQUIRE(isectTLAS.n.y == Approx(isectDirect.n.y).margin(1e-4f));
            REQUIRE(isectTLAS.n.z == Approx(isectDirect.n.z).margin(1e-4f));
        }
    }
}

TEST_CASE("TLAS - Multi-instance distinct affine transforms", "[tlas]") {
    auto sphereBlas = MakeUnitSphereBLAS();

    std::vector<std::shared_ptr<Instance>> instances;
    // 5 instances along X axis at x = -20, -10, 0, 10, 20
    for (int i = -2; i <= 2; ++i) {
        Transform T = Transform::Translate(Vector3f(i * 10.0f, 0.0f, 0.0f));
        instances.push_back(std::make_shared<Instance>(sphereBlas, T));
    }

    TLAS tlas(instances);

    for (int i = -2; i <= 2; ++i) {
        Ray ray(Point3f(i * 10.0f, 0.0f, 10.0f), Vector3f(0, 0, -1.0f));
        SurfaceInteraction isect;
        REQUIRE(tlas.Intersect(ray, &isect));
        REQUIRE(isect.t == Approx(9.0f).margin(1e-4f)); // hit front of sphere at z = 1 (10 - 1 = 9)
        REQUIRE(isect.p.x == Approx(i * 10.0f).margin(1e-4f));
        REQUIRE(isect.p.y == Approx(0.0f).margin(1e-4f));
        REQUIRE(isect.p.z == Approx(1.0f).margin(1e-4f));
        REQUIRE(isect.n.z == Approx(1.0f).margin(1e-4f));
    }
}

TEST_CASE("TLAS - Normal correctness under non-uniform scale", "[tlas]") {
    // Tilted triangle with normal (0, -1, 1) in object space
    auto mesh = std::make_shared<TriangleMesh>();
    mesh->positions = { Point3f(0, 0, 0), Point3f(1, 0, 0), Point3f(0, 1, 1) };
    mesh->indices = { 0, 1, 2 };
    auto blas = std::make_shared<BVH>(MakeTriangleMesh(mesh, nullptr));

    // Non-uniform scale (1, 10, 1)
    Transform S = Transform::Scale(1.0f, 10.0f, 1.0f);
    auto inst = std::make_shared<Instance>(blas, S);

    TLAS tlas;
    tlas.Add(inst);
    tlas.Build();

    Ray ray(Point3f(0.1f, 0.2f, 5.0f), Vector3f(0, 0, -1.0f));
    SurfaceInteraction isect;
    REQUIRE(tlas.Intersect(ray, &isect));

    float normLength = std::sqrt(isect.n.x * isect.n.x + isect.n.y * isect.n.y + isect.n.z * isect.n.z);
    REQUIRE(normLength == Approx(1.0f).margin(1e-4f));

    // Under inverse-transpose: unnormalized normal is (0, -1/10, 1) = (0, -0.1, 1)
    // Naive forward transform would give (0, -10, 1)
    REQUIRE(std::abs(isect.n.z) > 5.0f * std::abs(isect.n.y));
}

TEST_CASE("TLAS - Overlapping instances correctly return nearest hit via tMax shrinking", "[tlas]") {
    auto blas = MakeUnitSphereBLAS();

    // Two overlapping sphere instances along Z axis
    // Near sphere at z = 0 (bounds [-1, 1])
    // Far sphere at z = -1 (bounds [-2, 0])
    auto nearInst = std::make_shared<Instance>(blas, Transform::Translate(Vector3f(0, 0, 0)));
    auto farInst = std::make_shared<Instance>(blas, Transform::Translate(Vector3f(0, 0, -1.0f)));

    TLAS tlas;
    tlas.Add(farInst);  // Add far first
    tlas.Add(nearInst); // Add near second
    tlas.Build();

    Ray ray(Point3f(0, 0, 10.0f), Vector3f(0, 0, -1.0f));
    SurfaceInteraction isect;
    REQUIRE(tlas.Intersect(ray, &isect));

    // Near sphere hit should be at z = 1 (t = 9.0), not far sphere hit at z = 0 (t = 10.0)
    REQUIRE(isect.t == Approx(9.0f).margin(1e-4f));
    REQUIRE(isect.p.z == Approx(1.0f).margin(1e-4f));
    REQUIRE(isect.shape == nearInst.get());
}

TEST_CASE("TLAS - Shadow ray IntersectP parity with Intersect", "[tlas]") {
    auto blas = MakeUnitSphereBLAS();

    std::vector<std::shared_ptr<Instance>> instances;
    for (int i = 0; i < 10; ++i) {
        Transform T = Transform::Translate(Vector3f(i * 3.0f, 0, 0));
        instances.push_back(std::make_shared<Instance>(blas, T));
    }
    TLAS tlas(instances);

    std::mt19937 rng(999);
    std::uniform_real_distribution<float> xDist(-5.0f, 35.0f);
    std::uniform_real_distribution<float> yDist(-3.0f, 3.0f);

    for (int i = 0; i < 200; ++i) {
        Ray r(Point3f(xDist(rng), yDist(rng), 10.0f), Vector3f(0, 0, -1.0f));
        SurfaceInteraction isect;
        bool hit = tlas.Intersect(r, &isect);
        bool hitP = tlas.IntersectP(r);
        REQUIRE(hit == hitP);
    }
}

TEST_CASE("TLAS - BLAS sharing and memory diagnostic verification", "[tlas]") {
    auto meshBlas = MakeUnitTriangleBLAS();

    std::vector<std::shared_ptr<Instance>> instances;
    const int kNumInstances = 100;
    for (int i = 0; i < kNumInstances; ++i) {
        Transform T = Transform::Translate(Vector3f(static_cast<float>(i), 0, 0));
        instances.push_back(std::make_shared<Instance>(meshBlas, T));
    }

    TLAS tlas(instances);
    auto stats = tlas.GetStats();

    REQUIRE(stats.numInstances == 100);
    REQUIRE(stats.numUniqueBLAS == 1);
    REQUIRE(tlas.InstanceCount() == 100);
    REQUIRE(tlas.UniqueBLASCount() == 1);
}
