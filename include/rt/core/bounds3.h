#pragma once
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/ray.h"
#include <algorithm>
#include <cmath>
#include <limits>

// Ray Tracer Core Namespace
namespace rt {
    // =============================================
    // 3D AXIS-ALIGNED BOUNDING BOX (AABB / Bounds3)
    // =============================================

    // Represents a 3D box aligned with the coordinate axes.
    // fundamental building block for BVH (Bounding Volume Hierarchy) spatial acceleration!
    template <typename T>
    class Bounds3 {
    public:
        // --- Data Members ---
        Point3<T> pMin, pMax; // Min corner (lowest x,y,z) and Max corner (highest x,y,z)

        // ------------
        // CONSTRUCTORS
        // ------------

        // Default constructor: Initializes an EMPTY bounding box.
        // Counter-intuitive trick: pMin is set to + infinity, pMax to - infinity!
        // Why? So Union(empty_box, point) or Union(empty_box, box) immediately contracts around the new bounds!
        Bounds3() {
            T minNum = std::numeric_limits<T>::lowest();
            T maxNum = std::numeric_limits<T>::max();
            pMin = Point3<T>(maxNum, maxNum, maxNum);
            pMax = Point3<T>(minNum, minNum, minNum);
        }

        // Degenerate single-point bounding box
        explicit Bounds3(const Point3<T>& p) : pMin(p), pMax(p) {}

        // Construct bounds enclosing two arbitrary points p1 and p2
        Bounds3(const Point3<T>& p1, const Point3<T>& p2)
            : pMin(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::min(p1.z, p2.z)),
              pMax(std::max(p1.x, p2.x), std::max(p1.y, p2.y), std::max(p1.z, p2.z)) {}

        // ----------------------
        // BOUNDS QUERY UTILITIES
        // ----------------------

        // Diagonal vector extending from pMin to pMax
        Vector3<T> Diagonal() const { return pMax - pMin; }

        // Total surface area of the 6 box faces: 2 * (w*h + h*d + d*w).
        // Heavily used by BVH construction to evaluate SAH (Surface Area Heuristic) cost!
        T SurfaceArea() const {
            Vector3<T> d = Diagonal();
            return static_cast<T>(2) * (d.x * d.y + d.y * d.z + d.z * d.x);
        }

        // Returns index of the longest axis (0 = X, 1 = Y, 2 = Z).
        // Tells the BVH builder where to slice this node into left & right children!
        int MaxExtent() const {
            Vector3<T> d = Diagonal();
            if (d.x > d.y && d.x > d.z) return 0;
            if (d.y > d.z) return 1;
            return 2;
        }

        // Center point of the bounding box
        Point3<T> Centroid() const { return pMin + Diagonal() * static_cast<T>(0.5); }

        // ----------------------------------
        // RAY - AABB INTERSECTION ALGORITHMS
        // ----------------------------------

        // Standard Kay-Kajiya Slab Method Ray-AABB intersection test.
        // Tests ray overlap with 3 parallel slab pairs [tNear, tFar] for X, Y, Z axes.
        bool IntersectP(const Ray& ray, float* hitt0 = nullptr, float* hitt1 = nullptr) const {
            float t0 = 0.0f, t1 = ray.tMax;
            for (int i = 0; i < 3; ++i) {
                // Division by zero yields +/- infinity in IEEE 754 float math, which branchlessly works!
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

        // Ultra-fast BVH traversal intersection overload!
        // Uses precomputed inverse ray direction (invDir = 1/d) and direction sign flags (dirIsNeg[3]).
        // Avoids branch mispredictions and std::swap calls inside heavy rendering inner loops!
        bool IntersectP(const Ray& ray, const Vector3<T>& invDir, const int dirIsNeg[3]) const {
            // Check X slab
            float tMin  = ((dirIsNeg[0] ? pMax.x : pMin.x) - ray.o.x) * invDir.x;
            float tMax  = ((dirIsNeg[0] ? pMin.x : pMax.x) - ray.o.x) * invDir.x;
            // Check Y slab
            float tyMin = ((dirIsNeg[1] ? pMax.y : pMin.y) - ray.o.y) * invDir.y;
            float tyMax = ((dirIsNeg[1] ? pMin.y : pMax.y) - ray.o.y) * invDir.y;
            if (tMin > tyMax || tyMin > tMax) return false;
            if (tyMin > tMin) tMin = tyMin;
            if (tyMax < tMax) tMax = tyMax;

            // Check Z slab
            float tzMin = ((dirIsNeg[2] ? pMax.z : pMin.z) - ray.o.z) * invDir.z;
            float tzMax = ((dirIsNeg[2] ? pMin.z : pMax.z) - ray.o.z) * invDir.z;
            if (tMin > tzMax || tzMin > tMax) return false;
            if (tzMin > tMin) tMin = tzMin;
            if (tzMax < tMax) tMax = tzMax;

            return (tMin < ray.tMax) && (tMax > 0.0f);
        }
    };

    // --- Type Alias ---
    using Bounds3f = Bounds3<float>; // Standard bounding box type

    // =========================
    // GLOBAL BOUNDS UNION UTILS
    // =========================

    // Computes bounding box enclosing the union of two boxes b1 and b2
    template <typename T>
    Bounds3<T> Union(const Bounds3<T>& b1, const Bounds3<T>& b2) {
        return Bounds3<T>(
            Point3<T>(std::min(b1.pMin.x, b2.pMin.x), std::min(b1.pMin.y, b2.pMin.y), std::min(b1.pMin.z, b2.pMin.z)),
            Point3<T>(std::max(b1.pMax.x, b2.pMax.x), std::max(b1.pMax.y, b2.pMax.y), std::max(b1.pMax.z, b2.pMax.z)));
    }

    // Expands bounding box b to include point p
    template <typename T>
    Bounds3<T> Union(const Bounds3<T>& b, const Point3<T>& p) {
        return Bounds3<T>(
            Point3<T>(std::min(b.pMin.x, p.x), std::min(b.pMin.y, p.y), std::min(b.pMin.z, p.z)),
            Point3<T>(std::max(b.pMax.x, p.x), std::max(b.pMax.y, p.y), std::max(b.pMax.z, p.z)));
    }
}
