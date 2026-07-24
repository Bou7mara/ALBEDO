#pragma once
#include "rt/core/bounds3.h"
#include "rt/shapes/shape.h"
#include <memory>
#include <vector>

namespace rt {

class BVH {
public:
    enum class SplitMethod { Midpoint, SAH };

    explicit BVH(std::vector<std::shared_ptr<Shape>> shapes,
                 int maxPrimsInNode = 4,
                 SplitMethod splitMethod = SplitMethod::SAH);

    bool Intersect(const Ray& ray, SurfaceInteraction* isect) const;
    bool IntersectP(const Ray& ray) const;
    Bounds3f WorldBound() const;

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

    std::unique_ptr<BVHNode> BuildRecursive(std::vector<PrimitiveInfo>& primInfo,
                                             int start, int end);

    std::unique_ptr<BVHNode> MakeLeaf(std::vector<PrimitiveInfo>& primInfo,
                                       int start, int end, const Bounds3f& bounds);

    int CountNodes(const BVHNode* node) const;
    int FlattenBVHTree(const BVHNode* node, int* offset);

    std::vector<std::shared_ptr<Shape>> originalShapes_;
    std::vector<std::shared_ptr<Shape>> orderedShapes_;
    std::vector<LinearBVHNode> nodes_;
    int maxPrimsInNode_;
    SplitMethod splitMethod_;
};
}
