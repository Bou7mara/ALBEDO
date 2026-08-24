#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/scene/mesh_generators.h"
#include <cmath>
#include <algorithm>

using namespace rt;

TEST_CASE("MakeIcosahedronMesh produces 12 vertices and 20 faces", "[mesh_generators]") {
    auto mesh = MakeIcosahedronMesh(1.0f, Vector3f(1, 1, 1));
    REQUIRE(mesh->positions.size() == 12);
    REQUIRE(mesh->indices.size() == 60);
    REQUIRE(mesh->TriangleCount() == 20);
}

TEST_CASE("MakeIcosahedronMesh respects non-uniform scale per axis", "[mesh_generators]") {
    auto mesh = MakeIcosahedronMesh(1.0f, Vector3f(1.0f, 2.0f, 1.0f));
    float maxY = 0.0f;
    for (const auto& p : mesh->positions) maxY = std::max(maxY, std::abs(p.y));

    auto unscaled = MakeIcosahedronMesh(1.0f, Vector3f(1, 1, 1));
    float maxYUnscaled = 0.0f;
    for (const auto& p : unscaled->positions) maxYUnscaled = std::max(maxYUnscaled, std::abs(p.y));
    REQUIRE(maxY == Catch::Approx(maxYUnscaled * 2.0f).margin(0.01f));
}

TEST_CASE("MakeGridMesh produces the expected vertex and triangle counts", "[mesh_generators]") {
    auto mesh = MakeGridMesh(4, 2.0f);
    REQUIRE(mesh->positions.size() == 25);
    REQUIRE(mesh->TriangleCount() == 32);
}

TEST_CASE("MakeGridMesh spans the requested extent and stays flat at y=0", "[mesh_generators]") {
    auto mesh = MakeGridMesh(2, 3.0f);
    for (const auto& p : mesh->positions) {
        REQUIRE(p.y == Catch::Approx(0.0f));
        REQUIRE(p.x >= -3.0001f);
        REQUIRE(p.x <=  3.0001f);
        REQUIRE(p.z >= -3.0001f);
        REQUIRE(p.z <=  3.0001f);
    }
}

TEST_CASE("MakeQuadMesh produces a single flat rectangle, 2 triangles", "[mesh_generators]") {
    auto mesh = MakeQuadMesh(1.5f, 0.8f);
    REQUIRE(mesh->positions.size() == 4);
    REQUIRE(mesh->TriangleCount() == 2);
}
