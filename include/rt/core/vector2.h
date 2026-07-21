#pragma once
#include "rt/core/tuple2.h"
#include <cmath>
#include <algorithm>
#include <type_traits>

// Ray Tracer Core Namespace
namespace rt {
    // ====================================
    // 2D VECTOR CLASS (DIRECTION & OFFSET)
    // ====================================

    // Represents a 2D direction or displacement vector.
    // Inherits arithmetic operations, storage (x,y), and CRTP goodness from Tuple2.
    template <typename T>
    class Vector2 : public Tuple2<Vector2<T>, T> {
    public:
        // Inherit constructors from Tuple2 base class
        using Tuple2<Vector2<T>, T>::Tuple2;
    };

    // --- Type Aliases ---
    using Vector2f = Vector2<float>;  // Single-precision (standard for GPU/rendering)
    using Vector2d = Vector2<double>; // Double-precision (for ultra-high precision math)
    using Vector2i = Vector2<int>;    // Integer 2D vector (for pixel coordinates / grid indices)

    // ========================
    // VECTOR UTILITY FUNCTIONS
    // ========================

    // Computes squared length ||v||^2 = x^2 + y^2.
    // Pro tip: Avoids sqrt()! Use this for distance comparisons to save CPU cycles.
    template <typename T>
    constexpr T LengthSquared(const Vector2<T>& v) {
        return v.x * v.x + v.y * v.y;
    }

    // Computes true Euclidean length ||v|| = sqrt(x^2 + y^2).
    // Floating-point only — taking the square root of an integer vector is a recipe for heartbreak.
    template <typename T>
    T Length(const Vector2<T>& v) {
        static_assert(std::is_floating_point_v<T>, "Length() requires a floating-point type");
        return std::sqrt(LengthSquared(v));
    }

    // Returns a unit vector in the same direction (length == 1).
    // Division by zero risk if v is a zero vector — handle with care in physics/sampling!
    template <typename T>
    Vector2<T> Normalize(const Vector2<T>& v) {
        static_assert(std::is_floating_point_v<T>, "Normalize() requires a floating-point type");
        return v / Length(v);
    }

    // Standard 2D dot product: v1 . v2 = v1.x*v2.x + v1.y*v2.y
    // Gives cos(theta) when both vectors are unit length!
    template <typename T>
    constexpr T Dot(const Vector2<T>& v1, const Vector2<T>& v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }

    // Absolute value of the dot product |v1 . v2|.
    // Useful for surface shading when light can shine from either side!
    template <typename T>
    constexpr T AbsDot(const Vector2<T>& v1, const Vector2<T>& v2) {
        return std::abs(Dot(v1, v2));
    }

    // Component-wise absolute value: (|x|, |y|)
    template <typename T>
    constexpr Vector2<T> Abs(const Vector2<T>& v) {
        return Vector2<T>(std::abs(v.x), std::abs(v.y));
    }
}
