#include "rt/scene/mesh_generators.h"
#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>

namespace rt {

    std::shared_ptr<TriangleMesh> MakeIcosahedronMesh(float radius, const Vector3f& scale) {
        const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;

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

        mesh->indices = {
            0, 11, 5,   0, 5, 1,    0, 1, 7,    0, 7, 10,   0, 10, 11,
            1, 5, 9,    5, 11, 4,   11, 10, 2,  10, 7, 6,   7, 1, 8,
            3, 9, 4,    3, 4, 2,    3, 2, 6,    3, 6, 8,    3, 8, 9,
            4, 9, 5,    2, 4, 11,   6, 2, 10,   8, 6, 7,    9, 8, 1,
        };

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

    std::shared_ptr<TriangleMesh> MakeRoundBrilliantDiamondMesh(float radius, const Point3f& center) {
        auto mesh = std::make_shared<TriangleMesh>();

        const float R = radius;
        constexpr float pi = 3.14159265358979323846f;

        // Tolkowsky diamond proportions
        const float rTable = 0.53f * R;
        const float rStar  = 0.76f * R;
        const float rGirdle = R;

        const float hCrown    = 0.323f * R; // 34.5 deg crown angle
        const float hPavilion = 0.862f * R; // 40.75 deg pavilion angle
        const float hGirdle   = 0.035f * R;

        const float yTable = hGirdle * 0.5f + hCrown;
        const float yStar  = hGirdle * 0.5f + hCrown * 0.55f;
        const float yUG    = hGirdle * 0.5f;
        const float yLG    = -hGirdle * 0.5f;
        const float yCulet = -hPavilion;

        // 1. Table vertices (0..7)
        for (int j = 0; j < 8; ++j) {
            float angle = j * (2.0f * pi / 8.0f);
            mesh->positions.emplace_back(rTable * std::cos(angle), yTable, rTable * std::sin(angle));
        }

        // 2. Star vertices (8..15)
        for (int j = 0; j < 8; ++j) {
            float angle = (j + 0.5f) * (2.0f * pi / 8.0f);
            mesh->positions.emplace_back(rStar * std::cos(angle), yStar, rStar * std::sin(angle));
        }

        // 3. Upper Girdle vertices (16..31)
        for (int k = 0; k < 16; ++k) {
            float angle = k * (2.0f * pi / 16.0f);
            mesh->positions.emplace_back(rGirdle * std::cos(angle), yUG, rGirdle * std::sin(angle));
        }

        // 4. Lower Girdle vertices (32..47)
        for (int k = 0; k < 16; ++k) {
            float angle = k * (2.0f * pi / 16.0f);
            mesh->positions.emplace_back(rGirdle * std::cos(angle), yLG, rGirdle * std::sin(angle));
        }

        // 5. Culet vertex (48)
        mesh->positions.emplace_back(0.0f, yCulet, 0.0f);
        const int culetIdx = 48;

        // --- Indices ---

        // Table facet (8-gon fan)
        for (int j = 1; j < 7; ++j) {
            mesh->indices.push_back(0);
            mesh->indices.push_back(j + 1);
            mesh->indices.push_back(j);
        }

        // 8 Star facets
        for (int j = 0; j < 8; ++j) {
            int t1 = j;
            int t2 = (j + 1) % 8;
            int s = 8 + j;
            mesh->indices.push_back(t1);
            mesh->indices.push_back(t2);
            mesh->indices.push_back(s);
        }

        // 8 Bezel (Kite) facets (2 triangles each)
        for (int j = 0; j < 8; ++j) {
            int t = j;
            int sRight = 8 + j;
            int sLeft  = 8 + ((j + 7) % 8);
            int ug = 16 + 2 * j;

            mesh->indices.push_back(t);
            mesh->indices.push_back(sRight);
            mesh->indices.push_back(ug);

            mesh->indices.push_back(t);
            mesh->indices.push_back(ug);
            mesh->indices.push_back(sLeft);
        }

        // 16 Upper Girdle facets
        for (int j = 0; j < 8; ++j) {
            int s = 8 + j;
            int ug1 = 16 + 2 * j;
            int ug2 = 16 + 2 * j + 1;
            int ug3 = 16 + (2 * j + 2) % 16;

            mesh->indices.push_back(s);
            mesh->indices.push_back(ug1);
            mesh->indices.push_back(ug2);

            mesh->indices.push_back(s);
            mesh->indices.push_back(ug2);
            mesh->indices.push_back(ug3);
        }

        // 16 Girdle ribbon facets
        for (int k = 0; k < 16; ++k) {
            int ug1 = 16 + k;
            int ug2 = 16 + (k + 1) % 16;
            int lg1 = 32 + k;
            int lg2 = 32 + (k + 1) % 16;

            mesh->indices.push_back(ug1);
            mesh->indices.push_back(lg1);
            mesh->indices.push_back(lg2);

            mesh->indices.push_back(ug1);
            mesh->indices.push_back(lg2);
            mesh->indices.push_back(ug2);
        }

        // 16 Lower Girdle / Pavilion facets
        for (int k = 0; k < 16; ++k) {
            int lg1 = 32 + k;
            int lg2 = 32 + (k + 1) % 16;

            mesh->indices.push_back(lg1);
            mesh->indices.push_back(lg2);
            mesh->indices.push_back(culetIdx);
        }

        // Strict outward-facing normal verification for 100% of triangles
        int numTris = mesh->TriangleCount();
        for (int t = 0; t < numTris; ++t) {
            int idx0 = mesh->indices[t * 3 + 0];
            int idx1 = mesh->indices[t * 3 + 1];
            int idx2 = mesh->indices[t * 3 + 2];

            const Point3f& v0 = mesh->positions[idx0];
            const Point3f& v1 = mesh->positions[idx1];
            const Point3f& v2 = mesh->positions[idx2];

            Vector3f n = Cross(v1 - v0, v2 - v0);
            Point3f centroid((v0.x + v1.x + v2.x) / 3.0f, (v0.y + v1.y + v2.y) / 3.0f, (v0.z + v1.z + v2.z) / 3.0f);

            // Vector from internal center (0, yGirdle, 0) to facet centroid
            Vector3f outVec(centroid.x, centroid.y, centroid.z);
            if (Dot(n, outVec) < 0.0f) {
                // Swap winding to guarantee strictly outward normal
                std::swap(mesh->indices[t * 3 + 1], mesh->indices[t * 3 + 2]);
            }
        }

        // Offset to center position
        float culetOffsetY = -yCulet;
        for (auto& p : mesh->positions) {
            p = Point3f(p.x + center.x, p.y + center.y + culetOffsetY, p.z + center.z);
        }

        return mesh;
    }

}

