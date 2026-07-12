#ifndef RT_CORE_VECTOR2_H
#define RT_CORE_VECTOR2_H

#include <iostream>
#include <cmath>
#include <cassert>
#include <algorithm>

namespace rt {

template <typename T>
class Vector2 {
public:
    T x, y;

    constexpr Vector2() : x(0), y(0) {}
    constexpr Vector2(T x, T y) : x(x), y(y) {
        assert(!HasNaN());
    }

    constexpr bool HasNaN() const {
        return std::isnan(x) || std::isnan(y);
    }

    // Element access
    constexpr T operator[](int i) const {
        assert(i >= 0 && i < 2);
        return (i == 0) ? x : y;
    }

    constexpr T& operator[](int i) {
        assert(i >= 0 && i < 2);
        return (i == 0) ? x : y;
    }

    // Unary operators
    constexpr Vector2 operator-() const {
        return Vector2(-x, -y);
    }

    // Binary arithmetic operators
    constexpr Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    constexpr Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    constexpr Vector2 operator*(T scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    constexpr Vector2 operator/(T scalar) const {
        assert(scalar != 0);
        T inv = static_cast<T>(1) / scalar;
        return Vector2(x * inv, y * inv);
    }

    // Compound assignment
    constexpr Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vector2& operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vector2& operator*=(T scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vector2& operator/=(T scalar) {
        assert(scalar != 0);
        T inv = static_cast<T>(1) / scalar;
        x *= inv;
        y *= inv;
        return *this;
    }

    // Equality
    constexpr bool operator==(const Vector2& other) const {
        return x == other.x && y == other.y;
    }

    constexpr bool operator!=(const Vector2& other) const {
        return !(*this == other);
    }

    constexpr T LengthSquared() const {
        return x * x + y * y;
    }

    T Length() const {
        return std::sqrt(LengthSquared());
    }

    Vector2 Normalize() const {
        return *this / Length();
    }
};

// Typedefs for convenience
using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;
using Vector2i = Vector2<int>;

// Free Functions
template <typename T>
constexpr Vector2<T> operator*(T scalar, const Vector2<T>& v) {
    return v * scalar;
}

template <typename T>
constexpr T dot(const Vector2<T>& v1, const Vector2<T>& v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

template <typename T>
constexpr T absDot(const Vector2<T>& v1, const Vector2<T>& v2) {
    return std::abs(dot(v1, v2));
}

template <typename T>
constexpr Vector2<T> abs(const Vector2<T>& v) {
    return Vector2<T>(std::abs(v.x), std::abs(v.y));
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Vector2<T>& v) {
    os << "[" << v.x << ", " << v.y << "]";
    return os;
}

} // namespace rt

#endif // RT_CORE_VECTOR2_H
