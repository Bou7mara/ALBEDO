#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    class Emissive : public BSDF {
    public:
        explicit Emissive(const Vector3f& radiance) : radiance_(radiance) {}
        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;
        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const override;
        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;
        Vector3f Le(const Vector3f& wo, const Vector3f& n) const override;

    private:
        Vector3f radiance_;
    };
}