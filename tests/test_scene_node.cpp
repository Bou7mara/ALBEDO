#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/scene/scene_node.h"
#include "rt/materials/lambertian.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("SceneNode - Single node identity transform", "[scene_node]") {
    auto mesh = std::make_shared<TriangleMesh>();
    mesh->positions = { Point3f(0, 0, 0), Point3f(1, 0, 0), Point3f(0, 1, 0) };
    mesh->indices = { 0, 1, 2 };

    auto root = std::make_shared<SceneNode>();
    root->mesh = mesh;
    root->bsdf = std::make_shared<Lambertian>(Vector3f(1, 1, 1));

    Scene scene;
    FlattenSceneGraph(root, scene);
    scene.Build();

    Ray ray(Point3f(0.2f, 0.2f, 1.0f), Vector3f(0, 0, -1));
    SurfaceInteraction isect;
    REQUIRE(scene.Intersect(ray, &isect));
    REQUIRE(isect.t == Approx(1.0f));
}

TEST_CASE("SceneNode - Hierarchical transform composition order", "[scene_node]") {
    auto mesh = std::make_shared<TriangleMesh>();
    mesh->positions = { Point3f(-1, -1, 0), Point3f(1, -1, 0), Point3f(0, 1, 0) };
    mesh->indices = { 0, 1, 2 };

    auto parent = std::make_shared<SceneNode>();
    parent->localTransform = Transform::Translate(Vector3f(10, 0, 0));

    auto child = std::make_shared<SceneNode>();
    child->localTransform = Transform::Translate(Vector3f(0, 5, 0));
    child->mesh = mesh;

    parent->children.push_back(child);

    Scene scene;
    FlattenSceneGraph(parent, scene);
    scene.Build();

    Ray ray(Point3f(10.0f, 5.0f, 2.0f), Vector3f(0, 0, -1));
    SurfaceInteraction isect;
    REQUIRE(scene.Intersect(ray, &isect));
    REQUIRE(isect.p.x == Approx(10.0f));
    REQUIRE(isect.p.y == Approx(5.0f));
}

TEST_CASE("SceneNode - BLAS caching for shared mesh reference", "[scene_node]") {
    auto sharedMesh = std::make_shared<TriangleMesh>();
    sharedMesh->positions = { Point3f(0, 0, 0), Point3f(1, 0, 0), Point3f(0, 1, 0) };
    sharedMesh->indices = { 0, 1, 2 };

    auto root = std::make_shared<SceneNode>();

    auto node1 = std::make_shared<SceneNode>();
    node1->mesh = sharedMesh;
    node1->localTransform = Transform::Translate(Vector3f(-5, 0, 0));

    auto node2 = std::make_shared<SceneNode>();
    node2->mesh = sharedMesh;
    node2->localTransform = Transform::Translate(Vector3f(5, 0, 0));

    root->children.push_back(node1);
    root->children.push_back(node2);

    Scene scene;
    FlattenSceneGraph(root, scene);
    scene.Build();

    Ray r1(Point3f(-4.8f, 0.1f, 1.0f), Vector3f(0, 0, -1));
    Ray r2(Point3f(5.2f, 0.1f, 1.0f), Vector3f(0, 0, -1));
    SurfaceInteraction i1, i2;

    REQUIRE(scene.Intersect(r1, &i1));
    REQUIRE(scene.Intersect(r2, &i2));

    const MeshInstance* inst1 = dynamic_cast<const MeshInstance*>(i1.shape);
    const MeshInstance* inst2 = dynamic_cast<const MeshInstance*>(i2.shape);

    REQUIRE(inst1 != nullptr);
    REQUIRE(inst2 != nullptr);
    REQUIRE(inst1 != inst2);

    REQUIRE(inst1->GetBLAS() == inst2->GetBLAS());
}

TEST_CASE("SceneNode - Empty tree handling", "[scene_node]") {
    auto root = std::make_shared<SceneNode>();
    Scene scene;
    REQUIRE_NOTHROW(FlattenSceneGraph(root, scene));
    REQUIRE_NOTHROW(scene.Build());

    Ray ray(Point3f(0, 0, 0), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE_FALSE(scene.Intersect(ray, &isect));
}
