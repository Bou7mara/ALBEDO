#include "rt/accel/wide_bvh.h"
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

int Intersect4(const WideBVHNode<4>& node,
               const Ray& ray,
               const Vector3f& invDir,
               const int dirIsNeg[3],
               float tMaxVal,
               float tEnter[4]) {
#if ALBEDO_HAS_SSE
    __m128 rayOx = _mm_set1_ps(ray.o.x);
    __m128 rayOy = _mm_set1_ps(ray.o.y);
    __m128 rayOz = _mm_set1_ps(ray.o.z);

    __m128 invDx = _mm_set1_ps(invDir.x);
    __m128 invDy = _mm_set1_ps(invDir.y);
    __m128 invDz = _mm_set1_ps(invDir.z);

    __m128 minX = _mm_loadu_ps(node.minX);
    __m128 maxX = _mm_loadu_ps(node.maxX);
    __m128 minY = _mm_loadu_ps(node.minY);
    __m128 maxY = _mm_loadu_ps(node.maxY);
    __m128 minZ = _mm_loadu_ps(node.minZ);
    __m128 maxZ = _mm_loadu_ps(node.maxZ);

    __m128 t0x = _mm_mul_ps(_mm_sub_ps(dirIsNeg[0] ? maxX : minX, rayOx), invDx);
    __m128 t1x = _mm_mul_ps(_mm_sub_ps(dirIsNeg[0] ? minX : maxX, rayOx), invDx);

    __m128 t0y = _mm_mul_ps(_mm_sub_ps(dirIsNeg[1] ? maxY : minY, rayOy), invDy);
    __m128 t1y = _mm_mul_ps(_mm_sub_ps(dirIsNeg[1] ? minY : maxY, rayOy), invDy);

    __m128 t0z = _mm_mul_ps(_mm_sub_ps(dirIsNeg[2] ? maxZ : minZ, rayOz), invDz);
    __m128 t1z = _mm_mul_ps(_mm_sub_ps(dirIsNeg[2] ? minZ : maxZ, rayOz), invDz);

    __m128 tNear = _mm_max_ps(_mm_max_ps(t0x, t0y), _mm_max_ps(t0z, _mm_setzero_ps()));
    __m128 tFar  = _mm_min_ps(_mm_min_ps(t1x, t1y), _mm_min_ps(t1z, _mm_set1_ps(tMaxVal)));

    __m128 mask = _mm_cmple_ps(tNear, tFar);
    int hitMask = _mm_movemask_ps(mask);

    _mm_storeu_ps(tEnter, tNear);
    return hitMask;
#else
    int hitMask = 0;
    for (int i = 0; i < 4; ++i) {
        float t0x = ((dirIsNeg[0] ? node.maxX[i] : node.minX[i]) - ray.o.x) * invDir.x;
        float t1x = ((dirIsNeg[0] ? node.minX[i] : node.maxX[i]) - ray.o.x) * invDir.x;
        float t0y = ((dirIsNeg[1] ? node.maxY[i] : node.minY[i]) - ray.o.y) * invDir.y;
        float t1y = ((dirIsNeg[1] ? node.minY[i] : node.maxY[i]) - ray.o.y) * invDir.y;
        float t0z = ((dirIsNeg[2] ? node.maxZ[i] : node.minZ[i]) - ray.o.z) * invDir.z;
        float t1z = ((dirIsNeg[2] ? node.minZ[i] : node.maxZ[i]) - ray.o.z) * invDir.z;

        float tNear = std::max({0.0f, std::min(t0x, t1x), std::min(t0y, t1y), std::min(t0z, t1z)});
        float tFar  = std::min({tMaxVal, std::max(t0x, t1x), std::max(t0y, t1y), std::max(t0z, t1z)});

        if (tNear <= tFar) {
            hitMask |= (1 << i);
            tEnter[i] = tNear;
        } else {
            tEnter[i] = std::numeric_limits<float>::infinity();
        }
    }
    return hitMask;
#endif
}

