#pragma once
#include "rt/core/tuple3.h"
#include <algorithm>
#include <cmath>
#include <type_traits>

// Ray Tracer Core Namespace
namespace rt {
    // ====================================
    // 3D VECTOR CLASS (DIRECTION & OFFSET)
    // ====================================

    // Represents a 3D direction or displacement vector in 3D space.
    // Inherits storage (x,y,z), operators, and CRTP mechanics from Tuple3.
    template <typename T>
    class Vector3 : public Tuple3<Vector3<T>, T> {
    public:
        // Inherit constructors from Tuple3 base class
        using Tuple3<Vector3<T>, T>::Tuple3;
    };

    // --- Type Aliases ---
    using Vector3f = Vector3<float>;  // Single-precision float (workhorse of the renderer)
    using Vector3d = Vector3<double>; // Double-precision float (high precision geometry)
    using Vector3i = Vector3<int>;    // Integer vector (voxel grid coordinates, 3D indexes)

    // ========================
    // VECTOR UTILITY FUNCTIONS
    // ========================

    // Computes squared length ||v||^2 = x^2 + y^2 + z^2.
    // Cheaper than Length() because square roots are expensive!
    template <typename T>
    constexpr T LengthSquared(const Vector3<T>& v) {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    // Computes true Euclidean length ||v|| = sqrt(x^2 + y^2 + z^2).
    template <typename T>
    T Length(const Vector3<T>& v) {
        static_assert(std::is_floating_point_v<T>, "Length() requires a floating-point type");
        return std::sqrt(LengthSquared(v));
    }

    // Returns unit vector parallel to v (length == 1).
    template <typename T>
    Vector3<T> Normalize(const Vector3<T>& v) {
        static_assert(std::is_floating_point_v<T>, "Normalize() requires a floating-point type");
        return v / Length(v);
    }

    // Standard 3D dot product: v1 . v2 = x1*x2 + y1*y2 + z1*z2
    template <typename T>
    constexpr T Dot(const Vector3<T>& v1, const Vector3<T>& v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    // Absolute value of dot product |v1 . v2|.
    template <typename T>
    constexpr T AbsDot(const Vector3<T>& v1, const Vector3<T>& v2) {
        return std::abs(Dot(v1, v2));
    }

    // 3D Cross Product: v1 x v2 (perpendicular vector following right-hand rule)
    // Remember: order matters! Cross(a, b) == -Cross(b, a).
    template <typename T>
    constexpr Vector3<T> Cross(const Vector3<T>& v1, const Vector3<T>& v2) {
        return Vector3<T>(
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        );
    }

    // Component-wise absolute value: (|x|, |y|, |z|)
    template <typename T>
    constexpr Vector3<T> Abs(const Vector3<T>& v) {
        return Vector3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
    }

    // Returns the smallest single scalar value among (x, y, z)
    template <typename T>
    constexpr T MinComponent(const Vector3<T>& v) {
        return std::min({v.x, v.y, v.z});
    }

    // Returns the largest single scalar value among (x, y, z)
    template <typename T>
    constexpr T MaxComponent(const Vector3<T>& v) {
        return std::max({v.x, v.y, v.z});
    }

    // Returns index (0 for X, 1 for Y, 2 for Z) of the component with the maximum magnitude.
    // Crucial for picking BVH splitting axes and finding stable coordinate axes!
    template <typename T>
    constexpr int MaxDimension(const Vector3<T>& v) {
        return (v.x > v.y) ? ((v.x > v.z) ? 0 : 2) : ((v.y > v.z) ? 1 : 2);
    }

    // Reorders vector components according to indices (ix, iy, iz).
    // Example: Permute(v, 1, 2, 0) returns Vector3(v.y, v.z, v.x)
    template <typename T>
    constexpr Vector3<T> Permute(const Vector3<T>& v, int ix, int iy, int iz) {
        return Vector3<T>(v[ix], v[iy], v[iz]);
    }

    // Component-wise vector multiplication (Hadamard product): (a.x*b.x, a.y*b.y, a.z*b.z)
    // Unconventional overload! Not a dot or cross product — used for color attenuation/spectral math.
    template <typename T>
    constexpr Vector3<T> operator*(const Vector3<T>& a, const Vector3<T>& b) {
        return Vector3<T>(a.x * b.x, a.y * b.y, a.z * b.z);
    }
}
