#include "rt/materials/metal.h"

namespace rt {

Vector3f Metal::f(const Vector3f& /*wo*/, const Vector3f& /*wi*/,
                  const Vector3f& /*n*/) const {
    // Delta distribution: the probability of an arbitrary, independently
    // supplied (wo, wi) pair matching the single valid mirror direction
    // is zero. Only Sample_f produces a non-zero contribution -- see
    // the design note on why this is correct, not a placeholder.
    return Vector3f(0.0f, 0.0f, 0.0f);
}

Vector3f Metal::Sample_f(const Vector3f& wo, const Vector3f& n,
                         const Point2f& /*u*/, Vector3f* wi, float* pdf) const {
    // u is unused: reflection is fully determined by (wo, n), no
    // randomness involved. Still accepted, to keep the same call
    // signature every BSDF shares -- main.cpp's integrator doesn't
    // need to know which BSDF it's calling.
    *wi = Normalize(2.0f * Dot(wo, n) * n - wo);
    *pdf = 1.0f;

    float cosTheta = AbsDot(*wi, n);
    if (cosTheta < 1e-6f) {
        // Grazing/degenerate reflection: avoid dividing by ~0 below.
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    // Fabricated so f * cosTheta / pdf collapses exactly to albedo_ in
    // the integrator -- the same cancellation trick Lambertian uses,
    // adapted for pdf=1 instead of a cosine-weighted density. This is
    // pbrt's actual SpecularReflection convention, not a local invention.
    return albedo_ / cosTheta;
}

} // namespace rt
