#include "rt/materials/emissive.h"

namespace rt {

    Vector3f Emissive::f(const Vector3f&, const Vector3f&, const Vector3f&, const Point2f&) const {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    Vector3f Emissive::Sample_f(const Vector3f&, const Vector3f&, const Point2f&, Vector3f* wi, float* pdf, const Point2f&) const {
        *wi = Vector3f(0.0f, 0.0f, 0.0f);
        *pdf = 0.0f;
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    Vector3f Emissive::Le(const Vector3f& wo, const Vector3f& n) const {
        return Dot(wo, n) > 0.0f ? radiance_ : Vector3f(0.0f, 0.0f, 0.0f);
    }

    float Emissive::Pdf(const Vector3f&, const Vector3f&, const Vector3f&, const Point2f&) const {
        return 0.0f;
    }
}
