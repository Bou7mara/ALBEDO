#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    // Perfect specular metal material (mirror-like metallic reflection)
    class Metal : public BSDF {
    public:
        // Constructs metal material with specified albedo color tint (e.g. gold, copper, silver tint)
        explicit Metal(const Vector3f& albedo) : albedo_(albedo) {}

        // Returns black because smooth specular metal has zero diffuse reflection
        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        // Calculates exact specular reflection direction wi = Reflect(wo, n) and returns tinted reflection color
        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const override;

        // Returns 0.0 because discrete mirror reflections have zero continuous probability density
        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

    private:
        Vector3f albedo_; // Metal reflection color tint
    };
}

