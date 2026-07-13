#include "rt/accel/bvh.h"
#include <algorithm>
#include <array>

namespace rt {

namespace {
constexpr int kNumBuckets = 12;
struct BucketInfo {
    int count = 0;
    Bounds3f bounds;
};
} // namespace

BVH::BVH(std::vector<std::shared_ptr<Shape>> shapes, int maxPrimsInNode,
         SplitMethod splitMethod)
    : originalShapes_(std::move(shapes)),
      maxPrimsInNode_(maxPrimsInNode),
      splitMethod_(splitMethod) {
    if (originalShapes_.empty()) return;

    std::vector<PrimitiveInfo> primInfo(originalShapes_.size());
    for (size_t i = 0; i < originalShapes_.size(); ++i) {
        Bounds3f b = originalShapes_[i]->WorldBound();
        primInfo[i] = {i, b, b.Centroid()};
    }

    orderedShapes_.reserve(originalShapes_.size());
    root_ = BuildRecursive(primInfo, 0, static_cast<int>(primInfo.size()));
}

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

std::unique_ptr<BVH::BVHNode> BVH::BuildRecursive(std::vector<PrimitiveInfo>& primInfo,
                                                    int start, int end) {
    Bounds3f nodeBounds;
    for (int i = start; i < end; ++i) nodeBounds = Union(nodeBounds, primInfo[i].bounds);

    int nPrimitives = end - start;
    if (nPrimitives <= 1) return MakeLeaf(primInfo, start, end, nodeBounds);

    Bounds3f centroidBounds;
    for (int i = start; i < end; ++i) centroidBounds = Union(centroidBounds, primInfo[i].centroid);
    int axis = centroidBounds.MaxExtent();

    // Degenerate case: all centroids coincide on the split axis (e.g.
    // every primitive stacked at the same point). Neither midpoint nor
    // bucket-index math is well-defined when the extent is zero --
    // fall straight to a leaf rather than dividing by zero.
    if (centroidBounds.pMax[axis] == centroidBounds.pMin[axis]) {
        return MakeLeaf(primInfo, start, end, nodeBounds);
    }

    int mid = start;

    if (splitMethod_ == SplitMethod::Midpoint) {
        float pmid = centroidBounds.Centroid()[axis];
        auto midIter = std::partition(primInfo.begin() + start, primInfo.begin() + end,
            [axis, pmid](const PrimitiveInfo& p) { return p.centroid[axis] < pmid; });
        mid = static_cast<int>(midIter - primInfo.begin());
        // Partition can degenerate to "everything on one side" (e.g.
        // two distinct centroid clusters that don't straddle the
        // midpoint). Force an equal-count split so recursion always
        // makes progress instead of looping on an unsplit range.
        if (mid == start || mid == end) {
            mid = (start + end) / 2;
            std::nth_element(primInfo.begin() + start, primInfo.begin() + mid, primInfo.begin() + end,
                [axis](const PrimitiveInfo& a, const PrimitiveInfo& b) { return a.centroid[axis] < b.centroid[axis]; });
        }
    } else {
        // Binned SAH (TOC 5.3.2). Assign each primitive to one of
        // kNumBuckets buckets by where its centroid falls along the
        // split axis, then evaluate the SAH cost of splitting between
        // every adjacent pair of buckets in O(kNumBuckets) rather than
        // O(nPrimitives) candidate splits.
        std::array<BucketInfo, kNumBuckets> buckets;
        float cMin = centroidBounds.pMin[axis];
        float cExtent = centroidBounds.pMax[axis] - cMin;

        auto bucketFor = [&](const PrimitiveInfo& p) {
            int b = static_cast<int>(kNumBuckets * (p.centroid[axis] - cMin) / cExtent);
            return std::min(b, kNumBuckets - 1);
        };

        for (int i = start; i < end; ++i) {
            int b = bucketFor(primInfo[i]);
            buckets[b].count++;
            buckets[b].bounds = Union(buckets[b].bounds, primInfo[i].bounds);
        }

        // Cost of splitting after bucket i (i.e. buckets [0,i] left,
        // [i+1, kNumBuckets) right): traversalCost + relative surface
        // area of each side, weighted by primitive count -- the
        // standard SAH cost model (TOC 5.3.1). 0.125 is the assumed
        // relative cost of a traversal step vs. a primitive
        // intersection test (1.0) -- pbrt's own default constant.
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

        int minCostBucket = 0;
        float minCost = cost[0];
        for (int i = 1; i < kNumBuckets - 1; ++i) {
            if (cost[i] < minCost) { minCost = cost[i]; minCostBucket = i; }
        }

        float leafCost = static_cast<float>(nPrimitives);
        if (minCost < leafCost || nPrimitives > maxPrimsInNode_) {
            // Either the split is worth it outright, or it isn't but
            // the leaf would exceed maxPrimsInNode_ -- in which case
            // splitting anyway (even at a locally suboptimal point) is
            // still required to respect the leaf-size cap.
            auto midIter = std::partition(primInfo.begin() + start, primInfo.begin() + end,
                [&](const PrimitiveInfo& p) { return bucketFor(p) <= minCostBucket; });
            mid = static_cast<int>(midIter - primInfo.begin());
            if (mid == start || mid == end) {
                // Buckets can still degenerate to a no-op split (all
                // primitives landed in the same bucket range) -- same
                // equal-count fallback as the Midpoint path.
                mid = (start + end) / 2;
                std::nth_element(primInfo.begin() + start, primInfo.begin() + mid, primInfo.begin() + end,
                    [axis](const PrimitiveInfo& a, const PrimitiveInfo& b) { return a.centroid[axis] < b.centroid[axis]; });
            }
        } else {
            return MakeLeaf(primInfo, start, end, nodeBounds);
        }
    }

    auto node = std::make_unique<BVHNode>();
    node->bounds = nodeBounds;
    node->splitAxis = axis;
    node->left = BuildRecursive(primInfo, start, mid);
    node->right = BuildRecursive(primInfo, mid, end);
    return node;
}

Bounds3f BVH::WorldBound() const {
    return root_ ? root_->bounds : Bounds3f();
}

bool BVH::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    if (!root_) return false;
    Vector3f invDir(1.0f / ray.d.x, 1.0f / ray.d.y, 1.0f / ray.d.z);
    int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};
    return IntersectNode(root_.get(), ray, invDir, dirIsNeg, isect);
}

