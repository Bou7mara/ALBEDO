#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/shapes/triangle.h"

using namespace rt;

namespace {
    std::shared_ptr<TriangleMesh> MakeUnitTriangle() {

        auto mesh = std::make_shared<TriangleMesh>();
        mesh->positions = { Point3f(0,0,0), Point3f(1,0,0), Point3f(0,1,0) };
        mesh->indices   = { 0, 1, 2 };
        return mesh;
    }
}

TEST_CASE("Triangle: ray straight through the interior hits", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);

    Ray r(Point3f(0.2f, 0.2f, -5.0f), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE(tri.Intersect(r, &isect));
    REQUIRE(isect.t == Catch::Approx(5.0f));
    REQUIRE(isect.p.x == Catch::Approx(0.2f));
    REQUIRE(isect.p.y == Catch::Approx(0.2f));
}

TEST_CASE("Triangle: ray outside the edges misses", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);

    Ray r(Point3f(0.8f, 0.8f, -5.0f), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE_FALSE(tri.Intersect(r, &isect));
}

TEST_CASE("Triangle: ray parallel to the triangle's plane misses", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);

    Ray r(Point3f(0.2f, 0.2f, 1.0f), Vector3f(1, 0, 0));
    SurfaceInteraction isect;
    REQUIRE_FALSE(tri.Intersect(r, &isect));
}

TEST_CASE("Triangle: hit behind the ray origin is rejected", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);

    Ray r(Point3f(0.2f, 0.2f, 5.0f), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE_FALSE(tri.Intersect(r, &isect));
}

TEST_CASE("Triangle: back-face hits are NOT culled", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);

    Ray r(Point3f(0.2f, 0.2f, 5.0f), Vector3f(0, 0, -1));
    SurfaceInteraction isect;
    REQUIRE(tri.Intersect(r, &isect));
}

TEST_CASE("Triangle: geometric normal matches CCW winding via right-hand rule", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);

    Ray r(Point3f(0.2f, 0.2f, -5.0f), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE(tri.Intersect(r, &isect));
    REQUIRE(isect.n.z == Catch::Approx(1.0f));
}

TEST_CASE("Triangle: shading normal falls back to geometric normal when mesh has no per-vertex normals", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);

    Ray r(Point3f(0.2f, 0.2f, -5.0f), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE(tri.Intersect(r, &isect));
    REQUIRE(isect.ns.x == Catch::Approx(isect.n.x));
    REQUIRE(isect.ns.y == Catch::Approx(isect.n.y));
    REQUIRE(isect.ns.z == Catch::Approx(isect.n.z));
}

TEST_CASE("Triangle: shading normal is barycentric-interpolated when per-vertex normals are present", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    mesh->normals = {
        Normal3f(Normalize(Vector3f(0, 0, 1))),
        Normal3f(Normalize(Vector3f(0.2f, 0, 1))),
        Normal3f(Normalize(Vector3f(0, 0.2f, 1))),
    };
    Triangle tri(mesh, 0);

    Ray r(Point3f(0.9f, 0.05f, -5.0f), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE(tri.Intersect(r, &isect));
    REQUIRE(isect.ns.x > 0.1f);
}

TEST_CASE("Triangle: UV interpolation matches barycentric weights at triangle vertices", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    mesh->uvs = { Point2f(0,0), Point2f(1,0), Point2f(0,1) };
    Triangle tri(mesh, 0);

    Ray r(Point3f(0.99f, 0.005f, -5.0f), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE(tri.Intersect(r, &isect));
    REQUIRE(isect.uv.x == Catch::Approx(0.99f).margin(0.01f));
    REQUIRE(isect.uv.y == Catch::Approx(0.005f).margin(0.01f));
}

TEST_CASE("Triangle: Area matches half the cross product of edge vectors", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);
    REQUIRE(tri.Area() == Catch::Approx(0.5f));
}

TEST_CASE("Triangle: WorldBound contains all three vertices", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);
    Bounds3f b = tri.WorldBound();

    REQUIRE(b.minPt.x <= 0.0f); REQUIRE(b.maxPt.x >= 1.0f);
    REQUIRE(b.minPt.y <= 0.0f); REQUIRE(b.maxPt.y >= 1.0f);
    REQUIRE(b.minPt.z <= 0.0f); REQUIRE(b.maxPt.z >= 0.0f);
}

TEST_CASE("Triangle: Sample() and Pdf() agree on solid-angle measure", "[triangle]") {
    auto mesh = MakeUnitTriangle();
    Triangle tri(mesh, 0);
    Point3f ref(0.2f, 0.2f, -2.0f);

    for (float ux = 0.05f; ux < 1.0f; ux += 0.2f) {
        for (float uy = 0.05f; uy < 1.0f; uy += 0.2f) {
            ShapeSample s = tri.Sample(ref, Point2f(ux, uy));
            REQUIRE(s.p.z == Catch::Approx(0.0f));
            REQUIRE(s.p.x >= -1e-4f);
            REQUIRE(s.p.y >= -1e-4f);
            REQUIRE(s.p.x + s.p.y <= 1.0f + 1e-4f);

            Vector3f wi = Normalize(s.p - ref);
            float pdf = tri.Pdf(ref, wi);
            REQUIRE(s.pdf == Catch::Approx(pdf));
            REQUIRE(s.pdf > 0.0f);
        }
    }
}

TEST_CASE("MakeTriangleMesh: builds one Triangle per face and shares mesh data", "[triangle]") {
    auto mesh = std::make_shared<TriangleMesh>();
    mesh->positions = { Point3f(0,0,0), Point3f(1,0,0), Point3f(1,1,0), Point3f(0,1,0) };
    mesh->indices   = { 0, 1, 2,   0, 2, 3 };

    auto shapes = MakeTriangleMesh(mesh);
    REQUIRE(shapes.size() == 2);

    Ray r1(Point3f(0.2f, 0.2f, -5.0f), Vector3f(0, 0, 1));
    Ray r2(Point3f(0.8f, 0.8f, -5.0f), Vector3f(0, 0, 1));
    SurfaceInteraction isect;
    REQUIRE(shapes[0]->Intersect(r1, &isect));
    REQUIRE(shapes[1]->Intersect(r2, &isect));
}
