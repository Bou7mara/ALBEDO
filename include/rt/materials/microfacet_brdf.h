#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    class Microfacet : public BSDF {
    public:
        static Microfacet MakeDielectricMicrofacet(float roughness, float ior);

        static Microfacet MakeConductorMicrofacet(float roughness,
                                                  const Vector3f& eta,
                                                  const Vector3f& k,
                                                  const Vector3f& tint = Vector3f(1.0f, 1.0f, 1.0f));

        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                          const Point2f& u, Vector3f* wi, float* pdf) const override;

        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;

    private:
        enum class FresnelKind { Dielectric, Conductor };
        
        Microfacet(float roughness, FresnelKind kind,
                   float ior, const Vector3f& eta, const Vector3f& k, const Vector3f& tint);

        Vector3f EvaluateFresnel(float cosThetaI) const;

        float alpha_;
        FresnelKind kind_;
        float ior_;
        Vector3f eta_, k_;
        Vector3f tint_;
    };
}
