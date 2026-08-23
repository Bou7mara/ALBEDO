#include "rt/materials/dielectric.h"
#include "rt/materials/fresnel.h"
#include <algorithm>

namespace rt {

    namespace {
        constexpr float kChannelWavelengthUm[3] = { 0.630f, 0.532f, 0.465f }; // R, G, B
    }

    Dielectric::Dielectric(float ior, const Vector3f& tint, float dispersion)
        : tint_(tint), dispersion_(dispersion) {
        if (dispersion > 0.0f) {
            sellmeier_ = SellmeierCoefficients::MakeCauchy(ior, dispersion);
        } else {
            sellmeier_ = SellmeierCoefficients::MakeConstant(ior);
        }
    }

    Dielectric::Dielectric(const SellmeierCoefficients& sellmeier, const Vector3f& tint)
        : sellmeier_(sellmeier), tint_(tint), dispersion_(sellmeier.hasDispersion ? 1.0f : 0.0f) {
    }

    Vector3f Dielectric::f(const Vector3f&, const Vector3f&, const Vector3f&, const Point2f&) const {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    Vector3f Dielectric::Sample_f(const Vector3f& wo, const Vector3f& n,
                                  const Point2f& u, Vector3f* wi, float* pdf, const Point2f&) const {
        bool entering = Dot(wo, n) > 0.0f;
        float iorBase = SellmeierIOR(sellmeier_, 0.5893f);
        float etaI = entering ? 1.0f : iorBase;
        float etaT = entering ? iorBase : 1.0f;
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

        if (!sellmeier_.hasDispersion) {
            // Exact code path for non-dispersive dielectrics
            Vector3f nf = entering ? n : -n;
            float eta = etaI / etaT;

            if (!Refract(wo, nf, eta, wi)) {
                // TIR fallback
                *wi = Reflect(wo, n);
                *pdf = 1.0f;
                float cosThetaWi = AbsDot(*wi, n);
                return (cosThetaWi < 1e-6f) ? Vector3f(0.0f, 0.0f, 0.0f) : Vector3f(1.0f, 1.0f, 1.0f) / cosThetaWi;
            }

            float cosThetaWi = AbsDot(*wi, n);
            if (cosThetaWi < 1e-6f) {
                return Vector3f(0.0f, 0.0f, 0.0f);
            }

            Vector3f T = tint_ * (1.0f - R);
            return T / (eta * eta) / cosThetaWi;
        }

        // 3-channel Cauchy fallback for legacy non-hero single paths
        int channel = std::min(2, static_cast<int>(u.y * 3.0f));
        float lambdaUm = kChannelWavelengthUm[channel];
        float iorChannel = SellmeierIOR(sellmeier_, lambdaUm);

        float etaI_c = entering ? 1.0f : iorChannel;
        float etaT_c = entering ? iorChannel : 1.0f;
        float eta = etaI_c / etaT_c;
        Vector3f nf = entering ? n : -n;

        if (!Refract(wo, nf, eta, wi)) {
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

    bool Dielectric::Sample_HeroWavelengths(const Vector3f& wo, const Vector3f& n, const Point2f& u,
                                            const HeroWavelengths& hw, Vector3f* wi, float* pdfHero,
                                            float throughputWeights[4]) const {
        bool entering = Dot(wo, n) > 0.0f;
        Vector3f nf = entering ? n : -n;
        float cosThetaI = AbsDot(wo, n);

        // Hero wavelength IOR (slot 0)
        float lambda0Um = hw.lambda[0] * 0.001f;
        float iorHero = SellmeierIOR(sellmeier_, lambda0Um);
        float etaI0 = entering ? 1.0f : iorHero;
        float etaT0 = entering ? iorHero : 1.0f;
        float eta0 = etaI0 / etaT0;

        float R0 = FrDielectric(cosThetaI, etaI0, etaT0);
        bool heroReflect = (u.x < R0);

        if (heroReflect) {
            *wi = Reflect(wo, n);
            *pdfHero = R0;
        } else {
            if (!Refract(wo, nf, eta0, wi)) {
                // Hero TIR fallback
                *wi = Reflect(wo, n);
                heroReflect = true;
                *pdfHero = 1.0f;
            } else {
                *pdfHero = 1.0f - R0;
            }
        }

        float cosThetaWi = AbsDot(*wi, n);
        if (cosThetaWi < 1e-6f) {
            for (int i = 0; i < 4; ++i) throughputWeights[i] = 0.0f;
            return false;
        }

        // Evaluate all 4 companion wavelengths along the hero ray direction
        for (int i = 0; i < 4; ++i) {
            float lambdaUm = hw.lambda[i] * 0.001f;
            float ior_i = SellmeierIOR(sellmeier_, lambdaUm);
            float etaI_i = entering ? 1.0f : ior_i;
            float etaT_i = entering ? ior_i : 1.0f;
            float eta_i = etaI_i / etaT_i;

            float Ri = FrDielectric(cosThetaI, etaI_i, etaT_i);
            float tintWeight = RgbToSpectrum(tint_, hw.lambda[i]);

            if (heroReflect) {
                // Hero reflected: companion wavelength contribution
                float weight = (*pdfHero > 1e-6f) ? (Ri / *pdfHero) : 0.0f;
                throughputWeights[i] = tintWeight * weight;
            } else {
                // Hero refracted: check if companion wavelength would undergo TIR
                float sin2ThetaI = std::max(0.0f, 1.0f - cosThetaI * cosThetaI);
                float sin2ThetaT_i = (eta_i * eta_i) * sin2ThetaI;
                if (sin2ThetaT_i >= 1.0f) {
                    // Companion wavelength undergoes TIR at this angle, cannot transmit
                    throughputWeights[i] = 0.0f;
                } else {
                    float weight = (*pdfHero > 1e-6f) ? (((1.0f - Ri) / *pdfHero) / (eta_i * eta_i)) : 0.0f;
                    throughputWeights[i] = tintWeight * weight;
                }
            }
        }

        return true;
    }

    float Dielectric::Pdf(const Vector3f&, const Vector3f&, const Vector3f&, const Point2f&) const {
        return 0.0f;
    }

} // namespace rt