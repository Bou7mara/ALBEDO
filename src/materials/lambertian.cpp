#include "rt/materials/lambertian.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace rt {

Vector3f Lambertian::f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv) const {
    if (Dot(wo, n) * Dot(wi, n) < 0.0f) {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    Vector3f effectiveAlbedo = albedoTexture_ ? albedoTexture_->Sample(uv.x, uv.y) : albedo_;

    if (roughness_ <= 1e-4f) {
        return effectiveAlbedo * std::numbers::inv_pi_v<float>;
    }

    float cosThetaO = std::clamp(Dot(wo, n), 0.0f, 1.0f);
    float cosThetaI = std::clamp(Dot(wi, n), 0.0f, 1.0f);
    float sinThetaO = std::sqrt(std::max(0.0f, 1.0f - cosThetaO * cosThetaO));
    float sinThetaI = std::sqrt(std::max(0.0f, 1.0f - cosThetaI * cosThetaI));

    float sinAlpha, tanBeta;
    if (cosThetaI < cosThetaO) {
        sinAlpha = sinThetaI;
        tanBeta = (cosThetaO > 1e-6f) ? (sinThetaO / cosThetaO) : 0.0f;
    } else {
        sinAlpha = sinThetaO;
        tanBeta = (cosThetaI > 1e-6f) ? (sinThetaI / cosThetaI) : 0.0f;
    }

    float cosPhiDiff = 0.0f;
    if (sinThetaI > 1e-6f && sinThetaO > 1e-6f) {
        Vector3f vO = Normalize(wo - cosThetaO * n);
        Vector3f vI = Normalize(wi - cosThetaI * n);
        cosPhiDiff = Dot(vO, vI);
    }

    float sigma2 = roughness_ * roughness_;
    float A = 1.0f - (sigma2 / (2.0f * (sigma2 + 0.33f)));
    float B = 0.45f * sigma2 / (sigma2 + 0.09f);

    float orenNayar = A + B * std::max(0.0f, cosPhiDiff) * sinAlpha * tanBeta;
    return effectiveAlbedo * (std::numbers::inv_pi_v<float> * orenNayar);
}

Vector3f Lambertian::Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf, const Point2f& uv) const {
    ONB onb(n);
    Vector3f localDir = CosineSampleHemisphere(u);
    *wi = Normalize(onb.ToWorld(localDir));
    *pdf = CosineHemispherePdf(localDir.z);
    return f(wo, *wi, n, uv);
}

float Lambertian::Pdf(const Vector3f&, const Vector3f& wi, const Vector3f& n, const Point2f&) const {
    if (Dot(wi, n) <= 0.0f) {
        return 0.0f;
    }
    return CosineHemispherePdf(AbsDot(wi, n));
}
}
