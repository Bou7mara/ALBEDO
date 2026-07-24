#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    // Lambertian matte surface material that scatters light uniformly in all directions over the hemisphere
    class Lambertian : public BSDF {
    public:
        // Constructs matte material with specified albedo color (diffuse reflectivity in range [0, 1])
        explicit Lambertian(const Vector3f& albedo) : albedo_(albedo) {}

        // Calculates constant BRDF value f = albedo / pi for diffuse reflection
        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        // Samples a cosine-weighted direction on the surface hemisphere and returns reflection color
        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const override;

        // Calculates cosine-weighted probability density PDF = cos(theta) / pi
        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

    private:
        Vector3f albedo_; // Base diffuse surface color
    };
}