int Intersect8(const WideBVHNode<8>& node,
               const Ray& ray,
               const Vector3f& invDir,
               const int dirIsNeg[3],
               float tMaxVal,
               float tEnter[8]) {
#if ALBEDO_HAS_AVX2
    __m256 rayOx = _mm256_set1_ps(ray.o.x);
    __m256 rayOy = _mm256_set1_ps(ray.o.y);
    __m256 rayOz = _mm256_set1_ps(ray.o.z);

    __m256 invDx = _mm256_set1_ps(invDir.x);
    __m256 invDy = _mm256_set1_ps(invDir.y);
    __m256 invDz = _mm256_set1_ps(invDir.z);

    __m256 minX = _mm256_loadu_ps(node.minX);
    __m256 maxX = _mm256_loadu_ps(node.maxX);
    __m256 minY = _mm256_loadu_ps(node.minY);
    __m256 maxY = _mm256_loadu_ps(node.maxY);
    __m256 minZ = _mm256_loadu_ps(node.minZ);
    __m256 maxZ = _mm256_loadu_ps(node.maxZ);

    __m256 t0x = _mm256_mul_ps(_mm256_sub_ps(dirIsNeg[0] ? maxX : minX, rayOx), invDx);
    __m256 t1x = _mm256_mul_ps(_mm256_sub_ps(dirIsNeg[0] ? minX : maxX, rayOx), invDx);

    __m256 t0y = _mm256_mul_ps(_mm256_sub_ps(dirIsNeg[1] ? maxY : minY, rayOy), invDy);
    __m256 t1y = _mm256_mul_ps(_mm256_sub_ps(dirIsNeg[1] ? minY : maxY, rayOy), invDy);

    __m256 t0z = _mm256_mul_ps(_mm256_sub_ps(dirIsNeg[2] ? maxZ : minZ, rayOz), invDz);
    __m256 t1z = _mm256_mul_ps(_mm256_sub_ps(dirIsNeg[2] ? minZ : maxZ, rayOz), invDz);

    __m256 tNear = _mm256_max_ps(_mm256_max_ps(t0x, t0y), _mm256_max_ps(t0z, _mm256_setzero_ps()));
    __m256 tFar  = _mm256_min_ps(_mm256_min_ps(t1x, t1y), _mm256_min_ps(t1z, _mm256_set1_ps(tMaxVal)));

    __m256 mask = _mm256_cmp_ps(tNear, tFar, _CMP_LE_OQ);
    int hitMask = _mm256_movemask_ps(mask);

    _mm256_storeu_ps(tEnter, tNear);
    return hitMask;
#else
    int hitMask = 0;
    for (int i = 0; i < 8; ++i) {
        float t0x = ((dirIsNeg[0] ? node.maxX[i] : node.minX[i]) - ray.o.x) * invDir.x;
        float t1x = ((dirIsNeg[0] ? node.minX[i] : node.maxX[i]) - ray.o.x) * invDir.x;
        float t0y = ((dirIsNeg[1] ? node.maxY[i] : node.minY[i]) - ray.o.y) * invDir.y;
        float t1y = ((dirIsNeg[1] ? node.minY[i] : node.maxY[i]) - ray.o.y) * invDir.y;
        float t0z = ((dirIsNeg[2] ? node.maxZ[i] : node.minZ[i]) - ray.o.z) * invDir.z;
        float t1z = ((dirIsNeg[2] ? node.minZ[i] : node.maxZ[i]) - ray.o.z) * invDir.z;

        float tNear = std::max({0.0f, std::min(t0x, t1x), std::min(t0y, t1y), std::min(t0z, t1z)});
        float tFar  = std::min({tMaxVal, std::max(t0x, t1x), std::max(t0y, t1y), std::max(t0z, t1z)});

        if (tNear <= tFar) {
            hitMask |= (1 << i);
            tEnter[i] = tNear;
        } else {
            tEnter[i] = std::numeric_limits<float>::infinity();
        }
    }
    return hitMask;
#endif
}

