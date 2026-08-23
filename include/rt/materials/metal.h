#pragma once
#include "rt/materials/bsdf.h"
#include "rt/textures/image2d.h"
#include <memory>

namespace rt {
    class Metal : public BSDF {
    public:
        explicit Metal(const Vector3f& albedo) : albedo_(albedo) {}
        explicit Metal(std::shared_ptr<Image2D<Vector3f>> albedoTexture)
            : albedo_(Vector3f(1.0f, 1.0f, 1.0f)), albedoTexture_(std::move(albedoTexture)) {}

        Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        Vector3f Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv = Point2f(0.0f, 0.0f)) const override;

        const Vector3f& Albedo() const { return albedo_; }
        const std::shared_ptr<Image2D<Vector3f>>& AlbedoTexture() const { return albedoTexture_; }

    private:
        Vector3f albedo_;
        std::shared_ptr<Image2D<Vector3f>> albedoTexture_ = nullptr;
    };
}
