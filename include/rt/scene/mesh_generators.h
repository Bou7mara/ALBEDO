#pragma once
#include "rt/shapes/triangle.h"
#include <memory>

namespace rt {

    // Regular icosahedron, 12 vertices / 20 faces, centered at the origin.
    // `radius` sets the base circumradius before non-uniform scaling is applied.
    // `scale` lets callers stretch it (e.g. taller-than-wide for a gem look)
    // without touching face connectivity.
    std::shared_ptr<TriangleMesh> MakeIcosahedronMesh(float radius, const Vector3f& scale);

    // A flat, tessellated NxN grid of quads (2 triangles per cell) spanning
    // [-halfExtent, +halfExtent] in x and z, at height y = 0, facing +y.
    // Returns raw geometry only — callers assign per-cell BSDFs themselves.
    std::shared_ptr<TriangleMesh> MakeGridMesh(int gridResolution, float halfExtent);

    // A single-cell horizontal rectangle (2 triangles), useful as an area light
    // or light panel. Spans [-halfWidth, halfWidth] x, [-halfDepth, halfDepth] z,
    // at height y = 0.
    std::shared_ptr<TriangleMesh> MakeQuadMesh(float halfWidth, float halfDepth);

    // 57-facet Round Brilliant Cut Diamond mesh with Tolkowsky proportions
    // (crown angle 34.5 deg, pavilion angle 40.75 deg, table 53%, 8-fold symmetry).
    // All triangles guaranteed to have outward-pointing normals.
    std::shared_ptr<TriangleMesh> MakeRoundBrilliantDiamondMesh(float radius, const Point3f& center = Point3f(0.0f, 0.0f, 0.0f));

}
