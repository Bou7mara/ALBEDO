#include "rt/accel/bvh.h"
#include "rt/core/thread_pool.h"
#include <algorithm>
#include <array>
#include <cassert>

namespace rt {

namespace {
constexpr int kNumBuckets = 12;

struct BucketInfo {
    int count = 0;
    Bounds3f bounds;
};
}

BVH::BVH(std::vector<std::shared_ptr<Shape>> shapes, int maxPrimsInNode, SplitMethod splitMethod, int numThreads)
    : originalShapes_(std::move(shapes)), maxPrimsInNode_(maxPrimsInNode), splitMethod_(splitMethod) {
    if (originalShapes_.empty()) return;

    std::vector<PrimitiveInfo> primInfo(originalShapes_.size());

    for (size_t i = 0; i < originalShapes_.size(); ++i) {
        Bounds3f b = originalShapes_[i]->WorldBound();
        primInfo[i] = {i, b, b.Centroid()};
    }

    orderedShapes_.resize(originalShapes_.size());

    std::unique_ptr<BVHNode> root;
    if (numThreads == 1 || originalShapes_.size() <= static_cast<size_t>(kParallelCutoff)) {
        root = BuildRecursive(primInfo, 0, static_cast<int>(primInfo.size()), 0, nullptr, 0);
    } else {
        std::unique_ptr<ThreadPool> customPool;
        ThreadPool* poolPtr = nullptr;
        if (numThreads > 1) {
            customPool = std::make_unique<ThreadPool>(numThreads);
            poolPtr = customPool.get();
        } else {
            poolPtr = &ThreadPool::Default();
        }

        int threadsCount = static_cast<int>(poolPtr->ThreadCount());
        int maxDepth = kDefaultMaxParallelDepth;
        if (threadsCount > 1) {
            int log2T = 1;
            while ((1 << log2T) < threadsCount) ++log2T;
            maxDepth = 2 * log2T + 2;
        }

        TaskGroup taskGroup(*poolPtr);
        root = BuildRecursive(primInfo, 0, static_cast<int>(primInfo.size()), 0, &taskGroup, maxDepth);
        taskGroup.Wait();
    }

    if (root) {
        int totalNodes = CountNodes(root.get());
        nodes_.resize(totalNodes);
        int offset = 0;
        FlattenBVHTree(root.get(), &offset);
    }
}

int BVH::CountNodes(const BVHNode* node) const {
    if (!node) return 0;
    if (node->IsLeaf()) return 1;
    return 1 + CountNodes(node->left.get()) + CountNodes(node->right.get());
}

int BVH::FlattenBVHTree(const BVHNode* node, int* offset) {
    LinearBVHNode* linearNode = &nodes_[*offset];
    int myOffset = (*offset)++;
    linearNode->bounds = node->bounds;
    if (node->IsLeaf()) {
        linearNode->primitivesOffset = node->firstPrimOffset;
        linearNode->nPrimitives = static_cast<uint16_t>(node->nPrimitives);
    } else {
        linearNode->axis = static_cast<uint8_t>(node->splitAxis);
        linearNode->nPrimitives = 0;
        FlattenBVHTree(node->left.get(), offset);
        linearNode->secondChildOffset = FlattenBVHTree(node->right.get(), offset);
    }
    return myOffset;
}

std::unique_ptr<BVH::BVHNode> BVH::MakeLeaf(const std::vector<PrimitiveInfo>& primInfo, int start, int end, const Bounds3f& bounds) {
    auto node = std::make_unique<BVHNode>();
    node->bounds = bounds;
    node->firstPrimOffset = start;
    node->nPrimitives = end - start;
    for (int i = start; i < end; ++i) {
        orderedShapes_[i] = originalShapes_[primInfo[i].index];
    }
    return node;
}

std::unique_ptr<BVH::BVHNode> BVH::BuildRecursive(std::vector<PrimitiveInfo>& primInfo,
                                                  int start, int end,
                                                  int depth,
                                                  TaskGroup* taskGroup,
                                                  int maxParallelDepth) {
    assert(start >= 0 && end <= static_cast<int>(primInfo.size()) && start <= end);
    Bounds3f nodeBounds;
    for (int i = start; i < end; ++i) nodeBounds = Union(nodeBounds, primInfo[i].bounds);

    int nPrimitives = end - start;
    if (nPrimitives <= 1) return MakeLeaf(primInfo, start, end, nodeBounds);

    Bounds3f centroidBounds;
    for (int i = start; i < end; ++i) centroidBounds = Union(centroidBounds, primInfo[i].centroid);
    int axis = centroidBounds.MaxExtent();

    if (centroidBounds.maxPt[axis] == centroidBounds.minPt[axis]) {
        return MakeLeaf(primInfo, start, end, nodeBounds);
    }

    int mid = start;

    if (splitMethod_ == SplitMethod::Midpoint) {
        float pmid = centroidBounds.Centroid()[axis];
        auto midIter = std::partition(primInfo.begin() + start, primInfo.begin() + end,
            [axis, pmid](const PrimitiveInfo& p) { return p.centroid[axis] < pmid; });
        mid = static_cast<int>(midIter - primInfo.begin());
        if (mid == start || mid == end) {
            mid = (start + end) / 2;
            std::nth_element(primInfo.begin() + start, primInfo.begin() + mid, primInfo.begin() + end,
                [axis](const PrimitiveInfo& a, const PrimitiveInfo& b) { return a.centroid[axis] < b.centroid[axis]; });
        }
    } else {
        std::array<BucketInfo, kNumBuckets> buckets;
        float cMin = centroidBounds.minPt[axis];
        float cExtent = centroidBounds.maxPt[axis] - cMin;

        auto bucketFor = [&](const PrimitiveInfo& p) {
            int b = static_cast<int>(kNumBuckets * (p.centroid[axis] - cMin) / cExtent);
            return std::min(b, kNumBuckets - 1);
        };

        for (int i = start; i < end; ++i) {
            int b = bucketFor(primInfo[i]);
            buckets[b].count++;
            buckets[b].bounds = Union(buckets[b].bounds, primInfo[i].bounds);
        }

        std::array<float, kNumBuckets - 1> cost{};
        for (int i = 0; i < kNumBuckets - 1; ++i) {
            Bounds3f b0, b1;
            int count0 = 0, count1 = 0;
            for (int j = 0; j <= i; ++j) {
                b0 = Union(b0, buckets[j].bounds);
                count0 += buckets[j].count;
            }
            for (int j = i + 1; j < kNumBuckets; ++j) {
                b1 = Union(b1, buckets[j].bounds);
                count1 += buckets[j].count;
            }
            cost[i] = 0.125f + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea()) / nodeBounds.SurfaceArea();
        }

        int minCostBucket = 0;
        float minCost = cost[0];
        for (int i = 1; i < kNumBuckets - 1; ++i) {
            if (cost[i] < minCost) { minCost = cost[i]; minCostBucket = i; }
        }

        float leafCost = static_cast<float>(nPrimitives);
        if (minCost < leafCost || nPrimitives > maxPrimsInNode_) {
            auto midIter = std::partition(primInfo.begin() + start, primInfo.begin() + end, [&](const PrimitiveInfo& p) { return bucketFor(p) <= minCostBucket; });
            mid = static_cast<int>(midIter - primInfo.begin());
            if (mid == start || mid == end) {
                mid = (start + end) / 2;
                std::nth_element(primInfo.begin() + start, primInfo.begin() + mid, primInfo.begin() + end, [axis](const PrimitiveInfo& a, const PrimitiveInfo& b) { return a.centroid[axis] < b.centroid[axis]; });
            }
        } else {
            return MakeLeaf(primInfo, start, end, nodeBounds);
        }
    }

