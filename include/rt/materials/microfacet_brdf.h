#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    // Rough microfacet BRDF using the GGX normal distribution model
    // Supports both rough dielectric (plastic/coated) and rough conductor (rough metal) reflections
    class Microfacet : public BSDF {
    public:
        // Factory: Constructs a rough dielectric microfacet material (e.g. rough plastic)
        static Microfacet MakeDielectricMicrofacet(float roughness, float ior);

        // Factory: Constructs a rough conductor microfacet material (e.g. rough metal like copper or gold)
        static Microfacet MakeConductorMicrofacet(float roughness,
                                                  const Vector3f& eta,
                                                  const Vector3f& k,
                                                  const Vector3f& tint = Vector3f(1.0f, 1.0f, 1.0f));

        // Calculates GGX microfacet BRDF value f = D * G * F / (4 * NdotV * NdotL)
        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        // Samples a reflection direction wi using Visible Normal Distribution Function (VNDF) sampling
        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                          const Point2f& u, Vector3f* wi, float* pdf) const override;

        // Calculates the PDF for sampling direction wi using GGX VNDF
        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

    private:
        enum class FresnelKind { Dielectric, Conductor };
        
        // Private constructor initialized via static factory methods
        Microfacet(float roughness, FresnelKind kind,
                   float ior, const Vector3f& eta, const Vector3f& k, const Vector3f& tint);

        // Evaluates Fresnel reflection for either dielectric or conductor material
        Vector3f EvaluateFresnel(float cosThetaI) const;

        float alpha_;        // GGX roughness parameter (alpha = roughness^2)
        FresnelKind kind_;   // Type of material interface (Dielectric or Conductor)
        float ior_;          // Index of refraction for dielectrics
        Vector3f eta_, k_;   // Complex index of refraction parameters for metals
        Vector3f tint_;      // Color tint applied to metal reflection
    };
}

