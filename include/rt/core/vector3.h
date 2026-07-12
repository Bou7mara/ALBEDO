#ifndef RT_CORE_VECTOR3_H
#define RT_CORE_VECTOR3_H

#include "rt/core/tuple3.h"
#include <algorithm>

namespace rt {

template <typename T>
class Vector3 : public Tuple3<Vector3<T>, T> {
public:
    using Tuple3<Vector3<T>, T>::Tuple3;

    constexpr T LengthSquared() const {
        return this->x * this->x + this->y * this->y + this->z * this->z;
    }

    T Length() const {
        return std::sqrt(LengthSquared());
    }

    Vector3<T> Normalize() const {
        return *this / Length();
    }
};

// Typedefs for convenience
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;
using Vector3i = Vector3<int>;

// Free Functions

template <typename T>
constexpr T dot(const Vector3<T>& v1, const Vector3<T>& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template <typename T>
constexpr T absDot(const Vector3<T>& v1, const Vector3<T>& v2) {
    return std::abs(dot(v1, v2));
}

template <typename T>
constexpr Vector3<T> cross(const Vector3<T>& v1, const Vector3<T>& v2) {
    return Vector3<T>(
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x
    );
}

template <typename T>
constexpr Vector3<T> abs(const Vector3<T>& v) {
    return Vector3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

template <typename T>
constexpr T minComponent(const Vector3<T>& v) {
    return std::min({v.x, v.y, v.z});
}

template <typename T>
constexpr T maxComponent(const Vector3<T>& v) {
    return std::max({v.x, v.y, v.z});
}

template <typename T>
constexpr int maxDimension(const Vector3<T>& v) {
    return (v.x > v.y) ? ((v.x > v.z) ? 0 : 2) : ((v.y > v.z) ? 1 : 2);
}

template <typename T>
constexpr Vector3<T> permute(const Vector3<T>& v, int ix, int iy, int iz) {
    return Vector3<T>(v[ix], v[iy], v[iz]);
}

template <typename T>
Vector3<T> normalize(const Vector3<T>& v) {
    return v.Normalize();
}

} // namespace rt

#endif // RT_CORE_VECTOR3_H
