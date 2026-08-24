#pragma once
#include "rt/accel/blas.h"
#include "rt/core/bounds3.h"
#include "rt/core/simd.h"
#include "rt/shapes/shape.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace rt {

template <int N>
struct alignas(64) WideBVHNode {
    float minX[N];
    float maxX[N];
    float minY[N];
    float maxY[N];
    float minZ[N];
    float maxZ[N];

    uint32_t children[N];
    uint32_t numPrimitives[N];
    uint8_t  childIsLeaf;
    uint8_t  activeChildCount;
    uint8_t  pad[2];

    void SetChildBox(int i, const Bounds3f& b) {
        minX[i] = b.minPt.x;
        minY[i] = b.minPt.y;
        minZ[i] = b.minPt.z;
        maxX[i] = b.maxPt.x;
        maxY[i] = b.maxPt.y;
        maxZ[i] = b.maxPt.z;
    }

    void SetEmptyChild(int i) {
        constexpr float kInf = std::numeric_limits<float>::infinity();
        minX[i] = kInf;
        minY[i] = kInf;
        minZ[i] = kInf;
        maxX[i] = -kInf;
        maxY[i] = -kInf;
        maxZ[i] = -kInf;
        children[i] = 0;
        numPrimitives[i] = 0;
    }

    Bounds3f GetChildBox(int i) const {
        return Bounds3f(Point3f(minX[i], minY[i], minZ[i]),
                        Point3f(maxX[i], maxY[i], maxZ[i]));
    }
};

int Intersect4(const WideBVHNode<4>& node,
               const Ray& ray,
               const Vector3f& invDir,
               const int dirIsNeg[3],
               float tMaxVal,
               float tEnter[4]);

int Intersect8(const WideBVHNode<8>& node,
               const Ray& ray,
               const Vector3f& invDir,
               const int dirIsNeg[3],
               float tMaxVal,
               float tEnter[8]);

template <int N>
class WideBVH : public BLAS {
public:
    WideBVH() = default;
    explicit WideBVH(std::vector<std::shared_ptr<Shape>> shapes,
                     int maxPrimsInNode = 4,
                     int numThreads = 0);

    bool Intersect(const Ray& ray, SurfaceInteraction* isect) const override;
    bool IntersectP(const Ray& ray) const override;
    Bounds3f WorldBound() const override;

    size_t NodeCount() const { return nodes_.size(); }
    float AverageChildrenPerNode() const;
    const std::vector<WideBVHNode<N>>& Nodes() const { return nodes_; }
    const std::vector<std::shared_ptr<Shape>>& OrderedShapes() const { return orderedShapes_; }

private:
    struct BinaryBuildNode {
        Bounds3f bounds;
        std::unique_ptr<BinaryBuildNode> left, right;
        int splitAxis = 0;
        int firstPrimOffset = 0;
        int nPrimitives = 0;
        bool IsLeaf() const { return nPrimitives > 0; }
    };

    struct PrimitiveInfo {
        size_t index;
        Bounds3f bounds;
        Point3f centroid;
    };

    std::unique_ptr<BinaryBuildNode> BuildBinaryRecursive(std::vector<PrimitiveInfo>& primInfo,
                                                          int start, int end);
    std::unique_ptr<BinaryBuildNode> MakeBinaryLeaf(const std::vector<PrimitiveInfo>& primInfo,
                                                    int start, int end,
                                                    const Bounds3f& bounds);

    int CollapseNode(const BinaryBuildNode* node);

    std::vector<std::shared_ptr<Shape>> originalShapes_;
    std::vector<std::shared_ptr<Shape>> orderedShapes_;
    std::vector<WideBVHNode<N>> nodes_;
    int maxPrimsInNode_ = 4;
};

using BVH4 = WideBVH<4>;
using BVH8 = WideBVH<8>;

}
