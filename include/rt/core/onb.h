#pragma once
#include "rt/core/vector3.h"
#include <cmath>

namespace rt {

    struct ONB {
        Vector3f u, v, w;

        explicit ONB(const Vector3f& n) {
            w = n;

            float sign = std::copysign(1.0f, w.z);

            float a {-1.0f / (sign + w.z)}; float b {w.x * w.y * a};
            
            u = Vector3f(1.0f + sign * w.x * w.x * a, sign * b, -sign * w.x);
            v = Vector3f(b, sign + w.y * w.y * a, -w.y);
        }

        Vector3f ToWorld(const Vector3f& local) const {
            return local.x * u + local.y * v + local.z * w;
        }
    };
}
