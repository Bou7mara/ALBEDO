#ifndef RT_CORE_NORMAL3_H
#define RT_CORE_NORMAL3_H

#include "rt/core/tuple3.h"
#include "rt/core/vector3.h"
#include <cmath>

namespace rt {

template <typename T>
class Normal3 : public Tuple3<Normal3<T>, T> {
public:
    using Tuple3<Normal3<T>, T>::Tuple3;

    // Explicit construct from Vector3
    explicit constexpr Normal3(const Vector3<T>& v) : Tuple3<Normal3<T>, T>(v.x, v.y, v.z) {}

    constexpr T LengthSquared() const {
        return this->x * this->x + this->y * this->y + this->z * this->z;
    }

    T Length() const {
        return std::sqrt(LengthSquared());
    }

    Normal3<T> Normalize() const {
        return *this / Length();
    }
};

// Typedefs for convenience
using Normal3f = Normal3<float>;
using Normal3d = Normal3<double>;

// Free Functions

template <typename T>
constexpr T dot(const Normal3<T>& n, const Vector3<T>& v) {
    return n.x * v.x + n.y * v.y + n.z * v.z;
}

template <typename T>
constexpr T dot(const Vector3<T>& v, const Normal3<T>& n) {
    return v.x * n.x + v.y * n.y + v.z * n.z;
}

template <typename T>
constexpr T dot(const Normal3<T>& n1, const Normal3<T>& n2) {
    return n1.x * n2.x + n1.y * n2.y + n1.z * n2.z;
}

template <typename T>
constexpr T absDot(const Normal3<T>& n, const Vector3<T>& v) {
    return std::abs(dot(n, v));
}

template <typename T>
constexpr T absDot(const Vector3<T>& v, const Normal3<T>& n) {
    return std::abs(dot(v, n));
}

template <typename T>
constexpr T absDot(const Normal3<T>& n1, const Normal3<T>& n2) {
    return std::abs(dot(n1, n2));
}

template <typename T>
Normal3<T> normalize(const Normal3<T>& n) {
    return n.Normalize();
}

// FaceForward: flips normal if it points away from the reference direction
template <typename T>
constexpr Normal3<T> faceForward(const Normal3<T>& n, const Vector3<T>& v) {
    return (dot(n, v) < 0.f) ? -n : n;
}

template <typename T>
constexpr Normal3<T> faceForward(const Normal3<T>& n, const Normal3<T>& n2) {
    return (dot(n, n2) < 0.f) ? -n : n;
}

} // namespace rt

#endif // RT_CORE_NORMAL3_H
