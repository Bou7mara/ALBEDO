#pragma once
#include "rt/materials/bsdf.h"
#include "rt/textures/image2d.h"
#include <memory>

namespace rt {
    class Microfacet : public BSDF {
    public:
        static Microfacet MakeDielectricMicrofacet(float roughness, float ior);

        static Microfacet MakeConductorMicrofacet(float roughness,
                                                  const Vector3f& eta,
                                                  const Vector3f& k,
                                                  const Vector3f& tint = Vector3f(1.0f, 1.0f, 1.0f));

        static Microfacet MakeConductorMicrofacetTextured(std::shared_ptr<Image2D<float>> roughnessTexture,
                                                          const Vector3f& eta,
                                                          const Vector3f& k,
                                                          std::shared_ptr<Image2D<Vector3f>> tintTexture = nullptr);

        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                          const Point2f& u, Vector3f* wi, float* pdf, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        float Alpha() const { return alpha_; }
        bool IsDielectric() const { return kind_ == FresnelKind::Dielectric; }
        float Ior() const { return ior_; }
        const Vector3f& Eta() const { return eta_; }
        const Vector3f& K() const { return k_; }
        const Vector3f& Tint() const { return tint_; }
        const std::shared_ptr<Image2D<float>>& RoughnessTexture() const { return roughnessTexture_; }
        const std::shared_ptr<Image2D<Vector3f>>& TintTexture() const { return tintTexture_; }

    private:
        enum class FresnelKind { Dielectric, Conductor };

        Microfacet(float roughness, FresnelKind kind,
                   float ior, const Vector3f& eta, const Vector3f& k, const Vector3f& tint,
                   std::shared_ptr<Image2D<float>> roughnessTexture = nullptr,
                   std::shared_ptr<Image2D<Vector3f>> tintTexture = nullptr);

        Vector3f EvaluateFresnel(float cosThetaI) const;

        float alpha_;
        FresnelKind kind_;
        float ior_;
        Vector3f eta_, k_;
        Vector3f tint_;
        std::shared_ptr<Image2D<float>> roughnessTexture_ = nullptr;
        std::shared_ptr<Image2D<Vector3f>> tintTexture_ = nullptr;
    };
}
