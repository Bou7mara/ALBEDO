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

    std::unique_ptr<BVHNode> BuildRecursive(std::vector<PrimitiveInfo>& primInfo,
                                             int start, int end);

    std::unique_ptr<BVHNode> MakeLeaf(std::vector<PrimitiveInfo>& primInfo,
                                       int start, int end, const Bounds3f& bounds);

    bool IntersectNode(const BVHNode* node, const Ray& ray, const Vector3f& invDir,
                        const int dirIsNeg[3], SurfaceInteraction* isect) const;

    std::vector<std::shared_ptr<Shape>> originalShapes_;
    std::vector<std::shared_ptr<Shape>> orderedShapes_;
    std::unique_ptr<BVHNode> root_;
    int maxPrimsInNode_;
    SplitMethod splitMethod_;
};
}
