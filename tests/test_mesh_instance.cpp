#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/shapes/mesh_instance.h"
#include "rt/shapes/triangle.h"
#include "rt/materials/lambertian.h"
#include "rt/accel/bvh.h"
#include <numbers>

using namespace rt;
using Catch::Approx;

TEST_CASE("MeshInstance - Ray t-invariance under non-uniform scale and rotation", "[mesh_instance]") {
    // Single triangle in object space
    auto rawMesh = std::make_shared<TriangleMesh>();
    rawMesh->positions = { Point3f(0, 0, 0), Point3f(1, 0, 0), Point3f(0, 0, 1) };
    rawMesh->indices = { 0, 1, 2 };

    auto mat = std::make_shared<Lambertian>(Vector3f(0.8f, 0.8f, 0.8f));
    auto shapes = MakeTriangleMesh(rawMesh, mat);
    auto blas = std::make_shared<BVH>(shapes);

    // Transform: Scale(1, 2, 1) * RotateY(30) * Translate(10, 20, 30)
    Transform T = Transform::Translate(Vector3f(10.0f, 20.0f, 30.0f)) *
                  Transform::RotateY(30.0f) *
                  Transform::Scale(1.0f, 2.0f, 1.0f);

    auto instanceMat = std::make_shared<Lambertian>(Vector3f(0.1f, 0.2f, 0.3f));
    MeshInstance instance(blas, T, instanceMat);

    // Fire ray downward in world space toward y = 20 plane
    Ray ray(Point3f(10.2f, 50.0f, 30.2f), Vector3f(0.0f, -1.0f, 0.0f));
    SurfaceInteraction isect;

    REQUIRE(instance.Intersect(ray, &isect));

    // World hit distance expected: 50.0 - 20.0 = 30.0
    REQUIRE(isect.t == Approx(30.0f).margin(1e-3f));
    REQUIRE(isect.p.x == Approx(10.2f).margin(1e-3f));
    REQUIRE(isect.p.y == Approx(20.0f).margin(1e-3f));
    REQUIRE(isect.p.z == Approx(30.2f).margin(1e-3f));
}

TEST_CASE("MeshInstance - Normal inverse-transpose transformation under non-uniform scale", "[mesh_instance]") {
    // Tilted triangle with normal (0, -1, 1) unnormalized
    auto rawMesh = std::make_shared<TriangleMesh>();
    rawMesh->positions = { Point3f(0, 0, 0), Point3f(1, 0, 0), Point3f(0, 1, 1) };
    rawMesh->indices = { 0, 1, 2 };

    auto shapes = MakeTriangleMesh(rawMesh, nullptr);
    auto blas = std::make_shared<BVH>(shapes);

    // Non-uniform scale (1, 10, 1)
    Transform S = Transform::Scale(1.0f, 10.0f, 1.0f);
    MeshInstance instance(blas, S);

    // Ray hitting triangle
    Ray ray(Point3f(0.1f, 0.2f, 5.0f), Vector3f(0.0f, 0.0f, -1.0f));
    SurfaceInteraction isect;

    REQUIRE(instance.Intersect(ray, &isect));

    // Unit length check
    float normLength = std::sqrt(isect.n.x * isect.n.x + isect.n.y * isect.n.y + isect.n.z * isect.n.z);
    REQUIRE(normLength == Approx(1.0f).margin(1e-4f));

    // Expected inverse-transpose normal direction before normalization: (0, -1/10, 1) = (0, -0.1, 1)
    // Naive forward vector transform would give (0, -10, 1)
    // So isect.n.z should be significantly larger than abs(isect.n.y)
    REQUIRE(std::abs(isect.n.z) > 5.0f * std::abs(isect.n.y));
}

TEST_CASE("MeshInstance - WorldBound, BSDF resolution and multiple instances", "[mesh_instance]") {
    auto rawMesh = std::make_shared<TriangleMesh>();
    rawMesh->positions = { Point3f(-1, -1, 0), Point3f(1, -1, 0), Point3f(0, 1, 0) };
    rawMesh->indices = { 0, 1, 2 };

    auto shapes = MakeTriangleMesh(rawMesh, nullptr);
    auto blas = std::make_shared<BVH>(shapes);

    Transform T1 = Transform::Translate(Vector3f(10, 0, 0));
    Transform T2 = Transform::Translate(Vector3f(-10, 0, 0));

    auto mat1 = std::make_shared<Lambertian>(Vector3f(1, 0, 0));
    auto mat2 = std::make_shared<Lambertian>(Vector3f(0, 1, 0));

    MeshInstance inst1(blas, T1, mat1);
    MeshInstance inst2(blas, T2, mat2);

    // Test WorldBound
    Bounds3f b1 = inst1.WorldBound();
    REQUIRE(b1.minPt.x == Approx(9.0f));
    REQUIRE(b1.maxPt.x == Approx(11.0f));

    // Test Intersect on inst1
    Ray r1(Point3f(10, 0, 5), Vector3f(0, 0, -1));
    SurfaceInteraction isect1;
    REQUIRE(inst1.Intersect(r1, &isect1));
    REQUIRE(isect1.shape == &inst1);
    REQUIRE(isect1.shape->GetBSDF() == mat1.get());

    // Test Intersect on inst2
    Ray r2(Point3f(-10, 0, 5), Vector3f(0, 0, -1));
    SurfaceInteraction isect2;
    REQUIRE(inst2.Intersect(r2, &isect2));
    REQUIRE(isect2.shape == &inst2);
    REQUIRE(isect2.shape->GetBSDF() == mat2.get());
}
