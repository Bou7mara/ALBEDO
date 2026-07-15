#pragma once
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
        constexpr Ray()
            : o(), d(), tMax(std::numeric_limits<float>::infinity()), time(0.0f) {}

        constexpr Ray(const Point3f& origin, const Vector3f& direction,
                      float tMax_ = std::numeric_limits<float>::infinity(),
                      float time_ = 0.0f)
            : o(origin), d(direction), tMax(tMax_), time(time_) {}

        constexpr Point3f operator()(float t) const {
            return o + t * d;
        }
    };

    inline std::ostream& operator<<(std::ostream& os, const Ray& r) {
        os << "Ray[o=" << r.o << ", d=" << r.d << ", tMax=" << r.tMax
           << ", time=" << r.time << "]";
        return os;
    }
}
