#include "rt/materials/emissive.h"

namespace rt {

    // Emissive materials do not reflect incoming light
    Vector3f Emissive::f(const Vector3f&, const Vector3f&, const Vector3f&) const {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    // Emissive materials do not generate reflection sample directions
    Vector3f Emissive::Sample_f(const Vector3f&, const Vector3f&, const Point2f&, Vector3f* wi, float* pdf) const {
        *wi = Vector3f(0.0f, 0.0f, 0.0f);
        *pdf = 0.0f;
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    // Returns emitted light radiance when the ray leaves the front side of the surface
    Vector3f Emissive::Le(const Vector3f& wo, const Vector3f& n) const {
        return Dot(wo, n) > 0.0f ? radiance_ : Vector3f(0.0f, 0.0f, 0.0f);
    }

    // Scattering PDF is 0 for non-reflecting light emitters
    float Emissive::Pdf(const Vector3f&, const Vector3f&, const Vector3f&) const {
        return 0.0f;
    }
}