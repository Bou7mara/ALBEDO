#ifndef RT_CORE_VECTOR2_H
#define RT_CORE_VECTOR2_H

#include "rt/core/tuple2.h"
#include <cmath>
#include <algorithm>
#include <type_traits>

namespace rt {

template <typename T>
class Vector2 : public Tuple2<Vector2<T>, T> {
public:
    using Tuple2<Vector2<T>, T>::Tuple2;
};

// Typedefs for convenience
using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;
using Vector2i = Vector2<int>;

// Free Functions

template <typename T>
constexpr T LengthSquared(const Vector2<T>& v) {
    return v.x * v.x + v.y * v.y;
}

template <typename T>
T Length(const Vector2<T>& v) {
    static_assert(std::is_floating_point_v<T>, "Length() requires a floating-point type");
    return std::sqrt(LengthSquared(v));
}

template <typename T>
Vector2<T> Normalize(const Vector2<T>& v) {
    static_assert(std::is_floating_point_v<T>, "Normalize() requires a floating-point type");
    return v / Length(v);
}

template <typename T>
constexpr T Dot(const Vector2<T>& v1, const Vector2<T>& v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

template <typename T>
constexpr T AbsDot(const Vector2<T>& v1, const Vector2<T>& v2) {
    return std::abs(Dot(v1, v2));
}

template <typename T>
constexpr Vector2<T> Abs(const Vector2<T>& v) {
    return Vector2<T>(std::abs(v.x), std::abs(v.y));
}

} // namespace rt

#endif // RT_CORE_VECTOR2_H
