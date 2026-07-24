#pragma once
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/ray.h"
#include <algorithm>
#include <cmath>
#include <limits>

// My Ray Tracer's Core Namespace
namespace rt {

    // =============================================
    // 3D AXIS-ALIGNED BOUNDING BOX (AABB / Bounds3)
    // =============================================

    // Represent 3D blocks, slab method n shit (used for BVH)

    template <typename T>
    class Bounds3 {
    public:
        // --- Data Members ---
        Point3<T> minPt, maxPt; // Minimum corner (lowest x,y,z). Maximum corner (highest x,y,z)

        // ------------
        // CONSTRUCTORS
        // ------------

        // Default constructor: Initializes an empty (sorta inside out) bounding box.
        // Counterintuitive trick: minPt is set to ~(+3.4x10^38/+1.7x10^308), maxPt to ~(-3.4x10^38/-1.7x10^304)
        // Why? So Union(empty_box, point) or Union(empty_box, box) immediately contracts around the new bounds
        Bounds3() {
            T minNum = std::numeric_limits<T>::lowest();
            T maxNum = std::numeric_limits<T>::max();
            minPt = Point3<T>(maxNum, maxNum, maxNum);
            maxPt = Point3<T>(minNum, minNum, minNum);
        }

        // Degenerate single-point bounding box
        explicit Bounds3(const Point3<T>& p) : minPt(p), maxPt(p) {}

        // Construct bounds enclosing two arbitrary points p1 and p2
        Bounds3(const Point3<T>& p1, const Point3<T>& p2)
            : minPt(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::min(p1.z, p2.z)),
              maxPt(std::max(p1.x, p2.x), std::max(p1.y, p2.y), std::max(p1.z, p2.z)) {}

        // ----------------------
        // BOUNDS QUERY UTILITIES
        // ----------------------

        // Diagonal vector extending from minPt to maxPt
        Vector3<T> Diagonal() const { return maxPt - minPt; }

        // Total surface area of the 6 box faces: 2 * (w*h + h*d + d*w).
        // Heavily used by BVH construction to evaluate SAH (Surface Area Heuristic) cost.
        T SurfaceArea() const {
            Vector3<T> d = Diagonal();
            return static_cast<T>(2) * (d.x * d.y + d.y * d.z + d.z * d.x);
        }

        // Returns index of the longest axis (0 = X, 1 = Y, 2 = Z).
        // Tells the BVH builder where to slice this node into left & right children.
        int MaxExtent() const {
            Vector3<T> d = Diagonal();
            if (d.x > d.y && d.x > d.z) return 0;
            if (d.y > d.z) return 1;
            return 2;
        }

        // Center point of the bounding box
        Point3<T> Centroid() const { return minPt + Diagonal() * static_cast<T>(0.5); }

        // ----------------------------------
        // RAY - AABB INTERSECTION ALGORITHMS
        // ----------------------------------

        // Standard Kay-Kajiya Slab Method Ray-AABB intersection test.
        // Tests ray overlap with 3 parallel slab pairs [tNear, tFar] for X, Y, Z axes.
        bool IntersectP(const Ray& ray, float* hitt0 = nullptr, float* hitt1 = nullptr) const {
            float t0 = 0.0f, t1 = ray.tMax;
            for (int i = 0; i < 3; ++i) {
                // Division by zero yields +/- infinity in IEEE 754 float math, which branchlessly works
                float invRayDir = 1.0f / ray.d[i];
                float tNear = (minPt[i] - ray.o[i]) * invRayDir;
                float tFar  = (maxPt[i] - ray.o[i]) * invRayDir;
                if (tNear > tFar) std::swap(tNear, tFar);
                t0 = tNear > t0 ? tNear : t0;
                t1 = tFar < t1 ? tFar : t1;
                if (t0 > t1) return false;
                }
            if (hitt0) *hitt0 = t0;
            if (hitt1) *hitt1 = t1;
            return true;
        }

        // fast BVH traversal intersection overload
        // Using precomputed inverse ray direction (invDir = 1/d) and direction sign flags (dirIsNeg[3]).
        // Avoids branch mispredictions and std::swap calls inside heavy loops
        bool IntersectP(const Ray& ray, const Vector3<T>& invDir, const int dirIsNeg[3]) const {
            // Check X slab
            float tMin  = ((dirIsNeg[0] ? maxPt.x : minPt.x) - ray.o.x) * invDir.x;
            float tMax  = ((dirIsNeg[0] ? minPt.x : maxPt.x) - ray.o.x) * invDir.x;
            // Check Y slab
            float tyMin = ((dirIsNeg[1] ? maxPt.y : minPt.y) - ray.o.y) * invDir.y;
            float tyMax = ((dirIsNeg[1] ? minPt.y : maxPt.y) - ray.o.y) * invDir.y;
            if (tMin > tyMax || tyMin > tMax) return false;
            if (tyMin > tMin) tMin = tyMin;
            if (tyMax < tMax) tMax = tyMax;

            // Check Z slab
            float tzMin = ((dirIsNeg[2] ? maxPt.z : minPt.z) - ray.o.z) * invDir.z;
            float tzMax = ((dirIsNeg[2] ? minPt.z : maxPt.z) - ray.o.z) * invDir.z;
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

    // Make a bounding box enclosing that is the union of two boxes b1 and b2
    template <typename T>
    Bounds3<T> Union(const Bounds3<T>& b1, const Bounds3<T>& b2) {
        return Bounds3<T>(
            Point3<T>(std::min(b1.minPt.x, b2.minPt.x), std::min(b1.minPt.y, b2.minPt.y), std::min(b1.minPt.z, b2.minPt.z)),
            Point3<T>(std::max(b1.maxPt.x, b2.maxPt.x), std::max(b1.maxPt.y, b2.maxPt.y), std::max(b1.maxPt.z, b2.maxPt.z)));
    }

    // Expand bounding box b to include point p
    template <typename T>
    Bounds3<T> Union(const Bounds3<T>& b, const Point3<T>& p) {
        return Bounds3<T>(
            Point3<T>(std::min(b.minPt.x, p.x), std::min(b.minPt.y, p.y), std::min(b.minPt.z, p.z)),
            Point3<T>(std::max(b.maxPt.x, p.x), std::max(b.maxPt.y, p.y), std::max(b.maxPt.z, p.z)));
    }
}
