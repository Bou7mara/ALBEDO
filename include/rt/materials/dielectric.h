#pragma once
#include "rt/materials/bsdf.h"
#include "rt/materials/sellmeier.h"
#include "rt/spectral/spectrum.h"

namespace rt {

    class Dielectric : public BSDF {
    public:
        // Default constructor with constant IOR or dispersion factor (backward-compatible)
        explicit Dielectric(float ior, const Vector3f& tint = Vector3f(1.0f, 1.0f, 1.0f), float dispersion = 0.0f);

        // Constructor with explicit Sellmeier coefficients
        explicit Dielectric(const SellmeierCoefficients& sellmeier, const Vector3f& tint = Vector3f(1.0f, 1.0f, 1.0f));

        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                          const Point2f& u, Vector3f* wi, float* pdf, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        // Dispersive hero wavelength sampling:
        // Steers the ray geometry wi according to hero wavelength hw.lambda[0],
        // and computes per-wavelength throughput factors for all 4 hero wavelengths.
        bool Sample_HeroWavelengths(const Vector3f& wo, const Vector3f& n, const Point2f& u,
                                    const HeroWavelengths& hw, Vector3f* wi, float* pdfHero,
                                    float throughputWeights[4]) const;

        float Ior() const { return SellmeierIOR(sellmeier_, 0.5893f); }
        const Vector3f& Tint() const { return tint_; }
        float Dispersion() const { return dispersion_; }
        const SellmeierCoefficients& Sellmeier() const { return sellmeier_; }
        bool HasDispersion() const { return sellmeier_.hasDispersion; }

    private:
        SellmeierCoefficients sellmeier_;
        Vector3f tint_;
        float dispersion_;
    };

} // namespace rt