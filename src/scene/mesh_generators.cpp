#include "rt/scene/mesh_generators.h"
#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>

namespace rt {

    std::shared_ptr<TriangleMesh> MakeIcosahedronMesh(float radius, const Vector3f& scale) {
        const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

        // 12 vertices: cyclic permutations of (0, ±1, ±phi), normalized to `radius`.
        std::vector<Point3f> raw = {
            {-1.0f,  phi, 0.0f}, { 1.0f,  phi, 0.0f}, {-1.0f, -phi, 0.0f}, { 1.0f, -phi, 0.0f},
            { 0.0f, -1.0f,  phi}, { 0.0f,  1.0f,  phi}, { 0.0f, -1.0f, -phi}, { 0.0f,  1.0f, -phi},
            { phi, 0.0f, -1.0f}, { phi, 0.0f,  1.0f}, {-phi, 0.0f, -1.0f}, {-phi, 0.0f,  1.0f},
        };

        auto mesh = std::make_shared<TriangleMesh>();
        mesh->positions.reserve(raw.size());
        for (const auto& v : raw) {
            Vector3f n = Normalize(Vector3f(v.x, v.y, v.z)); // unit circumradius direction
            mesh->positions.push_back(Point3f(
                n.x * radius * scale.x,
                n.y * radius * scale.y,
                n.z * radius * scale.z));
        }

        // Fixed face connectivity for this vertex ordering (standard icosahedron table).
        mesh->indices = {
            0, 11, 5,   0, 5, 1,    0, 1, 7,    0, 7, 10,   0, 10, 11,
            1, 5, 9,    5, 11, 4,   11, 10, 2,  10, 7, 6,   7, 1, 8,
            3, 9, 4,    3, 4, 2,    3, 2, 6,    3, 6, 8,    3, 8, 9,
            4, 9, 5,    2, 4, 11,   6, 2, 10,   8, 6, 7,    9, 8, 1,
        };

        // Deliberately no per-vertex normals: this mesh is meant to stay hard-faceted
        // (flat shading fallback).
        return mesh;
    }

    std::shared_ptr<TriangleMesh> MakeGridMesh(int gridResolution, float halfExtent) {
        auto mesh = std::make_shared<TriangleMesh>();
        const int n = gridResolution;
        const float step = (2.0f * halfExtent) / static_cast<float>(n);

        mesh->positions.reserve((n + 1) * (n + 1));
        for (int j = 0; j <= n; ++j) {
            for (int i = 0; i <= n; ++i) {
                float x = -halfExtent + i * step;
                float z = -halfExtent + j * step;
                mesh->positions.push_back(Point3f(x, 0.0f, z));
            }
        }

        auto vertexIndex = [n](int i, int j) { return j * (n + 1) + i; };

        mesh->indices.reserve(n * n * 6);
        for (int j = 0; j < n; ++j) {
            for (int i = 0; i < n; ++i) {
                int v00 = vertexIndex(i, j);
                int v10 = vertexIndex(i + 1, j);
                int v11 = vertexIndex(i + 1, j + 1);
                int v01 = vertexIndex(i, j + 1);
                // Two triangles per cell, consistent CCW winding seen from +y.
                mesh->indices.insert(mesh->indices.end(), { v00, v10, v11 });
                mesh->indices.insert(mesh->indices.end(), { v00, v11, v01 });
            }
        }
        return mesh;
    }

    std::shared_ptr<TriangleMesh> MakeQuadMesh(float halfWidth, float halfDepth) {
        auto mesh = std::make_shared<TriangleMesh>();
        mesh->positions = {
            Point3f(-halfWidth, 0.0f, -halfDepth),
            Point3f( halfWidth, 0.0f, -halfDepth),
            Point3f( halfWidth, 0.0f,  halfDepth),
            Point3f(-halfWidth, 0.0f,  halfDepth),
        };
        mesh->indices = { 0, 1, 2,   0, 2, 3 };
        return mesh;
    }

}
