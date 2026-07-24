#pragma once
#include "rt/core/tuple3.h"
#include "rt/core/vector3.h"
#include <cmath>
#include <type_traits>

namespace rt {

    template <typename T>
    class Normal3 : public Tuple3<Normal3<T>, T> {
    public:
        using Tuple3<Normal3<T>, T>::Tuple3;

        explicit constexpr Normal3(const Vector3<T>& v) : Tuple3<Normal3<T>, T>(v.x, v.y, v.z) {}

        explicit constexpr operator Vector3<T>() const {
            return Vector3<T>(this->x, this->y, this->z);
        }
    };

    using Normal3f = Normal3<float>;
    using Normal3d = Normal3<double>;

    template <typename T>
    constexpr T LengthSquared(const Normal3<T>& n) {
        return n.x * n.x + n.y * n.y + n.z * n.z;
    }

    template <typename T>
    T Length(const Normal3<T>& n) {
        static_assert(std::is_floating_point_v<T>, "Length() requires a floating-point type");
        return std::sqrt(LengthSquared(n));
    }

    template <typename T>
    Normal3<T> Normalize(const Normal3<T>& n) {
        static_assert(std::is_floating_point_v<T>, "Normalize() requires a floating-point type");
        return n / Length(n);
    }

    template <typename T>
    constexpr T Dot(const Normal3<T>& n, const Vector3<T>& v) {
        return n.x * v.x + n.y * v.y + n.z * v.z;
    }

    template <typename T>
    constexpr T Dot(const Vector3<T>& v, const Normal3<T>& n) {
        return v.x * n.x + v.y * n.y + v.z * n.z;
    }

    template <typename T>
    constexpr T Dot(const Normal3<T>& n1, const Normal3<T>& n2) {
        return n1.x * n2.x + n1.y * n2.y + n1.z * n2.z;
    }

    template <typename T>
    constexpr T AbsDot(const Normal3<T>& n, const Vector3<T>& v) {
        return std::abs(Dot(n, v));
    }

    template <typename T>
    constexpr T AbsDot(const Vector3<T>& v, const Normal3<T>& n) {
        return std::abs(Dot(v, n));
    }

    template <typename T>
    constexpr T AbsDot(const Normal3<T>& n1, const Normal3<T>& n2) {
        return std::abs(Dot(n1, n2));
    }

    template <typename T>
    constexpr Normal3<T> FaceForward(const Normal3<T>& n, const Vector3<T>& v) {
        return (Dot(n, v) < static_cast<T>(0)) ? -n : n;
    }

    template <typename T>
    constexpr Normal3<T> FaceForward(const Normal3<T>& n, const Normal3<T>& n2) {
        return (Dot(n, n2) < static_cast<T>(0)) ? -n : n;
    }
}