template <int N>
inline int IntersectN(const WideBVHNode<N>& node,
                      const Ray& ray,
                      const Vector3f& invDir,
                      const int dirIsNeg[3],
                      float tMaxVal,
                      float tEnter[N]) {
    if constexpr (N == 4) {
        return Intersect4(node, ray, invDir, dirIsNeg, tMaxVal, tEnter);
    } else if constexpr (N == 8) {
        return Intersect8(node, ray, invDir, dirIsNeg, tMaxVal, tEnter);
    } else {
        int hitMask = 0;
        for (int i = 0; i < N; ++i) {
            float t0x = ((dirIsNeg[0] ? node.maxX[i] : node.minX[i]) - ray.o.x) * invDir.x;
            float t1x = ((dirIsNeg[0] ? node.minX[i] : node.maxX[i]) - ray.o.x) * invDir.x;
            float t0y = ((dirIsNeg[1] ? node.maxY[i] : node.minY[i]) - ray.o.y) * invDir.y;
            float t1y = ((dirIsNeg[1] ? node.minY[i] : node.maxY[i]) - ray.o.y) * invDir.y;
            float t0z = ((dirIsNeg[2] ? node.maxZ[i] : node.minZ[i]) - ray.o.z) * invDir.z;
            float t1z = ((dirIsNeg[2] ? node.minZ[i] : node.maxZ[i]) - ray.o.z) * invDir.z;

            float tNear = std::max({0.0f, std::min(t0x, t1x), std::min(t0y, t1y), std::min(t0z, t1z)});
            float tFar  = std::min({tMaxVal, std::max(t0x, t1x), std::max(t0y, t1y), std::max(t0z, t1z)});

            if (tNear <= tFar) {
                hitMask |= (1 << i);
                tEnter[i] = tNear;
            } else {
                tEnter[i] = std::numeric_limits<float>::infinity();
            }
        }
        return hitMask;
    }
}

template <int N>
WideBVH<N>::WideBVH(std::vector<std::shared_ptr<Shape>> shapes, int maxPrimsInNode, int numThreads)
    : originalShapes_(std::move(shapes)), maxPrimsInNode_(maxPrimsInNode) {
    (void)numThreads;
    if (originalShapes_.empty()) return;

    std::vector<PrimitiveInfo> primInfo(originalShapes_.size());
    for (size_t i = 0; i < originalShapes_.size(); ++i) {
        Bounds3f b = originalShapes_[i]->WorldBound();
        primInfo[i] = {i, b, b.Centroid()};
    }

    orderedShapes_.resize(originalShapes_.size());
    auto root = BuildBinaryRecursive(primInfo, 0, static_cast<int>(primInfo.size()));
    if (root) {
        CollapseNode(root.get());
    }
}

template <int N>
std::unique_ptr<typename WideBVH<N>::BinaryBuildNode> WideBVH<N>::MakeBinaryLeaf(
    const std::vector<PrimitiveInfo>& primInfo, int start, int end, const Bounds3f& bounds) {
    auto node = std::make_unique<BinaryBuildNode>();
    node->bounds = bounds;
    node->firstPrimOffset = start;
    node->nPrimitives = end - start;
    for (int i = start; i < end; ++i) {
        orderedShapes_[i] = originalShapes_[primInfo[i].index];
    }
    return node;
}

template <int N>
std::unique_ptr<typename WideBVH<N>::BinaryBuildNode> WideBVH<N>::BuildBinaryRecursive(
    std::vector<PrimitiveInfo>& primInfo, int start, int end) {
    Bounds3f nodeBounds;
    for (int i = start; i < end; ++i) nodeBounds = Union(nodeBounds, primInfo[i].bounds);

    int nPrimitives = end - start;
    if (nPrimitives <= 1) return MakeBinaryLeaf(primInfo, start, end, nodeBounds);

    Bounds3f centroidBounds;
    for (int i = start; i < end; ++i) centroidBounds = Union(centroidBounds, primInfo[i].centroid);
    int axis = centroidBounds.MaxExtent();

    if (centroidBounds.maxPt[axis] == centroidBounds.minPt[axis]) {
        return MakeBinaryLeaf(primInfo, start, end, nodeBounds);
    }

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
    int mid = start;
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
        return MakeBinaryLeaf(primInfo, start, end, nodeBounds);
    }

    auto node = std::make_unique<BinaryBuildNode>();
    node->bounds = nodeBounds;
    node->splitAxis = axis;
    node->left = BuildBinaryRecursive(primInfo, start, mid);
    node->right = BuildBinaryRecursive(primInfo, mid, end);
    return node;
}

