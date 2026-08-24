#pragma once
#include "rt/core/tuple3.h"
#include <algorithm>
#include <cmath>
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

    template <typename T>
    class Vector3 : public Tuple3<Vector3<T>, T> {
    public:
        constexpr __host__ __device__ Vector3() : Tuple3<Vector3<T>, T>(0, 0, 0) {}
        constexpr __host__ __device__ Vector3(T x_, T y_, T z_) : Tuple3<Vector3<T>, T>(x_, y_, z_) {}
    };

    using Vector3f = Vector3<float>;
    using Vector3d = Vector3<double>;
    using Vector3i = Vector3<int>;

    template <typename T>
    constexpr __host__ __device__ T LengthSquared(const Vector3<T>& v) {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    template <typename T>
    __host__ __device__ T Length(const Vector3<T>& v) {
        static_assert(std::is_floating_point_v<T>, "Length() requires a floating-point type");
        return std::sqrt(LengthSquared(v));
    }

    template <typename T>
    __host__ __device__ Vector3<T> Normalize(const Vector3<T>& v) {
        static_assert(std::is_floating_point_v<T>, "Normalize() requires a floating-point type");
        return v / Length(v);
    }

    template <typename T>
    constexpr __host__ __device__ T Dot(const Vector3<T>& v1, const Vector3<T>& v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    template <typename T>
    constexpr __host__ __device__ T AbsDot(const Vector3<T>& v1, const Vector3<T>& v2) {
        return std::abs(Dot(v1, v2));
    }

    template <typename T>
    constexpr __host__ __device__ Vector3<T> Cross(const Vector3<T>& v1, const Vector3<T>& v2) {
        return Vector3<T>(
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        );
    }

    template <typename T>
    constexpr __host__ __device__ Vector3<T> Abs(const Vector3<T>& v) {
        return Vector3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
    }

    template <typename T>
    constexpr __host__ __device__ T MinComponent(const Vector3<T>& v) {
        return std::min(v.x, std::min(v.y, v.z));
    }

    template <typename T>
    constexpr __host__ __device__ T MaxComponent(const Vector3<T>& v) {
        return std::max(v.x, std::max(v.y, v.z));
    }

    template <typename T>
    constexpr __host__ __device__ int MaxDimension(const Vector3<T>& v) {
        return (v.x > v.y) ? ((v.x > v.z) ? 0 : 2) : ((v.y > v.z) ? 1 : 2);
    }

    template <typename T>
    constexpr __host__ __device__ Vector3<T> Permute(const Vector3<T>& v, int ix, int iy, int iz) {
        return Vector3<T>(v[ix], v[iy], v[iz]);
    }

    template <typename T>
    constexpr __host__ __device__ Vector3<T> operator*(const Vector3<T>& a, const Vector3<T>& b) {
        return Vector3<T>(a.x * b.x, a.y * b.y, a.z * b.z);
    }

}
