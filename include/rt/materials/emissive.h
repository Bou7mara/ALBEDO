#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    // Emissive material that radiates light energy (used for area lights and glowing surfaces)
    class Emissive : public BSDF {
    public:
        // Constructs emissive material with specified RGB radiance color and intensity
        explicit Emissive(const Vector3f& radiance) : radiance_(radiance) {}

        // Returns black because light-emitting surfaces do not reflect incident light in this model
        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        // Returns black and sets PDF to 0 because emissive surfaces do not scatter rays
        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const override;

        // Returns 0.0 because there is no scattering direction PDF for light emitters
        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        // Returns the emitted light radiance Le if the outgoing ray points into the front hemisphere of the surface normal
        Vector3f Le(const Vector3f& wo, const Vector3f& n) const override;

    private:
        Vector3f radiance_; // RGB color and power emitted by the light surface
    };
}