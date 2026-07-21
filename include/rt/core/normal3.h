#pragma once
#include "rt/core/tuple3.h"
#include "rt/core/vector3.h"
#include <cmath>
#include <type_traits>

// Ray Tracer Core Namespace
namespace rt {
    // ===========================================
    // 3D SURFACE NORMAL CLASS (PERPENDICULAR VEC)
    // ===========================================

    // Represents a vector perpendicular to a surface at a specific point.
    // Important math distinction: Normals are NOT vectors! When transformed by a matrix M,
    // vectors use M while normals MUST use (M^-1)^T (inverse transpose) to remain perpendicular.
    template <typename T>
    class Normal3 : public Tuple3<Normal3<T>, T> {
    public:
        // Inherit constructors from Tuple3 base class
        using Tuple3<Normal3<T>, T>::Tuple3;

        // Explicit constructor from Vector3 (dangerous if not normalized/oriented properly!)
        explicit constexpr Normal3(const Vector3<T>& v) : Tuple3<Normal3<T>, T>(v.x, v.y, v.z) {}

        // Explicit conversion operator to Vector3 when needed for vector arithmetic
        explicit constexpr operator Vector3<T>() const {
            return Vector3<T>(this->x, this->y, this->z);
        }
    };

    // --- Type Aliases ---
    using Normal3f = Normal3<float>;  // Standard single-precision surface normal
    using Normal3d = Normal3<double>; // High-precision surface normal

    // ============================
    // NORMAL UTILITY FUNCTIONS
    // ============================

    // Computes squared norm ||n||^2 = x^2 + y^2 + z^2
    template <typename T>
    constexpr T LengthSquared(const Normal3<T>& n) {
        return n.x * n.x + n.y * n.y + n.z * n.z;
    }

    // Computes Euclidean length ||n||
    template <typename T>
    T Length(const Normal3<T>& n) {
        static_assert(std::is_floating_point_v<T>, "Length() requires a floating-point type");
        return std::sqrt(LengthSquared(n));
    }

    // Normalizes a surface normal so ||n|| == 1
    template <typename T>
    Normal3<T> Normalize(const Normal3<T>& n) {
        static_assert(std::is_floating_point_v<T>, "Normalize() requires a floating-point type");
        return n / Length(n);
    }

    // ---------------------------------------------
    // DOT PRODUCTS (MIXED NORMAL & VECTOR OPERANDS)
    // ---------------------------------------------

    // Dot product: Normal . Vector
    template <typename T>
    constexpr T Dot(const Normal3<T>& n, const Vector3<T>& v) {
        return n.x * v.x + n.y * v.y + n.z * v.z;
    }

    // Dot product: Vector . Normal (Commutative)
    template <typename T>
    constexpr T Dot(const Vector3<T>& v, const Normal3<T>& n) {
        return v.x * n.x + v.y * n.y + v.z * n.z;
    }

    // Dot product: Normal . Normal
    template <typename T>
    constexpr T Dot(const Normal3<T>& n1, const Normal3<T>& n2) {
        return n1.x * n2.x + n1.y * n2.y + n1.z * n2.z;
    }

    // Absolute dot product: |Normal . Vector|
    template <typename T>
    constexpr T AbsDot(const Normal3<T>& n, const Vector3<T>& v) {
        return std::abs(Dot(n, v));
    }

    // Absolute dot product: |Vector . Normal|
    template <typename T>
    constexpr T AbsDot(const Vector3<T>& v, const Normal3<T>& n) {
        return std::abs(Dot(v, n));
    }

    // Absolute dot product: |Normal . Normal|
    template <typename T>
    constexpr T AbsDot(const Normal3<T>& n1, const Normal3<T>& n2) {
        return std::abs(Dot(n1, n2));
    }

    // ---------------------
    // ORIENTATION UTILITIES
    // ---------------------

    // Flips normal n to point into the same hemisphere as vector v if n . v < 0
    // Crucial for double-sided material rendering!
    template <typename T>
    constexpr Normal3<T> FaceForward(const Normal3<T>& n, const Vector3<T>& v) {
        return (Dot(n, v) < static_cast<T>(0)) ? -n : n;
    }

    // Flips normal n to point into the same hemisphere as reference normal n2
    template <typename T>
    constexpr Normal3<T> FaceForward(const Normal3<T>& n, const Normal3<T>& n2) {
        return (Dot(n, n2) < static_cast<T>(0)) ? -n : n;
    }
}
