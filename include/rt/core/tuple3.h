#ifndef RT_CORE_TUPLE3_H
#define RT_CORE_TUPLE3_H

#include <iostream>
#include <cmath>
#include <cassert>

namespace rt {

template <typename Derived, typename T>
class Tuple3 {
public:
    T x, y, z;

    constexpr Tuple3() : x(0), y(0), z(0) {}
    constexpr Tuple3(T x, T y, T z) : x(x), y(y), z(z) {}

    // Element access
    constexpr T operator[](int i) const {
        assert(i >= 0 && i < 3);
        return (i == 0) ? x : ((i == 1) ? y : z);
    }

    constexpr T& operator[](int i) {
        assert(i >= 0 && i < 3);
        return (i == 0) ? x : ((i == 1) ? y : z);
    }

    // Unary operators
    constexpr Derived operator-() const {
        return Derived(-x, -y, -z);
    }

    // Binary arithmetic operators
    constexpr Derived operator+(const Derived& other) const {
        return Derived(x + other.x, y + other.y, z + other.z);
    }

    constexpr Derived operator-(const Derived& other) const {
        return Derived(x - other.x, y - other.y, z - other.z);
    }

    constexpr Derived operator*(T scalar) const {
        return Derived(x * scalar, y * scalar, z * scalar);
    }

    constexpr Derived operator/(T scalar) const {
        assert(scalar != 0);
        T inv = static_cast<T>(1) / scalar;
        return Derived(x * inv, y * inv, z * inv);
    }

    // Compound assignment operators
    constexpr Derived& operator+=(const Derived& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& operator-=(const Derived& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& operator*=(T scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return static_cast<Derived&>(*this);
    }

    constexpr Derived& operator/=(T scalar) {
        assert(scalar != 0);
        T inv = static_cast<T>(1) / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
        return static_cast<Derived&>(*this);
    }

    // Equality operators
    constexpr bool operator==(const Derived& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    constexpr bool operator!=(const Derived& other) const {
        return !(*this == other);
    }

    // Checks for NaNs
    bool has_nan() const {
        return std::isnan(x) || std::isnan(y) || std::isnan(z);
    }
};

// Scalar multiplication: scalar * tuple
template <typename Derived, typename T>
constexpr Derived operator*(T scalar, const Tuple3<Derived, T>& tuple) {
    return static_cast<const Derived&>(tuple) * scalar;
}

// Stream insertion
template <typename Derived, typename T>
std::ostream& operator<<(std::ostream& os, const Tuple3<Derived, T>& tuple) {
    os << "[" << tuple.x << ", " << tuple.y << ", " << tuple.z << "]";
    return os;
}

} // namespace rt

#endif // RT_CORE_TUPLE3_H
