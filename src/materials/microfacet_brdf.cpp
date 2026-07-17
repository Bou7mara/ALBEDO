#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/microfacet.h"
#include "rt/materials/fresnel.h"
#include "rt/core/onb.h"

namespace rt {

    Microfacet Microfacet::MakeDielectricMicrofacet(float roughness, float ior) {
        return Microfacet(roughness, FresnelKind::Dielectric, ior, Vector3f(1.0f, 1.0f, 1.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f, 1.0f, 1.0f));
    }

    Microfacet Microfacet::MakeConductorMicrofacet(float roughness, const Vector3f& eta, const Vector3f& k, const Vector3f& tint) {
        return Microfacet(roughness, FresnelKind::Conductor, 1.0f, eta, k, tint);
    }

    Microfacet::Microfacet(float roughness, FresnelKind kind, float ior, const Vector3f& eta, const Vector3f& k, const Vector3f& tint)
        : alpha_(AlphaFromRoughness(roughness)), kind_(kind), ior_(ior), eta_(eta), k_(k), tint_(tint) {
    }

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

    Vector3f Microfacet::f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const {
        float NdotV = AbsDot(wo, n);
        float NdotL = AbsDot(wi, n);

        if (NdotV < 1e-4f || NdotL < 1e-4f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        Vector3f wh = wo + wi;
        if (LengthSquared(wh) == 0.0f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }
        wh = Normalize(wh);

        float NdotH = AbsDot(wh, n);
        float D = GgxD(NdotH, alpha_);

        // Note: SmithG already returns G / (4 * NdotV * NdotL).
        // Therefore, we do NOT divide by 4 * NdotV * NdotL again below.
        float Gterm = SmithG(NdotV, NdotL, alpha_);

        float VdotH = AbsDot(wo, wh);
        Vector3f F = EvaluateFresnel(VdotH);

        return D * Gterm * F;
    }

    Vector3f Microfacet::Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const {
        ONB onb(n);

        // Transform wo to local shading space (where n is (0,0,1))
        Vector3f localWo = Vector3f(Dot(wo, onb.u), Dot(wo, onb.v), Dot(wo, onb.w));

        // Sample half-vector in local space
        Vector3f localWh = SampleGgxVndf(localWo, alpha_, u.x, u.y);

        // Transform half-vector back to world space
        Vector3f wh = onb.ToWorld(localWh);

        // Reflect wo about wh to get wi
        *wi = Reflect(wo, wh);

        if (Dot(*wi, n) <= 0.0f) {
            *pdf = 0.0f;
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        float NdotV = AbsDot(wo, n);
        float NdotH = AbsDot(wh, n);
        float VdotH = AbsDot(wo, wh);

        *pdf = GgxVndfPdf(NdotV, NdotH, VdotH, alpha_);

        // Do not hand-reassemble D*G*F here; delegate to f()
        return f(wo, *wi, n);
    }
}