    auto node = std::make_unique<BVHNode>();
    node->bounds = nodeBounds;
    node->splitAxis = axis;

    if (taskGroup && nPrimitives > kParallelCutoff && depth < maxParallelDepth) {
        std::unique_ptr<BVHNode> rightChild;
        TaskGroup childGroup(taskGroup->Pool());
        childGroup.Run([&]() {
            rightChild = BuildRecursive(primInfo, mid, end, depth + 1, &childGroup, maxParallelDepth);
        });

        node->left = BuildRecursive(primInfo, start, mid, depth + 1, taskGroup, maxParallelDepth);

        childGroup.Wait();
        node->right = std::move(rightChild);
    } else {
        node->left = BuildRecursive(primInfo, start, mid, depth + 1, nullptr, maxParallelDepth);
        node->right = BuildRecursive(primInfo, mid, end, depth + 1, nullptr, maxParallelDepth);
    }

    return node;
}

Bounds3f BVH::WorldBound() const {
    return nodes_.empty() ? Bounds3f() : nodes_[0].bounds;
}

bool BVH::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    if (nodes_.empty()) return false;
    bool hit = false;
    Vector3f invDir(1.0f / ray.d.x, 1.0f / ray.d.y, 1.0f / ray.d.z);
    int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};

    int toVisitTop = 0;
    int nodesToVisit[64];
    int currentNodeIndex = 0;

    while (true) {
        const LinearBVHNode* node = &nodes_[currentNodeIndex];
        if (node->bounds.IntersectP(ray, invDir, dirIsNeg)) {
            if (node->nPrimitives > 0) {
                for (int i = 0; i < node->nPrimitives; ++i) {
                    if (orderedShapes_[node->primitivesOffset + i]->Intersect(ray, isect)) {
                        hit = true;
                    }
                }
                if (toVisitTop == 0) break;
                currentNodeIndex = nodesToVisit[--toVisitTop];
            } else {
                if (dirIsNeg[node->axis]) {
                    nodesToVisit[toVisitTop++] = currentNodeIndex + 1;
                    currentNodeIndex = node->secondChildOffset;
                } else {
                    nodesToVisit[toVisitTop++] = node->secondChildOffset;
                    currentNodeIndex = currentNodeIndex + 1;
                }
            }
        } else {
            if (toVisitTop == 0) break;
            currentNodeIndex = nodesToVisit[--toVisitTop];
        }
    }
    return hit;
}

bool BVH::IntersectP(const Ray& ray) const {
    if (nodes_.empty()) return false;
    Vector3f invDir(1.0f / ray.d.x, 1.0f / ray.d.y, 1.0f / ray.d.z);
    int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};

    int toVisitTop = 0;
    int nodesToVisit[64];
    int currentNodeIndex = 0;

    while (true) {
        const LinearBVHNode* node = &nodes_[currentNodeIndex];
        if (node->bounds.IntersectP(ray, invDir, dirIsNeg)) {
            if (node->nPrimitives > 0) {
                for (int i = 0; i < node->nPrimitives; ++i) {
                    if (orderedShapes_[node->primitivesOffset + i]->IntersectP(ray)) {
                        return true;
                    }
                }
                if (toVisitTop == 0) break;
                currentNodeIndex = nodesToVisit[--toVisitTop];
            } else {
                if (dirIsNeg[node->axis]) {
                    nodesToVisit[toVisitTop++] = currentNodeIndex + 1;
                    currentNodeIndex = node->secondChildOffset;
                } else {
                    nodesToVisit[toVisitTop++] = node->secondChildOffset;
                    currentNodeIndex = currentNodeIndex + 1;
                }
            }
        } else {
            if (toVisitTop == 0) break;
            currentNodeIndex = nodesToVisit[--toVisitTop];
        }
    }
    return false;
}

}
