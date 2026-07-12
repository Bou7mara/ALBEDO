#ifndef RT_CORE_RAY_H
#define RT_CORE_RAY_H

#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include <limits>

namespace rt {

class Ray {
public:
    Point3f o;
    Vector3f d;
    mutable float tMax;
    float time;

    // Note: no Medium* yet. Add when participating media (TOC 6.11 / 7.4)
    // is implemented; do not guess at the interface prematurely.

    constexpr Ray()
        : o(), d(), tMax(std::numeric_limits<float>::infinity()), time(0.0f) {}

    constexpr Ray(const Point3f& origin, const Vector3f& direction,
                  float tMax_ = std::numeric_limits<float>::infinity(),
                  float time_ = 0.0f)
        : o(origin), d(direction), tMax(tMax_), time(time_) {}

    // Evaluates the point along the ray at parameter t: o + t*d
    constexpr Point3f operator()(float t) const {
        return o + t * d;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Ray& r) {
    os << "Ray[o=" << r.o << ", d=" << r.d << ", tMax=" << r.tMax
       << ", time=" << r.time << "]";
    return os;
}

} // namespace rt

#endif // RT_CORE_RAY_H
