#include "rt/scene/scene_node.h"
#include <unordered_map>

namespace rt {

namespace {

void FlattenRecursive(const std::shared_ptr<SceneNode>& node,
                      const Transform& parentToWorld,
                      Scene& scene,
                      std::unordered_map<const TriangleMesh*, std::shared_ptr<BVH>>& cache) {
    if (!node) return;

    Transform nodeToWorld = parentToWorld * node->localTransform;

    if (node->mesh) {
        auto it = cache.find(node->mesh.get());
        std::shared_ptr<BVH> blas;
        if (it != cache.end()) {
            blas = it->second;
        } else {
            auto triangles = MakeTriangleMesh(node->mesh, nullptr);
            blas = std::make_shared<BVH>(triangles);
            cache[node->mesh.get()] = blas;
        }

        auto instance = std::make_shared<MeshInstance>(blas, nodeToWorld, node->bsdf);
        scene.Add(instance);
    }

    for (const auto& child : node->children) {
        FlattenRecursive(child, nodeToWorld, scene, cache);
    }
}

}

void FlattenSceneGraph(const std::shared_ptr<SceneNode>& root, Scene& scene) {
    std::unordered_map<const TriangleMesh*, std::shared_ptr<BVH>> cache;
    FlattenRecursive(root, Transform::Identity(), scene, cache);
}

}
