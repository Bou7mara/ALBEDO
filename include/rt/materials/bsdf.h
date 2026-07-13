#ifndef RT_MATERIALS_BSDF_H
#define RT_MATERIALS_BSDF_H

#include "rt/core/vector3.h"
#include "rt/core/point2.h"

namespace rt {

// Both wo and wi point AWAY from the surface (wo = -ray.d, already
// flipped by Shape::Intersect; wi = the scattering direction). World
// space throughout -- see the design note on why there's no local
// shading frame yet.
class BSDF {
public:
    virtual ~BSDF() = default;

    // BSDF value for an explicit (wo, wi) pair. Used when a future
    // light-sampling / NEE path needs to evaluate a *specific*
    // direction rather than importance-sample one (TOC 8.1).
    virtual Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const = 0;

    // Importance-samples wi given wo and the geometric normal n.
    // u supplies two uniform randoms in [0,1) driving the sample.
    // Fills wi and pdf (solid-angle measure); returns f(wo, wi).
    virtual Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                               const Point2f& u, Vector3f* wi,
                               float* pdf) const = 0;
};

} // namespace rt

#endif // RT_MATERIALS_BSDF_H
