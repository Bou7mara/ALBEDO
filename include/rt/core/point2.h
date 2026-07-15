#pragma once
#include "rt/core/tuple2.h"
#include "rt/core/vector2.h"
#include <algorithm>
#include <cmath>
#include <type_traits>

namespace rt {

template <typename T>
class Point2 : public Tuple2<Point2<T>, T> {
public:
    using Tuple2<Point2<T>, T>::Tuple2;

    explicit constexpr Point2(const Vector2<T>& v) : Tuple2<Point2<T>, T>(v.x, v.y) {}

    Point2 operator+(const Point2& other) const = delete;
    Point2& operator+=(const Point2& other) = delete;

    constexpr Point2<T> operator+(const Vector2<T>& v) const {
        return Point2<T>(this->x + v.x, this->y + v.y);
    }

    constexpr Point2<T>& operator+=(const Vector2<T>& v) {
        this->x += v.x;
        this->y += v.y;
        return *this;
    }

    constexpr Point2<T> operator-(const Vector2<T>& v) const {
        return Point2<T>(this->x - v.x, this->y - v.y);
    }

    constexpr Point2<T>& operator-=(const Vector2<T>& v) {
        this->x -= v.x;
        this->y -= v.y;
        return *this;
    }

    constexpr Vector2<T> operator-(const Point2<T>& p) const {
        return Vector2<T>(this->x - p.x, this->y - p.y);
    }
};

using Point2f = Point2<float>;
using Point2d = Point2<double>;
using Point2i = Point2<int>;

template <typename T>
constexpr Point2<T> operator+(const Vector2<T>& v, const Point2<T>& p) {
    return p + v;
}

template <typename T>
constexpr T DistanceSquared(const Point2<T>& p1, const Point2<T>& p2) {
    return LengthSquared(p1 - p2);
}

template <typename T>
T Distance(const Point2<T>& p1, const Point2<T>& p2) {
    static_assert(std::is_floating_point_v<T>, "Distance() requires a floating-point type");
    return Length(p1 - p2);
}

template <typename T>
constexpr Point2<T> Lerp(T t, const Point2<T>& p0, const Point2<T>& p1) {
    static_assert(std::is_floating_point_v<T>, "Lerp() requires a floating-point type");
    return p0 + t * (p1 - p0);
}

template <typename T>
constexpr Point2<T> Floor(const Point2<T>& p) {
    return Point2<T>(std::floor(p.x), std::floor(p.y));
}

template <typename T>
constexpr Point2<T> Ceil(const Point2<T>& p) {
    return Point2<T>(std::ceil(p.x), std::ceil(p.y));
}

template <typename T>
constexpr Point2<T> Min(const Point2<T>& p1, const Point2<T>& p2) {
    return Point2<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
}

template <typename T>
constexpr Point2<T> Max(const Point2<T>& p1, const Point2<T>& p2) {
    return Point2<T>(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
}
}
