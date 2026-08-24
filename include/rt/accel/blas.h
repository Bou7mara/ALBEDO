#pragma once
#include "rt/core/bounds3.h"
#include "rt/core/ray.h"
#include "rt/shapes/shape.h"

namespace rt {

class BLAS {
public:
    virtual ~BLAS() = default;

    virtual bool Intersect(const Ray& ray, SurfaceInteraction* isect) const = 0;
    virtual bool IntersectP(const Ray& ray) const = 0;
    virtual Bounds3f WorldBound() const = 0;
};

}
