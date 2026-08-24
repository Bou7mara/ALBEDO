#pragma once
#include "rt/materials/bsdf.h"
#include "rt/textures/image2d.h"
#include <algorithm>
#include <memory>

namespace rt {

    struct DisneyParams {
        Vector3f baseColor{0.8f, 0.8f, 0.8f};
        float metallic = 0.0f;
        float subsurface = 0.0f;
        float specular = 0.5f;
        float roughness = 0.5f;
        float specularTint = 0.0f;
        float anisotropic = 0.0f;
        float sheen = 0.0f;
        float sheenTint = 0.5f;
        float clearcoat = 0.0f;
        float clearcoatGloss = 1.0f;
        std::shared_ptr<Image2D<Vector3f>> baseColorTexture = nullptr;
        std::shared_ptr<Image2D<float>> roughnessTexture = nullptr;
        std::shared_ptr<Image2D<float>> metallicTexture = nullptr;
    };

    class DisneyPrincipled : public BSDF {
    public:
        explicit DisneyPrincipled(const DisneyParams& params = DisneyParams{})
            : params_(params) {}

        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        const DisneyParams& Params() const { return params_; }

    private:
        DisneyParams params_;
    };

}
