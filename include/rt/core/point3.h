#pragma once
#include "rt/core/tuple3.h"
#include "rt/core/vector3.h"
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
    class Point3 : public Tuple3<Point3<T>, T> {
    public:
        constexpr __host__ __device__ Point3() : Tuple3<Point3<T>, T>(0, 0, 0) {}
        constexpr __host__ __device__ Point3(T x_, T y_, T z_) : Tuple3<Point3<T>, T>(x_, y_, z_) {}

        explicit constexpr __host__ __device__ Point3(const Vector3<T>& v) : Tuple3<Point3<T>, T>(v.x, v.y, v.z) {}

        Point3 operator+(const Point3& other) const = delete;
        Point3& operator+=(const Point3& other) = delete;

        constexpr __host__ __device__ Point3<T> operator+(const Vector3<T>& v) const {
            return Point3<T>(this->x + v.x, this->y + v.y, this->z + v.z);
        }

        constexpr __host__ __device__ Point3<T>& operator+=(const Vector3<T>& v) {
            this->x += v.x;
            this->y += v.y;
            this->z += v.z;
            return *this;
        }

        constexpr __host__ __device__ Point3<T> operator-(const Vector3<T>& v) const {
            return Point3<T>(this->x - v.x, this->y - v.y, this->z - v.z);
        }

        constexpr __host__ __device__ Point3<T>& operator-=(const Vector3<T>& v) {
            this->x -= v.x;
            this->y -= v.y;
            this->z -= v.z;
            return *this;
        }

        constexpr __host__ __device__ Vector3<T> operator-(const Point3<T>& p) const {
            return Vector3<T>(this->x - p.x, this->y - p.y, this->z - p.z);
        }
    };

    using Point3f = Point3<float>;
    using Point3d = Point3<double>;
    using Point3i = Point3<int>;

    template <typename T>
    constexpr __host__ __device__ Point3<T> operator+(const Vector3<T>& v, const Point3<T>& p) {
        return p + v;
    }

    template <typename T>
    constexpr __host__ __device__ T DistanceSquared(const Point3<T>& p1, const Point3<T>& p2) {
        return LengthSquared(p1 - p2);
    }

    template <typename T>
    __host__ __device__ T Distance(const Point3<T>& p1, const Point3<T>& p2) {
        static_assert(std::is_floating_point_v<T>, "Distance() requires a floating-point type");
        return Length(p1 - p2);
    }

    template <typename T>
    constexpr __host__ __device__ Point3<T> Lerp(T t, const Point3<T>& p0, const Point3<T>& p1) {
        static_assert(std::is_floating_point_v<T>, "Lerp() requires a floating-point type");
        return p0 + t * (p1 - p0);
    }

    template <typename T>
    constexpr __host__ __device__ Point3<T> Floor(const Point3<T>& p) {
        return Point3<T>(std::floor(p.x), std::floor(p.y), std::floor(p.z));
    }

    template <typename T>
    constexpr __host__ __device__ Point3<T> Ceil(const Point3<T>& p) {
        return Point3<T>(std::ceil(p.x), std::ceil(p.y), std::ceil(p.z));
    }

    template <typename T>
    constexpr __host__ __device__ Point3<T> Min(const Point3<T>& p1, const Point3<T>& p2) {
        return Point3<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::min(p1.z, p2.z));
    }

    template <typename T>
    constexpr __host__ __device__ Point3<T> Max(const Point3<T>& p1, const Point3<T>& p2) {
        return Point3<T>(std::max(p1.x, p2.x), std::max(p1.y, p2.y), std::max(p1.z, p2.z));
    }

    template <typename T>
    constexpr __host__ __device__ Point3<T> Permute(const Point3<T>& p, int ix, int iy, int iz) {
        return Point3<T>(p[ix], p[iy], p[iz]);
    }

} // namespace rt
