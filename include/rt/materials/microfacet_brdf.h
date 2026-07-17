#pragma once
#include "rt/materials/bsdf.h"

namespace rt {
    // Rough specular BRDF (Cook-Torrance, GGX distribution, TOC 6.6).
    // Construct via MakeDielectricMicrofacet or MakeConductorMicrofacet,
    // not the raw constructor directly -- the two factories are what keep
    // "which Fresnel term does this material use" a construction-time
    // decision instead of a per-call branch inside f()/Sample_f(). This
    // mirrors the Reflect/Refract split in fresnel.h: two callers sharing
    // one geometric core (D, G, half-vector sampling), diverging only in
    // Fresnel and in the tint applied to the result.
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

    private:
        enum class FresnelKind { Dielectric, Conductor };
        
        // Private constructor -- only the two factories above may build one,
        // so every live Microfacet instance is unambiguously "the
        // dielectric kind" or "the conductor kind," never a half-configured
        // in-between state.
        Microfacet(float roughness, FresnelKind kind,
                   float ior, const Vector3f& eta, const Vector3f& k, const Vector3f& tint);

        Vector3f EvaluateFresnel(float cosThetaI) const;

        float alpha_;          // AlphaFromRoughness(roughness), precomputed once
        FresnelKind kind_;
        float ior_;            // used only when kind_ == Dielectric
        Vector3f eta_, k_;     // used only when kind_ == Conductor (per-channel)
        Vector3f tint_;
    };
}
