#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    class Dielectric : public BSDF {
    public:
        // Default tint to white and dispersion to 0 so existing code still works
        explicit Dielectric(float ior, const Vector3f& tint = Vector3f(1.0f, 1.0f, 1.0f), float dispersion = 0.0f)
            : ior_(ior), tint_(tint), dispersion_(dispersion) {}

        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                          const Point2f& u, Vector3f* wi, float* pdf, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        float Ior() const { return ior_; }
        const Vector3f& Tint() const { return tint_; }
        float Dispersion() const { return dispersion_; }

    private:
        float ior_;
        Vector3f tint_;
        float dispersion_;
    };
}