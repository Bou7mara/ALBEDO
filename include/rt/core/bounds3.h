#pragma once
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/ray.h"
#include <algorithm>
#include <limits>

namespace rt {

template <typename T>
class Bounds3 {
public:
    Point3<T> pMin, pMax;

    // Empty/invalid bounds: pMin = +inf, pMax = -inf per axis, so that
    // Union()-ing anything into a default-constructed Bounds3 always
    // absorbs it correctly, with no special-cased "first union" branch
    // anywhere that builds one up incrementally (exactly how BVH
    // construction accumulates bounds over a primitive range below).
    Bounds3() {
        T minNum = std::numeric_limits<T>::lowest();
        T maxNum = std::numeric_limits<T>::max();
        pMin = Point3<T>(maxNum, maxNum, maxNum);
        pMax = Point3<T>(minNum, minNum, minNum);
    }

    explicit Bounds3(const Point3<T>& p) : pMin(p), pMax(p) {}

    // p1/p2 need NOT be pre-ordered -- sorted component-wise here, so
    // callers never have to think about which corner is "min".
    Bounds3(const Point3<T>& p1, const Point3<T>& p2)
        : pMin(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::min(p1.z, p2.z)),
          pMax(std::max(p1.x, p2.x), std::max(p1.y, p2.y), std::max(p1.z, p2.z)) {}

    Vector3<T> Diagonal() const { return pMax - pMin; }

    T SurfaceArea() const {
        Vector3<T> d = Diagonal();
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    // Returns 0/1/2 for the longest axis (x/y/z) -- the axis BVH
    // construction splits along.
    int MaxExtent() const {
        Vector3<T> d = Diagonal();
        if (d.x > d.y && d.x > d.z) return 0;
        if (d.y > d.z) return 1;
        return 2;
    }

    Point3<T> Centroid() const { return pMin + Diagonal() * static_cast<T>(0.5); }

    // General-purpose slab test. Simple, standard form -- note the one
    // known caveat: an axis-aligned ray landing EXACTLY on a slab
    // boundary can hit a 0 * inf = NaN case in the invRayDir
    // computation. This is a well-known, accepted limitation of the
    // basic slab test (pbrt's own default form has the same property);
    // the more involved "watertight" variant that closes this gap is
    // real future work, not something worth solving preemptively here
    // -- same judgment call as leaving Quadratic's edge cases as
    // numerically careful but not exhaustively bulletproof.
    bool IntersectP(const Ray& ray, float* hitt0 = nullptr, float* hitt1 = nullptr) const {
        float t0 = 0.0f, t1 = ray.tMax;
        for (int i = 0; i < 3; ++i) {
            float invRayDir = 1.0f / ray.d[i];
            float tNear = (pMin[i] - ray.o[i]) * invRayDir;
            float tFar  = (pMax[i] - ray.o[i]) * invRayDir;
            if (tNear > tFar) std::swap(tNear, tFar);
            t0 = tNear > t0 ? tNear : t0;
            t1 = tFar < t1 ? tFar : t1;
            if (t0 > t1) return false;
        }
        if (hitt0) *hitt0 = t0;
        if (hitt1) *hitt1 = t1;
        return true;
    }

    // BVH-traversal-optimized overload: precomputed inverse ray
    // direction and sign, so a multi-node tree walk doesn't recompute
    // 1/d.x/y/z at every single node. This is pbrt's standard
    // optimized slab-test form.
    bool IntersectP(const Ray& ray, const Vector3<T>& invDir, const int dirIsNeg[3]) const {
        float tMin  = ((dirIsNeg[0] ? pMax.x : pMin.x) - ray.o.x) * invDir.x;
        float tMax  = ((dirIsNeg[0] ? pMin.x : pMax.x) - ray.o.x) * invDir.x;
        float tyMin = ((dirIsNeg[1] ? pMax.y : pMin.y) - ray.o.y) * invDir.y;
        float tyMax = ((dirIsNeg[1] ? pMin.y : pMax.y) - ray.o.y) * invDir.y;
        if (tMin > tyMax || tyMin > tMax) return false;
        if (tyMin > tMin) tMin = tyMin;
        if (tyMax < tMax) tMax = tyMax;

        float tzMin = ((dirIsNeg[2] ? pMax.z : pMin.z) - ray.o.z) * invDir.z;
        float tzMax = ((dirIsNeg[2] ? pMin.z : pMax.z) - ray.o.z) * invDir.z;
        if (tMin > tzMax || tzMin > tMax) return false;
        if (tzMin > tMin) tMin = tzMin;
        if (tzMax < tMax) tMax = tzMax;

        return (tMin < ray.tMax) && (tMax > 0.0f);
    }
};

using Bounds3f = Bounds3<float>;

template <typename T>
Bounds3<T> Union(const Bounds3<T>& b1, const Bounds3<T>& b2) {
    return Bounds3<T>(
        Point3<T>(std::min(b1.pMin.x, b2.pMin.x), std::min(b1.pMin.y, b2.pMin.y), std::min(b1.pMin.z, b2.pMin.z)),
        Point3<T>(std::max(b1.pMax.x, b2.pMax.x), std::max(b1.pMax.y, b2.pMax.y), std::max(b1.pMax.z, b2.pMax.z)));
}

template <typename T>
Bounds3<T> Union(const Bounds3<T>& b, const Point3<T>& p) {
    return Bounds3<T>(
        Point3<T>(std::min(b.pMin.x, p.x), std::min(b.pMin.y, p.y), std::min(b.pMin.z, p.z)),
        Point3<T>(std::max(b.pMax.x, p.x), std::max(b.pMax.y, p.y), std::max(b.pMax.z, p.z)));
}
}
