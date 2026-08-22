#pragma once
#ifndef __CUDA_ARCH__
#include <iostream>
#endif
#include <cmath>
#include <cassert>
#include <type_traits>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    template <typename Derived, typename T>
    class Tuple2 {
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    public:
        T x, y;

        constexpr __host__ __device__ Tuple2() : x(0), y(0) {}

        constexpr __host__ __device__ Tuple2(T x, T y) : x(x), y(y) {}

        constexpr __host__ __device__ T operator[](int i) const {
#ifndef NDEBUG
            assert(i >= 0 && i < 2);
#endif
            return (i == 0) ? x : y;
        }

        constexpr __host__ __device__ T& operator[](int i) {
#ifndef NDEBUG
            assert(i >= 0 && i < 2);
#endif
            return (i == 0) ? x : y;
        }

        constexpr __host__ __device__ Derived operator-() const {
            return Derived(-x, -y);
        }

        constexpr __host__ __device__ Derived operator+(const Derived& other) const {
            return Derived(x + other.x, y + other.y);
        }

        constexpr __host__ __device__ Derived operator-(const Derived& other) const {
            return Derived(x - other.x, y - other.y);
        }

        constexpr __host__ __device__ Derived operator*(T scalar) const {
            return Derived(x * scalar, y * scalar);
        }

        constexpr __host__ __device__ Derived operator/(T scalar) const {
            if constexpr (std::is_floating_point_v<T>) {
                T inv = static_cast<T>(1) / scalar;
                return Derived(x * inv, y * inv);
            } else {
                return Derived(x / scalar, y / scalar);
            }
        }

        constexpr __host__ __device__ Derived& operator+=(const Derived& other) {
            x += other.x;
            y += other.y;
            return static_cast<Derived&>(*this);
        }

        constexpr __host__ __device__ Derived& operator-=(const Derived& other) {
            x -= other.x;
            y -= other.y;
            return static_cast<Derived&>(*this);
        }

        constexpr __host__ __device__ Derived& operator*=(T scalar) {
            x *= scalar;
            y *= scalar;
            return static_cast<Derived&>(*this);
        }

        constexpr __host__ __device__ Derived& operator/=(T scalar) {
#ifndef NDEBUG
            assert(scalar != 0);
#endif
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

        constexpr __host__ __device__ bool operator==(const Derived& other) const {
            return x == other.x && y == other.y;
        }

        constexpr __host__ __device__ bool operator!=(const Derived& other) const {
            return !(*this == other);
        }

        constexpr __host__ __device__ bool HasNaN() const {
            return std::isnan(x) || std::isnan(y);
        }
    };

    template <typename Derived, typename T>
    constexpr __host__ __device__ Derived operator*(T scalar, const Tuple2<Derived, T>& tuple) {
        return static_cast<const Derived&>(tuple) * scalar;
    }

#ifndef __CUDA_ARCH__
    template <typename Derived, typename T>
    std::ostream& operator<<(std::ostream& os, const Tuple2<Derived, T>& tuple) {
        os << "[" << tuple.x << ", " << tuple.y << "]";
        return os;
    }
#endif

} // namespace rt