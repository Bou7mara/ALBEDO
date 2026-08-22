#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    class Lambertian : public BSDF {
    public:
        explicit Lambertian(const Vector3f& albedo, float roughness = 0.0f)
            : albedo_(albedo), roughness_(roughness) {}

        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const override;

        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        const Vector3f& Albedo() const { return albedo_; }
        float Roughness() const { return roughness_; }

    private:
        Vector3f albedo_;
        float roughness_;
    };
}
