#pragma once
#include "rt/core/tuple3.h"
#include "rt/core/vector3.h"
#include <algorithm>
#include <cmath>
#include <type_traits>

// My Ray Tracer's Core Namespace
namespace rt {

    // =================================
    // 3D POINT CLASS (SPATIAL POSITION)
    // =================================

    // Represents a fixed location in 3D world space.
    // Follows strict Affine Geometry rules (Points != Vectors).
    template <typename T>
    class Point3 : public Tuple3<Point3<T>, T> {
    public:
        // Inherit constructors from Tuple3 base class
        using Tuple3<Point3<T>, T>::Tuple3;

        // Explicit conversion from 3D direction vector to 3D point location
        explicit constexpr Point3(const Vector3<T>& v) : Tuple3<Point3<T>, T>(v.x, v.y, v.z) {}

        // ---------------------------------------------------------
        // AFFINE GEOMETRY RULES (INTENTIONAL OPERATOR RESTRICTIONS)
        // ---------------------------------------------------------

        // Adding two 3D points in space is mathematically illegal (no origin reference point).
        // Deleting these operators catches illegal spatial math at compile time.
        Point3 operator+(const Point3& other) const = delete;
        Point3& operator+=(const Point3& other) = delete;

        // Point + Vector = New Point (Translates position along direction vector)
        constexpr Point3<T> operator+(const Vector3<T>& v) const {
            return Point3<T>(this->x + v.x, this->y + v.y, this->z + v.z);
        }

        // Point += Vector (In-place spatial translation)
        constexpr Point3<T>& operator+=(const Vector3<T>& v) {
            this->x += v.x;
            this->y += v.y;
            this->z += v.z;
            return *this;
        }

        // Point - Vector = New Point (Translates position in opposite direction of vector)
        constexpr Point3<T> operator-(const Vector3<T>& v) const {
            return Point3<T>(this->x - v.x, this->y - v.y, this->z - v.z);
        }

        // Point -= Vector (In-place reverse translation)
        constexpr Point3<T>& operator-=(const Vector3<T>& v) {
            this->x -= v.x;
            this->y -= v.y;
            this->z -= v.z;
            return *this;
        }

        // Point - Point = Vector3 (Yields displacement vector pointing from p to *this)
        constexpr Vector3<T> operator-(const Point3<T>& p) const {
            return Vector3<T>(this->x - p.x, this->y - p.y, this->z - p.z);
        }
    };

    // --- Type Aliases ---
    using Point3f = Point3<float>;  // Standard single-precision 3D point (world coordinates)
    using Point3d = Point3<double>; // High-precision 3D point (astronomy / planetary scale rendering)
    using Point3i = Point3<int>;    // Discrete 3D spatial voxel grid position

    // Commutative vector-point addition: vec + point == point + vec
    template <typename T>
    constexpr Point3<T> operator+(const Vector3<T>& v, const Point3<T>& p) {
        return p + v;
    }

    // =======================
    // POINT UTILITY FUNCTIONS
    // =======================

    // Squared Euclidean distance between two 3D points (fast, no sqrt).
    template <typename T>
    constexpr T DistanceSquared(const Point3<T>& p1, const Point3<T>& p2) {
        return LengthSquared(p1 - p2);
    }

    // True Euclidean distance between two 3D points. Requires floating-point T.
    template <typename T>
    T Distance(const Point3<T>& p1, const Point3<T>& p2) {
        static_assert(std::is_floating_point_v<T>, "Distance() requires a floating-point type");
        return Length(p1 - p2);
    }

    // Linear Interpolation in 3D: p0 + t * (p1 - p0)
    template <typename T>
    constexpr Point3<T> Lerp(T t, const Point3<T>& p0, const Point3<T>& p1) {
        static_assert(std::is_floating_point_v<T>, "Lerp() requires a floating-point type");
        return p0 + t * (p1 - p0);
    }

    // Component-wise floor: (floor(x), floor(y), floor(z))
    template <typename T>
    constexpr Point3<T> Floor(const Point3<T>& p) {
        return Point3<T>(std::floor(p.x), std::floor(p.y), std::floor(p.z));
    }

    // Component-wise ceiling: (ceil(x), ceil(y), ceil(z))
    template <typename T>
    constexpr Point3<T> Ceil(const Point3<T>& p) {
        return Point3<T>(std::ceil(p.x), std::ceil(p.y), std::ceil(p.z));
    }

    // Component-wise minimum bounding point
    template <typename T>
    constexpr Point3<T> Min(const Point3<T>& p1, const Point3<T>& p2) {
        return Point3<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::min(p1.z, p2.z));
    }

    // Component-wise maximum bounding point
    template <typename T>
    constexpr Point3<T> Max(const Point3<T>& p1, const Point3<T>& p2) {
        return Point3<T>(std::max(p1.x, p2.x), std::max(p1.y, p2.y), std::max(p1.z, p2.z));
    }

    // Reorders point coordinates according to indices (ix, iy, iz).
    // Useful for axis swaps and spatial projections.
    template <typename T>
    constexpr Point3<T> Permute(const Point3<T>& p, int ix, int iy, int iz) {
        return Point3<T>(p[ix], p[iy], p[iz]);
    }
}
