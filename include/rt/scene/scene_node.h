#pragma once
#include "rt/core/transform.h"
#include "rt/scene/scene.h"
#include "rt/shapes/mesh_instance.h"
#include "rt/shapes/triangle.h"
#include <memory>
#include <vector>

namespace rt {

    struct SceneNode {
        Transform localTransform = Transform::Identity();
        std::shared_ptr<TriangleMesh> mesh;
        std::shared_ptr<BSDF> bsdf;
        std::vector<std::shared_ptr<SceneNode>> children;
    };

    void FlattenSceneGraph(const std::shared_ptr<SceneNode>& root, Scene& scene);

} // namespace rt
