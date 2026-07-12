#ifndef RT_CORE_POINT3_H
#define RT_CORE_POINT3_H

#include "rt/core/tuple3.h"
#include "rt/core/vector3.h"
#include <algorithm>
#include <cmath>

namespace rt {

template <typename T>
class Point3 : public Tuple3<Point3<T>, T> {
public:
    using Tuple3<Point3<T>, T>::Tuple3;

    // Explicitly construct from Vector3
    explicit constexpr Point3(const Vector3<T>& v) : Tuple3<Point3<T>, T>(v.x, v.y, v.z) {}

    // Disable adding two points
    Point3 operator+(const Point3& other) const = delete;
    Point3& operator+=(const Point3& other) = delete;

    // Point + Vector -> Point
    constexpr Point3<T> operator+(const Vector3<T>& v) const {
        return Point3<T>(this->x + v.x, this->y + v.y, this->z + v.z);
    }

    constexpr Point3<T>& operator+=(const Vector3<T>& v) {
        this->x += v.x;
        this->y += v.y;
        this->z += v.z;
        return *this;
    }

    // Point - Vector -> Point
    constexpr Point3<T> operator-(const Vector3<T>& v) const {
        return Point3<T>(this->x - v.x, this->y - v.y, this->z - v.z);
    }

    constexpr Point3<T>& operator-=(const Vector3<T>& v) {
        this->x -= v.x;
        this->y -= v.y;
        this->z -= v.z;
        return *this;
    }

    // Point - Point -> Vector
    constexpr Vector3<T> operator-(const Point3<T>& p) const {
        return Vector3<T>(this->x - p.x, this->y - p.y, this->z - p.z);
    }
};

// Typedefs for convenience
using Point3f = Point3<float>;
using Point3d = Point3<double>;
using Point3i = Point3<int>;

// Commutative operator: Vector3 + Point3 -> Point3
template <typename T>
constexpr Point3<T> operator+(const Vector3<T>& v, const Point3<T>& p) {
    return p + v;
}

// Free Functions

template <typename T>
constexpr T distanceSquared(const Point3<T>& p1, const Point3<T>& p2) {
    return (p1 - p2).LengthSquared();
}

template <typename T>
T distance(const Point3<T>& p1, const Point3<T>& p2) {
    return (p1 - p2).Length();
}

template <typename T>
constexpr Point3<T> lerp(T t, const Point3<T>& p0, const Point3<T>& p1) {
    return (1 - t) * p0 + t * p1;
}

template <typename T>
constexpr Point3<T> floor(const Point3<T>& p) {
    return Point3<T>(std::floor(p.x), std::floor(p.y), std::floor(p.z));
}

template <typename T>
constexpr Point3<T> ceil(const Point3<T>& p) {
    return Point3<T>(std::ceil(p.x), std::ceil(p.y), std::ceil(p.z));
}

template <typename T>
constexpr Point3<T> min(const Point3<T>& p1, const Point3<T>& p2) {
    return Point3<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::min(p1.z, p2.z));
}

template <typename T>
constexpr Point3<T> max(const Point3<T>& p1, const Point3<T>& p2) {
    return Point3<T>(std::max(p1.x, p2.x), std::max(p1.y, p2.y), std::max(p1.z, p2.z));
}

template <typename T>
constexpr Point3<T> permute(const Point3<T>& p, int ix, int iy, int iz) {
    return Point3<T>(p[ix], p[iy], p[iz]);
}

} // namespace rt

#endif // RT_CORE_POINT3_H
