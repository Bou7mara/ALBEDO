#ifndef RT_CORE_ONB_H
#define RT_CORE_ONB_H

#include "rt/core/vector3.h"
#include <cmath>

namespace rt {

// Orthonormal basis (u, v, w) built around a unit vector n, with
// w == n. ToWorld() maps a direction expressed in this local frame
// (z-axis == n) into world space.
struct ONB {
    Vector3f u, v, w;

    explicit ONB(const Vector3f& n) {
        w = n;
        float sign = std::copysign(1.0f, w.z);
        float a = -1.0f / (sign + w.z);
        float b = w.x * w.y * a;
        u = Vector3f(1.0f + sign * w.x * w.x * a, sign * b, -sign * w.x);
        v = Vector3f(b, sign + w.y * w.y * a, -w.y);
    }

    Vector3f ToWorld(const Vector3f& local) const {
        return local.x * u + local.y * v + local.z * w;
    }
};

} // namespace rt

#endif // RT_CORE_ONB_H