bool BVH::IntersectNode(const BVHNode* node, const Ray& ray, const Vector3f& invDir,
                         const int dirIsNeg[3], SurfaceInteraction* isect) const {
    if (!node->bounds.IntersectP(ray, invDir, dirIsNeg)) return false;

    if (node->IsLeaf()) {
        bool hit = false;
        for (int i = 0; i < node->nPrimitives; ++i) {
            if (orderedShapes_[node->firstPrimOffset + i]->Intersect(ray, isect)) hit = true;
        }
        return hit;
    }

    // Visit the near child first (per the ray's direction sign on the
    // split axis), then the far child. No extra bookkeeping needed for
    // the classic "skip the far child if the near hit is already
    // closer" pruning: ray.tMax is mutable and already shrunk by any
    // hit in the near subtree, so the far child's own bounds.IntersectP
    // call re-reads that shrunk tMax automatically and can reject
    // early on its own -- this is the mutable-tMax convention (from
    // Ray's original design) doing real work here, not just carried
    // along for consistency.
    const BVHNode* first  = dirIsNeg[node->splitAxis] ? node->right.get() : node->left.get();
    const BVHNode* second = dirIsNeg[node->splitAxis] ? node->left.get()  : node->right.get();

    bool hit = IntersectNode(first, ray, invDir, dirIsNeg, isect);
    hit |= IntersectNode(second, ray, invDir, dirIsNeg, isect);
    return hit;
}

bool BVH::IntersectP(const Ray& ray) const {
    SurfaceInteraction isect;
    Ray r = ray;
    return Intersect(r, &isect);
}

} // namespace rt
