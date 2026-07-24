#include "rt/accel/bvh.h"
#include <algorithm>
#include <array>

namespace rt {

namespace {
// Number of buckets used when partitioning shapes with SAH (Surface Area Heuristic)
constexpr int kNumBuckets = 12;

// Stores total shape count and combined bounding box for shapes in a single bucket
struct BucketInfo {
    int count = 0;
    Bounds3f bounds;
};
}

// Constructor: Initializes the BVH by collecting shape bounds and building the tree
BVH::BVH(std::vector<std::shared_ptr<Shape>> shapes, int maxPrimsInNode,
         SplitMethod splitMethod)
    : originalShapes_(std::move(shapes)),
      maxPrimsInNode_(maxPrimsInNode),
      splitMethod_(splitMethod) {
    if (originalShapes_.empty()) return;

    // Collect bounding box and center point for every input shape
    std::vector<PrimitiveInfo> primInfo(originalShapes_.size());
    for (size_t i = 0; i < originalShapes_.size(); ++i) {
        Bounds3f b = originalShapes_[i]->WorldBound();
        primInfo[i] = {i, b, b.Centroid()};
    }

    // Reserve space for ordered shapes list and build the tree starting from root
    orderedShapes_.reserve(originalShapes_.size());
    root_ = BuildRecursive(primInfo, 0, static_cast<int>(primInfo.size()));
}

// Helper: Creates a leaf node holding shapes from start index to end index
std::unique_ptr<BVH::BVHNode> BVH::MakeLeaf(std::vector<PrimitiveInfo>& primInfo,
                                             int start, int end,
                                             const Bounds3f& bounds) {
    auto node = std::make_unique<BVHNode>();
    node->bounds = bounds;
    node->firstPrimOffset = static_cast<int>(orderedShapes_.size());
    node->nPrimitives = end - start;
    for (int i = start; i < end; ++i) {
        orderedShapes_.push_back(originalShapes_[primInfo[i].index]);
    }
    return node;
}

// Recursively divides shapes between start and end indices to build BVH nodes
std::unique_ptr<BVH::BVHNode> BVH::BuildRecursive(std::vector<PrimitiveInfo>& primInfo,
                                                    int start, int end) {
    // Calculate combined bounding box for all shapes in this range
    Bounds3f nodeBounds;
    for (int i = start; i < end; ++i) nodeBounds = Union(nodeBounds, primInfo[i].bounds);

    int nPrimitives = end - start;
    // If only 1 shape remains, create a leaf node immediately
    if (nPrimitives <= 1) return MakeLeaf(primInfo, start, end, nodeBounds);

    // Calculate bounding box of shape centroids to choose the split axis
    Bounds3f centroidBounds;
    for (int i = start; i < end; ++i) centroidBounds = Union(centroidBounds, primInfo[i].centroid);
    int axis = centroidBounds.MaxExtent();

    // If all centroids are at the exact same location, we cannot split further
    if (centroidBounds.maxPt[axis] == centroidBounds.minPt[axis]) {
        return MakeLeaf(primInfo, start, end, nodeBounds);
    }

    int mid = start;

    // Strategy 1: Midpoint splitting
    if (splitMethod_ == SplitMethod::Midpoint) {
        float pmid = centroidBounds.Centroid()[axis];
        auto midIter = std::partition(primInfo.begin() + start, primInfo.begin() + end,
            [axis, pmid](const PrimitiveInfo& p) { return p.centroid[axis] < pmid; });
        mid = static_cast<int>(midIter - primInfo.begin());
        // Fallback to equal counts if partition placed all shapes on one side
        if (mid == start || mid == end) {
            mid = (start + end) / 2;
            std::nth_element(primInfo.begin() + start, primInfo.begin() + mid, primInfo.begin() + end,
                [axis](const PrimitiveInfo& a, const PrimitiveInfo& b) { return a.centroid[axis] < b.centroid[axis]; });
        }
    } else {
        // Strategy 2: Surface Area Heuristic (SAH) splitting
        // Group shapes into 12 buckets along the chosen axis
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

        // Calculate estimated ray intersection cost for splits between adjacent buckets
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
            cost[i] = 0.125f + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea())
                                / nodeBounds.SurfaceArea();
        }

        // Find split location with the lowest cost
        int minCostBucket = 0;
        float minCost = cost[0];
        for (int i = 1; i < kNumBuckets - 1; ++i) {
            if (cost[i] < minCost) { minCost = cost[i]; minCostBucket = i; }
        }

        // Create leaf if split cost is higher than testing all shapes directly
        float leafCost = static_cast<float>(nPrimitives);
        if (minCost < leafCost || nPrimitives > maxPrimsInNode_) {
            auto midIter = std::partition(primInfo.begin() + start, primInfo.begin() + end,
                [&](const PrimitiveInfo& p) { return bucketFor(p) <= minCostBucket; });
            mid = static_cast<int>(midIter - primInfo.begin());
            if (mid == start || mid == end) {
                mid = (start + end) / 2;
                std::nth_element(primInfo.begin() + start, primInfo.begin() + mid, primInfo.begin() + end,
                    [axis](const PrimitiveInfo& a, const PrimitiveInfo& b) { return a.centroid[axis] < b.centroid[axis]; });
            }
        } else {
            return MakeLeaf(primInfo, start, end, nodeBounds);
        }
    }

    // Create internal node and recursively build left and right subtrees
    auto node = std::make_unique<BVHNode>();
    node->bounds = nodeBounds;
    node->splitAxis = axis;
    node->left = BuildRecursive(primInfo, start, mid);
    node->right = BuildRecursive(primInfo, mid, end);
    return node;
}

// Gets the overall bounding box enclosing all shapes in the tree
Bounds3f BVH::WorldBound() const {
    return root_ ? root_->bounds : Bounds3f();
}

// Main ray intersection entry point: Pre-calculates ray direction inverses for fast box tests
bool BVH::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    if (!root_) return false;
    Vector3f invDir(1.0f / ray.d.x, 1.0f / ray.d.y, 1.0f / ray.d.z);
    int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};
    return IntersectNode(root_.get(), ray, invDir, dirIsNeg, isect);
}

// Recursive helper for testing ray intersection against a BVH node
bool BVH::IntersectNode(const BVHNode* node, const Ray& ray, const Vector3f& invDir,
                         const int dirIsNeg[3], SurfaceInteraction* isect) const {
    // Skip node completely if ray misses its bounding box
    if (!node->bounds.IntersectP(ray, invDir, dirIsNeg)) return false;

    // If leaf node, test ray against all shapes contained inside it
    if (node->IsLeaf()) {
        bool hit = false;
        for (int i = 0; i < node->nPrimitives; ++i) {
            if (orderedShapes_[node->firstPrimOffset + i]->Intersect(ray, isect)) hit = true;
        }
        return hit;
    }

    // For internal nodes, visit child node closer to ray origin first based on ray direction sign
    const BVHNode* first  = dirIsNeg[node->splitAxis] ? node->right.get() : node->left.get();
    const BVHNode* second = dirIsNeg[node->splitAxis] ? node->left.get()  : node->right.get();

    bool hit = IntersectNode(first, ray, invDir, dirIsNeg, isect);
    hit |= IntersectNode(second, ray, invDir, dirIsNeg, isect);
    return hit;
}

// Fast check for shadow ray occlusion
bool BVH::IntersectP(const Ray& ray) const {
    SurfaceInteraction isect;
    Ray r = ray;
    return Intersect(r, &isect);
}

}

