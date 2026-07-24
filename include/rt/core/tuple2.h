#pragma once
#include <iostream>
#include <cmath>
#include <cassert>
#include <type_traits>

namespace rt {

    template <typename Derived, typename T>
    class Tuple2 {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    public:
        T x, y;

        constexpr Tuple2() : x(0), y(0) {}

        constexpr Tuple2(T x, T y) : x(x), y(y) {
            assert(!HasNaN());
        }

        constexpr T operator[](int i) const {
            assert(i >= 0 && i < 2);
            return (i == 0) ? x : y;
        }

        constexpr T& operator[](int i) {
            assert(i >= 0 && i < 2);
            return (i == 0) ? x : y;
        }

        constexpr Derived operator-() const {
            return Derived(-x, -y);
        }

        constexpr Derived operator+(const Derived& other) const {
            return Derived(x + other.x, y + other.y);
        }

        constexpr Derived operator-(const Derived& other) const {
            return Derived(x - other.x, y - other.y);
        }

        constexpr Derived operator*(T scalar) const {
            return Derived(x * scalar, y * scalar);
        }

        constexpr Derived operator/(T scalar) const {
            assert(scalar != 0);
            if constexpr (std::is_floating_point_v<T>) {
                T inv = static_cast<T>(1) / scalar;
                return Derived(x * inv, y * inv);
            } else {
                return Derived(x / scalar, y / scalar);
            }
        }

        constexpr Derived& operator+=(const Derived& other) {
            x += other.x;
            y += other.y;
            return static_cast<Derived&>(*this);
        }

        constexpr Derived& operator-=(const Derived& other) {
            x -= other.x;
            y -= other.y;
            return static_cast<Derived&>(*this);
        }

        constexpr Derived& operator*=(T scalar) {
            x *= scalar;
            y *= scalar;
            return static_cast<Derived&>(*this);
        }

        constexpr Derived& operator/=(T scalar) {
            assert(scalar != 0);

            if constexpr (std::is_floating_point_v<T>) {
                T inv = static_cast<T>(1) / scalar;
                x *= inv;
                y *= inv;
            } else {
                x /= scalar;
                y /= scalar;
            }
            return static_cast<Derived&>(*this);
        }

        constexpr bool operator==(const Derived& other) const {
            return x == other.x && y == other.y;
        }

        constexpr bool operator!=(const Derived& other) const {
            return !(*this == other);
        }

        constexpr bool HasNaN() const {
            return std::isnan(x) || std::isnan(y);
        }
    };

    template <typename Derived, typename T>
    constexpr Derived operator*(T scalar, const Tuple2<Derived, T>& tuple) {
        return static_cast<const Derived&>(tuple) * scalar;
    }

    template <typename Derived, typename T>
    std::ostream& operator<<(std::ostream& os, const Tuple2<Derived, T>& tuple) {
        os << "[" << tuple.x << ", " << tuple.y << "]";
        return os;
    }
}