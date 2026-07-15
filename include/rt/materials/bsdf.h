#pragma once
#include "rt/core/vector3.h"
#include "rt/core/point2.h"

namespace rt {
 
    class BSDF {
    public:
        virtual ~BSDF() = default;

        virtual Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const = 0;

        virtual Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                                   const Point2f& u, Vector3f* wi,
                                   float* pdf) const = 0;
    };
}
