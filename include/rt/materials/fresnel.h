#pragma once
#include "rt/materials/bsdf.h"
#include "rt/core/vector3.h"
#include "rt/core/point2.h"

namespace rt {
    class Fresnel : public BSDF {

        inline Vector3f Reflect(const Vector3f& wo, const Vector3f& n) {
            return Normalize(2.0f * Dot(wo, n) * n - wo);
        }
    };
}