#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    // Glass or dielectric material that handles both specular reflection and refraction (transmission)
    // Uses Snell's law and Fresnel equations to calculate reflection vs refraction probability
    class Dielectric : public BSDF {
    public:
        // Constructs glass material with given index of refraction (eta/ior), e.g. 1.5 for glass
        explicit Dielectric(float ior) : ior_(ior){}

        // Returns black because ideal specular glass has zero smooth diffuse reflection
        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        // Samples specular reflection or refraction direction based on Fresnel reflectance probability
        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                           const Point2f& u, Vector3f* wi, float* pdf) const override;

        // Returns 0.0 because discrete specular directions have zero continuous PDF density
        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

    private:
        float ior_; // Index of refraction ratio (inside relative to outside)
    };
}