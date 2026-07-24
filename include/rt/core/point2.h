#pragma once
#include "rt/core/tuple2.h"
#include "rt/core/vector2.h"
#include <algorithm>
#include <cmath>
#include <type_traits>

// My Ray Tracer's Core Namespace
namespace rt {

    // =================================
    // 2D POINT CLASS (SPATIAL POSITION)
    // =================================

    // Represents a fixed location in 2D space.
    // Unlike Vector2 (directions), points follow Affine Geometry rules.
    template <typename T>
    class Point2 : public Tuple2<Point2<T>, T> {
    public:
        // Inherit constructors from Tuple2
        using Tuple2<Point2<T>, T>::Tuple2;

        // Explicit constructor from a Vector2 (converting direction/offset to coordinate)
        explicit constexpr Point2(const Vector2<T>& v) : Tuple2<Point2<T>, T>(v.x, v.y) {}

        // ---------------------------------------------------------
        // AFFINE GEOMETRY RULES (INTENTIONAL OPERATOR RESTRICTIONS)
        // ---------------------------------------------------------

        // Adding two points (Paris + London) makes no physical sense.
        // We explicitly delete these operators so the compiler slaps your wrist if you try.
        Point2 operator+(const Point2& other) const = delete;
        Point2& operator+=(const Point2& other) = delete;

        // Point + Vector = New Point (Translates position by displacement vector)
        constexpr Point2<T> operator+(const Vector2<T>& v) const {
            return Point2<T>(this->x + v.x, this->y + v.y);
        }

        // Point += Vector (In-place translation)
        constexpr Point2<T>& operator+=(const Vector2<T>& v) {
            this->x += v.x;
            this->y += v.y;
            return *this;
        }

        // Point - Vector = New Point (Translates position backward by displacement vector)
        constexpr Point2<T> operator-(const Vector2<T>& v) const {
            return Point2<T>(this->x - v.x, this->y - v.y);
        }

        // Point -= Vector (In-place backward translation)
        constexpr Point2<T>& operator-=(const Vector2<T>& v) {
            this->x -= v.x;
            this->y -= v.y;
            return *this;
        }

        // Point - Point = Vector2 (Displacement vector pointing from p to *this)
        constexpr Vector2<T> operator-(const Point2<T>& p) const {
            return Vector2<T>(this->x - p.x, this->y - p.y);
        }
    };

    // --- Type Aliases ---
    // Sub-pixel coordinates, UV texture mapping
    using Point2f = Point2<float>;
    // High precision coordinates
    using Point2d = Point2<double>;
    // Pixel raster grid locations (discrete coordinates)
    using Point2i = Point2<int>;

    // Commutative vector-point addition: vec + point == point + vec
    template <typename T>
    constexpr Point2<T> operator+(const Vector2<T>& v, const Point2<T>& p) {
        return p + v;
    }

    // =======================
    // POINT UTILITY FUNCTIONS
    // =======================

    // Squared distance between two 2D points. Saves sqrt CPU cycles.
    template <typename T>
    constexpr T DistanceSquared(const Point2<T>& p1, const Point2<T>& p2) {
        return LengthSquared(p1 - p2);
    }

    // True Euclidean distance between two 2D points. Floating-point only.
    template <typename T>
    T Distance(const Point2<T>& p1, const Point2<T>& p2) {
        static_assert(std::is_floating_point_v<T>, "Distance() requires a floating-point type");
        return Length(p1 - p2);
    }

    // Linear Interpolation between p0 (t=0) and p1 (t=1): p0 + t * (p1 - p0)
    template <typename T>
    constexpr Point2<T> Lerp(T t, const Point2<T>& p0, const Point2<T>& p1) {
        static_assert(std::is_floating_point_v<T>, "Lerp() requires a floating-point type");
        return p0 + t * (p1 - p0);
    }

    // Component-wise floor: (floor(x), floor(y))
    template <typename T>
    constexpr Point2<T> Floor(const Point2<T>& p) {
        return Point2<T>(std::floor(p.x), std::floor(p.y));
    }

    // Component-wise ceiling: (ceil(x), ceil(y))
    template <typename T>
    constexpr Point2<T> Ceil(const Point2<T>& p) {
        return Point2<T>(std::ceil(p.x), std::ceil(p.y));
    }

    // Component-wise minimum bounding point
    template <typename T>
    constexpr Point2<T> Min(const Point2<T>& p1, const Point2<T>& p2) {
        return Point2<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
    }

    // Component-wise maximum bounding point
    template <typename T>
    constexpr Point2<T> Max(const Point2<T>& p1, const Point2<T>& p2) {
        return Point2<T>(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
    }
}
