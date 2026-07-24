#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/microfacet.h"
#include "rt/materials/fresnel.h"
#include "rt/core/onb.h"

namespace rt {

    // Factory: Creates rough dielectric plastic/coated material
    Microfacet Microfacet::MakeDielectricMicrofacet(float roughness, float ior) {
        return Microfacet(roughness, FresnelKind::Dielectric, ior, Vector3f(1.0f, 1.0f, 1.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f, 1.0f, 1.0f));
    }

    // Factory: Creates rough metallic conductor material
    Microfacet Microfacet::MakeConductorMicrofacet(float roughness, const Vector3f& eta, const Vector3f& k, const Vector3f& tint) {
        return Microfacet(roughness, FresnelKind::Conductor, 1.0f, eta, k, tint);
    }

    // Private constructor initializing parameters
    Microfacet::Microfacet(float roughness, FresnelKind kind, float ior, const Vector3f& eta, const Vector3f& k, const Vector3f& tint)
        : alpha_(AlphaFromRoughness(roughness)), kind_(kind), ior_(ior), eta_(eta), k_(k), tint_(tint) {
    }

    // Evaluates Fresnel reflectance for dielectric or metallic conductor interface
    Vector3f Microfacet::EvaluateFresnel(float cosThetaI) const {
        if (kind_ == FresnelKind::Dielectric) {
            float f = FrDielectric(cosThetaI, 1.0f, ior_);
            return Vector3f(f, f, f);
        } else {
            float fr = FrConductor(cosThetaI, eta_.x, k_.x);
            float fg = FrConductor(cosThetaI, eta_.y, k_.y);
            float fb = FrConductor(cosThetaI, eta_.z, k_.z);
            return tint_ * Vector3f(fr, fg, fb);
        }
    }

    // Calculates GGX microfacet BRDF value f = D * G * F / (4 * NdotV * NdotL)
    Vector3f Microfacet::f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const {
        float NdotV = AbsDot(wo, n);
        float NdotL = AbsDot(wi, n);

        if (NdotV < 1e-4f || NdotL < 1e-4f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        // Calculate half-vector H halfway between wo and wi
        Vector3f wh = wo + wi;
        if (LengthSquared(wh) == 0.0f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }
        wh = Normalize(wh);

        // Evaluate GGX Normal Distribution D(H)
        float NdotH = AbsDot(wh, n);
        float D = GgxD(NdotH, alpha_);

        // Evaluate Smith masking-shadowing function G / (4 * NdotV * NdotL)
        float Gterm = SmithG(NdotV, NdotL, alpha_);

        // Evaluate Fresnel factor F(wo . H)
        float VdotH = AbsDot(wo, wh);
        Vector3f F = EvaluateFresnel(VdotH);

        return D * Gterm * F;
    }

    // Samples reflection direction wi using GGX Visible Normal Distribution Function (VNDF)
    Vector3f Microfacet::Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const {
        // Construct local shading basis
        ONB onb(n);

        // Convert outgoing direction to local coordinates
        Vector3f localWo = Vector3f(Dot(wo, onb.u), Dot(wo, onb.v), Dot(wo, onb.w));

        // Sample half-vector in local space from GGX VNDF
        Vector3f localWh = SampleGgxVndf(localWo, alpha_, u.x, u.y);

        // Transform half-vector to world space
        Vector3f wh = onb.ToWorld(localWh);

        // Reflect wo across half-vector wh to get incoming light direction wi
        *wi = Reflect(wo, wh);

        // Discard directions that reflect into the surface
        if (Dot(*wi, n) <= 0.0f) {
            *pdf = 0.0f;
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        float NdotV = AbsDot(wo, n);
        float NdotH = AbsDot(wh, n);

        // Calculate PDF for VNDF sampling
        *pdf = GgxVndfPdf(NdotV, NdotH, alpha_);

        return f(wo, *wi, n);
    }

    // Calculates sampling PDF for given directions wo, wi and surface normal n
    float Microfacet::Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const {
        if (Dot(wo, n) * Dot(wi, n) <= 0.0f) {
            return 0.0f;
        }
        Vector3f wh = wo + wi;
        if (LengthSquared(wh) == 0.0f) return 0.0f;
        wh = Normalize(wh);
        float NdotV = AbsDot(wo, n);
        float NdotH = AbsDot(wh, n);
        return GgxVndfPdf(NdotV, NdotH, alpha_);
    }
}

