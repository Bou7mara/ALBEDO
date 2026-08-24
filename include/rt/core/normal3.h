#pragma once
#include "rt/core/tuple3.h"
#include "rt/core/vector3.h"
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
    class Normal3 : public Tuple3<Normal3<T>, T> {
    public:
        constexpr __host__ __device__ Normal3() : Tuple3<Normal3<T>, T>(0, 0, 0) {}
        constexpr __host__ __device__ Normal3(T x_, T y_, T z_) : Tuple3<Normal3<T>, T>(x_, y_, z_) {}

        explicit constexpr __host__ __device__ Normal3(const Vector3<T>& v) : Tuple3<Normal3<T>, T>(v.x, v.y, v.z) {}

        explicit constexpr __host__ __device__ operator Vector3<T>() const {
            return Vector3<T>(this->x, this->y, this->z);
        }
    };

    using Normal3f = Normal3<float>;
    using Normal3d = Normal3<double>;

    template <typename T>
    constexpr __host__ __device__ T LengthSquared(const Normal3<T>& n) {
        return n.x * n.x + n.y * n.y + n.z * n.z;
    }

    template <typename T>
    __host__ __device__ T Length(const Normal3<T>& n) {
        static_assert(std::is_floating_point_v<T>, "Length() requires a floating-point type");
        return std::sqrt(LengthSquared(n));
    }

    template <typename T>
    __host__ __device__ Normal3<T> Normalize(const Normal3<T>& n) {
        static_assert(std::is_floating_point_v<T>, "Normalize() requires a floating-point type");
        return n / Length(n);
    }

    template <typename T>
    constexpr __host__ __device__ T Dot(const Normal3<T>& n, const Vector3<T>& v) {
        return n.x * v.x + n.y * v.y + n.z * v.z;
    }

    template <typename T>
    constexpr __host__ __device__ T Dot(const Vector3<T>& v, const Normal3<T>& n) {
        return v.x * n.x + v.y * n.y + v.z * n.z;
    }

    template <typename T>
    constexpr __host__ __device__ T Dot(const Normal3<T>& n1, const Normal3<T>& n2) {
        return n1.x * n2.x + n1.y * n2.y + n1.z * n2.z;
    }

    template <typename T>
    constexpr __host__ __device__ T AbsDot(const Normal3<T>& n, const Vector3<T>& v) {
        return std::abs(Dot(n, v));
    }

    template <typename T>
    constexpr __host__ __device__ T AbsDot(const Vector3<T>& v, const Normal3<T>& n) {
        return std::abs(Dot(v, n));
    }

    template <typename T>
    constexpr __host__ __device__ T AbsDot(const Normal3<T>& n1, const Normal3<T>& n2) {
        return std::abs(Dot(n1, n2));
    }

    template <typename T>
    constexpr __host__ __device__ Normal3<T> FaceForward(const Normal3<T>& n, const Vector3<T>& v) {
        return (Dot(n, v) < static_cast<T>(0)) ? -n : n;
    }

    template <typename T>
    constexpr __host__ __device__ Normal3<T> FaceForward(const Normal3<T>& n, const Normal3<T>& n2) {
        return (Dot(n, n2) < static_cast<T>(0)) ? -n : n;
    }

}