template <int N>
int WideBVH<N>::CollapseNode(const BinaryBuildNode* node) {
    if (!node) return -1;

    int myIndex = static_cast<int>(nodes_.size());
    nodes_.emplace_back();

    if (node->IsLeaf()) {
        nodes_[myIndex].SetChildBox(0, node->bounds);
        nodes_[myIndex].children[0] = node->firstPrimOffset;
        nodes_[myIndex].numPrimitives[0] = node->nPrimitives;
        nodes_[myIndex].childIsLeaf = 1;
        nodes_[myIndex].activeChildCount = 1;
        for (int i = 1; i < N; ++i) {
            nodes_[myIndex].SetEmptyChild(i);
        }
        return myIndex;
    }

    std::vector<const BinaryBuildNode*> treelet = { node->left.get(), node->right.get() };

    while (treelet.size() < static_cast<size_t>(N)) {
        int bestCandidate = -1;
        float maxArea = -1.0f;

        for (size_t i = 0; i < treelet.size(); ++i) {
            const auto* cand = treelet[i];
            if (cand && !cand->IsLeaf() && cand->left && cand->right) {
                float area = cand->bounds.SurfaceArea();
                if (area > maxArea) {
                    maxArea = area;
                    bestCandidate = static_cast<int>(i);
                }
            }
        }

        if (bestCandidate < 0) {
            break;
        }

        const auto* toExpand = treelet[bestCandidate];
        treelet[bestCandidate] = toExpand->left.get();
        treelet.push_back(toExpand->right.get());
    }

    int activeCount = static_cast<int>(treelet.size());
    nodes_[myIndex].activeChildCount = static_cast<uint8_t>(activeCount);
    nodes_[myIndex].childIsLeaf = 0;

    for (int i = 0; i < activeCount; ++i) {
        nodes_[myIndex].SetChildBox(i, treelet[i]->bounds);
        if (treelet[i]->IsLeaf()) {
            nodes_[myIndex].childIsLeaf |= static_cast<uint8_t>(1 << i);
            nodes_[myIndex].children[i] = treelet[i]->firstPrimOffset;
            nodes_[myIndex].numPrimitives[i] = treelet[i]->nPrimitives;
        } else {
            int childIdx = CollapseNode(treelet[i]);
            nodes_[myIndex].children[i] = childIdx;
            nodes_[myIndex].numPrimitives[i] = 0;
        }
    }

    for (int i = activeCount; i < N; ++i) {
        nodes_[myIndex].SetEmptyChild(i);
    }

    return myIndex;
}

