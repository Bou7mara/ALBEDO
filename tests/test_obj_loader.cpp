#include <catch2/catch_test_macros.hpp>
#include "rt/io/obj_loader.h"
#include <stdexcept>

using namespace rt;

TEST_CASE("OBJ Loader - Quad face without vt/vn", "[obj_loader]") {
    std::string objData = R"(
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
f 1 2 3 4
)";
    auto mesh = LoadOBJFromString(objData);
    REQUIRE(mesh->TriangleCount() == 2);
    REQUIRE(mesh->positions.size() == 4);
    REQUIRE(mesh->uvs.empty());
    REQUIRE(mesh->normals.empty());
}

TEST_CASE("OBJ Loader - Flat quad with full v/vt/vn triples deduplication", "[obj_loader]") {
    std::string objData = R"(
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
vt 0 0
vt 1 0
vt 1 1
vt 0 1
vn 0 0 1
f 1/1/1 2/2/1 3/3/1 4/4/1
)";
    auto mesh = LoadOBJFromString(objData);
    REQUIRE(mesh->TriangleCount() == 2);
    REQUIRE(mesh->positions.size() == 4);
    REQUIRE(mesh->uvs.size() == 4);
    REQUIRE(mesh->normals.size() == 4);
}

TEST_CASE("OBJ Loader - Cube corner with shared position but different normals", "[obj_loader]") {
    // Position v1 at (0,0,0) used by 3 faces with 3 different normals (vn1, vn2, vn3).
    // Positions v2, v3, v4 share vn1 across their face references.
    std::string objData = R"(
v 0 0 0
v 1 0 0
v 0 1 0
v 0 0 1
vn 1 0 0
vn 0 1 0
vn 0 0 1
f 1//1 2//1 3//1
f 1//2 3//1 4//1
f 1//3 4//1 2//1
)";
    auto mesh = LoadOBJFromString(objData);
    REQUIRE(mesh->TriangleCount() == 3);
    // Position (0,0,0) with 3 different normals must produce 3 distinct unified vertices for that position
    // Plus v2, v3, v4 each used once = total 6 unified vertices
    REQUIRE(mesh->positions.size() == 6);
    REQUIRE(mesh->normals.size() == 6);
}

TEST_CASE("OBJ Loader - Negative relative indices", "[obj_loader]") {
    std::string objData = R"(
v 0 0 0
v 1 0 0
v 0 1 0
f -3 -2 -1
)";
    auto mesh = LoadOBJFromString(objData);
    REQUIRE(mesh->TriangleCount() == 1);
    REQUIRE(mesh->positions.size() == 3);
    REQUIRE(mesh->indices[0] == 0);
    REQUIRE(mesh->indices[1] == 1);
    REQUIRE(mesh->indices[2] == 2);
}

TEST_CASE("OBJ Loader - Double slash (no UVs, with normals)", "[obj_loader]") {
    std::string objData = R"(
v 0 0 0
v 1 0 0
v 0 1 0
vn 0 0 1
f 1//1 2//1 3//1
)";
    auto mesh = LoadOBJFromString(objData);
    REQUIRE(mesh->TriangleCount() == 1);
    REQUIRE(mesh->uvs.empty());
    REQUIRE_FALSE(mesh->normals.empty());
    REQUIRE(mesh->normals.size() == 3);
}

TEST_CASE("OBJ Loader - Ignored lines", "[obj_loader]") {
    std::string objData = R"(
# This is a comment
o CubeObject
g MainGroup
s 1
mtllib material.mtl
v 0 0 0
usemtl DefaultMat
v 1 0 0
v 0 1 0
f 1 2 3
)";
    auto mesh = LoadOBJFromString(objData);
    REQUIRE(mesh->TriangleCount() == 1);
    REQUIRE(mesh->positions.size() == 3);
}

TEST_CASE("OBJ Loader - Malformed line (< 3 vertices in face)", "[obj_loader]") {
    std::string objData = R"(
v 0 0 0
v 1 0 0
f 1 2
)";
    REQUIRE_THROWS_AS(LoadOBJFromString(objData), std::runtime_error);
}
