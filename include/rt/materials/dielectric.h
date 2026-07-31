#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    class Dielectric : public BSDF {
    public:
        // Default tint to white so existing uncolored glass code still works
        explicit Dielectric(float ior, const Vector3f& tint = Vector3f(1.0f, 1.0f, 1.0f))
            : ior_(ior), tint_(tint) {}

        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                           const Point2f& u, Vector3f* wi, float* pdf) const override;

        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

    private:
        float ior_;
        Vector3f tint_;
    };
}