template <int N>
bool WideBVH<N>::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    if (nodes_.empty()) return false;
    bool hit = false;
    Vector3f invDir(1.0f / ray.d.x, 1.0f / ray.d.y, 1.0f / ray.d.z);
    int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};

    struct StackEntry {
        uint32_t index;
        float dist;
        uint8_t isLeaf;
        uint8_t count;
    };

    StackEntry stack[128];
    int stackTop = 0;
    stack[stackTop++] = {0, 0.0f, 0, 0};

    while (stackTop > 0) {
        StackEntry entry = stack[--stackTop];
        if (entry.dist > ray.tMax) continue;

        if (entry.isLeaf) {
            for (uint32_t p = 0; p < entry.count; ++p) {
                if (orderedShapes_[entry.index + p]->Intersect(ray, isect)) {
                    hit = true;
                }
            }
            continue;
        }

        const auto& node = nodes_[entry.index];
        float tEnter[N];
        int hitMask = IntersectN<N>(node, ray, invDir, dirIsNeg, ray.tMax, tEnter);
        if (hitMask == 0) continue;

        StackEntry hits[N];
        int hitCount = 0;
        for (int lane = 0; lane < N; ++lane) {
            if ((hitMask & (1 << lane)) && tEnter[lane] <= ray.tMax) {
                bool isLeaf = (node.childIsLeaf & (1 << lane)) != 0;
                hits[hitCount++] = {
                    node.children[lane],
                    tEnter[lane],
                    static_cast<uint8_t>(isLeaf ? 1 : 0),
                    static_cast<uint8_t>(isLeaf ? node.numPrimitives[lane] : 0)
                };
            }
        }

        for (int i = 1; i < hitCount; ++i) {
            StackEntry key = hits[i];
            int j = i - 1;
            while (j >= 0 && hits[j].dist < key.dist) {
                hits[j + 1] = hits[j];
                --j;
            }
            hits[j + 1] = key;
        }

        for (int i = 0; i < hitCount; ++i) {
            stack[stackTop++] = hits[i];
        }
    }

    return hit;
}

template <int N>
bool WideBVH<N>::IntersectP(const Ray& ray) const {
    if (nodes_.empty()) return false;
    Vector3f invDir(1.0f / ray.d.x, 1.0f / ray.d.y, 1.0f / ray.d.z);
    int dirIsNeg[3] = {invDir.x < 0, invDir.y < 0, invDir.z < 0};

    struct StackEntry {
        uint32_t index;
        float dist;
        uint8_t isLeaf;
        uint8_t count;
    };

    StackEntry stack[128];
    int stackTop = 0;
    stack[stackTop++] = {0, 0.0f, 0, 0};

    while (stackTop > 0) {
        StackEntry entry = stack[--stackTop];
        if (entry.dist > ray.tMax) continue;

        if (entry.isLeaf) {
            for (uint32_t p = 0; p < entry.count; ++p) {
                if (orderedShapes_[entry.index + p]->IntersectP(ray)) {
                    return true;
                }
            }
            continue;
        }

        const auto& node = nodes_[entry.index];
        float tEnter[N];
        int hitMask = IntersectN<N>(node, ray, invDir, dirIsNeg, ray.tMax, tEnter);
        if (hitMask == 0) continue;

        StackEntry hits[N];
        int hitCount = 0;
        for (int lane = 0; lane < N; ++lane) {
            if ((hitMask & (1 << lane)) && tEnter[lane] <= ray.tMax) {
                bool isLeaf = (node.childIsLeaf & (1 << lane)) != 0;
                hits[hitCount++] = {
                    node.children[lane],
                    tEnter[lane],
                    static_cast<uint8_t>(isLeaf ? 1 : 0),
                    static_cast<uint8_t>(isLeaf ? node.numPrimitives[lane] : 0)
                };
            }
        }

        for (int i = 1; i < hitCount; ++i) {
            StackEntry key = hits[i];
            int j = i - 1;
            while (j >= 0 && hits[j].dist < key.dist) {
                hits[j + 1] = hits[j];
                --j;
            }
            hits[j + 1] = key;
        }

        for (int i = 0; i < hitCount; ++i) {
            stack[stackTop++] = hits[i];
        }
    }

    return false;
}

template <int N>
Bounds3f WideBVH<N>::WorldBound() const {
    if (nodes_.empty()) return Bounds3f();
    Bounds3f bounds;
    for (int i = 0; i < nodes_[0].activeChildCount; ++i) {
        bounds = Union(bounds, nodes_[0].GetChildBox(i));
    }
    return bounds;
}

template <int N>
float WideBVH<N>::AverageChildrenPerNode() const {
    if (nodes_.empty()) return 0.0f;
    size_t totalChildren = 0;
    for (const auto& node : nodes_) {
        totalChildren += node.activeChildCount;
    }
    return static_cast<float>(totalChildren) / static_cast<float>(nodes_.size());
}

template class WideBVH<4>;
template class WideBVH<8>;

}
