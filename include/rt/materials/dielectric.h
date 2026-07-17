#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    class Dielectric : public BSDF {
    public:
        explicit Dielectric(float ior) : ior_(ior){}
        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;
        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                           const Point2f& u, Vector3f* wi, float* pdf) const override;

    private:
        float ior_;
    };
}