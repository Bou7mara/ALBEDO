#pragma once
#include "rt/accel/blas.h"
#include "rt/core/bounds3.h"
#include "rt/shapes/shape.h"
#include <memory>
#include <vector>

namespace rt {

class TaskGroup;

class BVH : public BLAS {
public:
    enum class SplitMethod { Midpoint, SAH };

    static constexpr int kParallelCutoff = 1024;
    static constexpr int kDefaultMaxParallelDepth = 8;

    explicit BVH(std::vector<std::shared_ptr<Shape>> shapes,
                 int maxPrimsInNode = 4,
                 SplitMethod splitMethod = SplitMethod::SAH,
                 int numThreads = 0);

    bool Intersect(const Ray& ray, SurfaceInteraction* isect) const override;
    bool IntersectP(const Ray& ray) const override;
    Bounds3f WorldBound() const override;

    size_t NodeCount() const { return nodes_.size(); }

    struct alignas(32) LinearBVHNode {
        Bounds3f bounds;
        union {
            int primitivesOffset;
            int secondChildOffset;
        };
        uint16_t nPrimitives = 0;
        uint8_t axis = 0;
        uint8_t pad = 0;
    };

    const std::vector<LinearBVHNode>& Nodes() const { return nodes_; }
    const std::vector<std::shared_ptr<Shape>>& OrderedShapes() const { return orderedShapes_; }

private:
    struct PrimitiveInfo {
        size_t index;
        Bounds3f bounds;
        Point3f centroid;
    };

    struct BVHNode {
        Bounds3f bounds;
        std::unique_ptr<BVHNode> left, right;
        int splitAxis = 0;
        int firstPrimOffset = 0;
        int nPrimitives = 0;

        bool IsLeaf() const { return nPrimitives > 0; }
    };

    std::unique_ptr<BVHNode> BuildRecursive(std::vector<PrimitiveInfo>& primInfo,
                                            int start, int end,
                                            int depth,
                                            TaskGroup* taskGroup,
                                            int maxParallelDepth);

    std::unique_ptr<BVHNode> MakeLeaf(const std::vector<PrimitiveInfo>& primInfo,
                                      int start, int end,
                                      const Bounds3f& bounds);

    int CountNodes(const BVHNode* node) const;
    int FlattenBVHTree(const BVHNode* node, int* offset);

    std::vector<std::shared_ptr<Shape>> originalShapes_;
    std::vector<std::shared_ptr<Shape>> orderedShapes_;
    std::vector<LinearBVHNode> nodes_;
    int maxPrimsInNode_;
    SplitMethod splitMethod_;
};
}
