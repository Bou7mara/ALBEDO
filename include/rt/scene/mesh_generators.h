#pragma once
#include "rt/shapes/triangle.h"
#include <memory>

namespace rt {

    std::shared_ptr<TriangleMesh> MakeIcosahedronMesh(float radius, const Vector3f& scale);

    std::shared_ptr<TriangleMesh> MakeGridMesh(int gridResolution, float halfExtent);

    std::shared_ptr<TriangleMesh> MakeQuadMesh(float halfWidth, float halfDepth);

    std::shared_ptr<TriangleMesh> MakeRoundBrilliantDiamondMesh(float radius, const Point3f& center = Point3f(0.0f, 0.0f, 0.0f));

}
