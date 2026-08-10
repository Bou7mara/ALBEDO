#include "rt/materials/dielectric.h"
#include "rt/materials/fresnel.h"
#include <algorithm>

namespace rt {

    namespace {
        constexpr float kChannelWavelengthUm[3] = { 0.630f, 0.532f, 0.465f }; // R, G, B
    }

    Vector3f Dielectric::f(const Vector3f&, const Vector3f&, const Vector3f&) const {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    Vector3f Dielectric::Sample_f(const Vector3f& wo, const Vector3f& n,
                                  const Point2f& u, Vector3f* wi, float* pdf) const {

        bool entering = Dot(wo, n) > 0.0f;
        float etaI = entering ? 1.0f : ior_;
        float etaT = entering ? ior_ : 1.0f;
        float cosThetaI = AbsDot(wo, n);

        float R = FrDielectric(cosThetaI, etaI, etaT);

        if (u.x < R) {
            // Reflection: Stays untinted (white specular reflections)
            *wi = Reflect(wo, n);
            *pdf = R;

            float cosThetaWi = AbsDot(*wi, n);
            if (cosThetaWi < 1e-6f) {
                return Vector3f(0.0f, 0.0f, 0.0f);
            }

            return Vector3f(R, R, R) / cosThetaWi;
        }

        *pdf = 1.0f - R;

        if (dispersion_ <= 0.0f) {
            // Exact existing code path for non-dispersive dielectrics
            Vector3f nf = entering ? n : -n;
            float eta = etaI / etaT;

            Refract(wo, nf, eta, wi);

            float cosThetaWi = AbsDot(*wi, n);
            if (cosThetaWi < 1e-6f) {
                return Vector3f(0.0f, 0.0f, 0.0f);
            }

            Vector3f T = tint_ * (1.0f - R);
            return T / (eta * eta) / cosThetaWi;
        }

        // Dispersive transmit branch
        int channel = std::min(2, static_cast<int>(u.y * 3.0f));
        float lambdaUm = kChannelWavelengthUm[channel];
        float iorChannel = CauchyIOR(ior_, dispersion_, lambdaUm);

        float etaI_c = entering ? 1.0f : iorChannel;
        float etaT_c = entering ? iorChannel : 1.0f;
        float eta = etaI_c / etaT_c;
        Vector3f nf = entering ? n : -n;

        if (!Refract(wo, nf, eta, wi)) {
            // Per-channel TIR fallback
            *wi = Reflect(wo, n);
            float cosThetaWi = AbsDot(*wi, n);
            if (cosThetaWi < 1e-6f) {
                return Vector3f(0.0f, 0.0f, 0.0f);
            }
            Vector3f result(0.0f, 0.0f, 0.0f);
            result[channel] = 3.0f / cosThetaWi;
            return result;
        }

        float cosThetaWi = AbsDot(*wi, n);
        if (cosThetaWi < 1e-6f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        Vector3f result(0.0f, 0.0f, 0.0f);
        result[channel] = tint_[channel] * (1.0f - R) * 3.0f / (eta * eta) / cosThetaWi;
        return result;
    }

    float Dielectric::Pdf(const Vector3f&, const Vector3f&, const Vector3f&) const {
        return 0.0f;
    }

} // namespace rt