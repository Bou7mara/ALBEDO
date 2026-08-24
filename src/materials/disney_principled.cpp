#include "rt/materials/disney_principled.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"
#include "rt/materials/fresnel.h"
#include "rt/materials/microfacet.h"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace rt {

namespace {

    inline float Sq(float x) { return x * x; }

    inline float SchlickWeight(float cosTheta) {
        float m = std::clamp(1.0f - cosTheta, 0.0f, 1.0f);
        float m2 = m * m;
        return m2 * m2 * m;
    }

    inline float Luminance(const Vector3f& c) {
        return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
    }

    inline float Gtr1D(float NdotH, float a) {
        if (a >= 1.0f) return std::numbers::inv_pi_v<float>;
        float a2 = a * a;
        float t = 1.0f + (a2 - 1.0f) * NdotH * NdotH;
        return (a2 - 1.0f) / (std::numbers::pi_v<float> * std::log(a2) * t);
    }

    inline float Gtr1SmithG(float NdotV, float NdotL) {

        return SmithG(NdotV, NdotL, 0.25f);
    }

    inline Vector3f SampleGtr1(const Point2f& u, float alpha) {
        float alpha2 = alpha * alpha;
        float cosTheta2 = (alpha2 < 1.0f) ? ((1.0f - std::pow(alpha2, 1.0f - u.x)) / (1.0f - alpha2)) : (1.0f - u.x);
        cosTheta2 = std::clamp(cosTheta2, 0.0f, 1.0f);
        float cosTheta = std::sqrt(cosTheta2);
        float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta2));
        float phi = 2.0f * kPi * u.y;
        return Vector3f(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
    }

    struct LobeWeights {
        float wDiff;
        float wSpec;
        float wClear;
    };

    inline LobeWeights ComputeLobeWeights(const DisneyParams& params) {
        float diffWeight = (1.0f - params.metallic) * (1.0f - params.subsurface * 0.5f);
        float specWeight = std::max(0.05f, params.specular + params.metallic);
        float clearWeight = 0.25f * params.clearcoat;
        float total = diffWeight + specWeight + clearWeight;
        if (total <= 0.0f) return {1.0f, 0.0f, 0.0f};
        return {diffWeight / total, specWeight / total, clearWeight / total};
    }

    inline DisneyParams ResolveDisneyParams(const DisneyParams& p, const Point2f& uv) {
        DisneyParams res = p;
        if (p.baseColorTexture) res.baseColor = p.baseColorTexture->Sample(uv.x, uv.y);
        if (p.roughnessTexture) res.roughness = p.roughnessTexture->Sample(uv.x, uv.y);
        if (p.metallicTexture)  res.metallic  = p.metallicTexture->Sample(uv.x, uv.y);
        return res;
    }

}

Vector3f DisneyPrincipled::f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv) const {
    float NdotO = Dot(wo, n);
    float NdotI = Dot(wi, n);
    if (NdotO <= 0.0f || NdotI <= 0.0f) {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    DisneyParams p = ResolveDisneyParams(params_, uv);

    Vector3f wh = wo + wi;
    if (LengthSquared(wh) == 0.0f) return Vector3f(0.0f, 0.0f, 0.0f);
    wh = Normalize(wh);

    float NdotH = Dot(wh, n);
    float VdotH = Dot(wo, wh);

    float lum = Luminance(p.baseColor);
    Vector3f cTint = (lum > 0.0f) ? (p.baseColor / lum) : Vector3f(1.0f, 1.0f, 1.0f);

    Vector3f fDiffuse(0.0f, 0.0f, 0.0f);
    if (p.metallic < 1.0f) {
        float Fo = SchlickWeight(NdotO);
        float Fi = SchlickWeight(NdotI);
        float Fd90 = 0.5f + 2.0f * p.roughness * VdotH * VdotH;
        float Fd = (1.0f + (Fd90 - 1.0f) * Fo) * (1.0f + (Fd90 - 1.0f) * Fi);

        float Fss90 = p.roughness * VdotH * VdotH;
        float Fss = (1.0f + (Fss90 - 1.0f) * Fo) * (1.0f + (Fss90 - 1.0f) * Fi);
        float ss = 1.25f * (Fss * (1.0f / (NdotO + NdotI) - 0.5f) + 0.5f);

        float diffuseFactor = std::lerp(Fd, ss, p.subsurface);
        fDiffuse = p.baseColor * (std::numbers::inv_pi_v<float> * diffuseFactor * (1.0f - p.metallic));

        if (p.sheen > 0.0f) {
            float Fh = SchlickWeight(VdotH);
            Vector3f cSheen = (1.0f - p.sheenTint) * Vector3f(1.0f, 1.0f, 1.0f) + p.sheenTint * cTint;
            Vector3f fSheen = p.sheen * cSheen * Fh * (1.0f - p.metallic);
            fDiffuse += fSheen;
        }
    }

    Vector3f fSpec(0.0f, 0.0f, 0.0f);
    float alpha = std::max(0.001f, Sq(p.roughness));
    float D = GgxD(NdotH, alpha);
    float G = SmithG(NdotO, NdotI, alpha);

    Vector3f cSpec0 = 0.08f * p.specular * ((1.0f - p.specularTint) * Vector3f(1.0f, 1.0f, 1.0f) + p.specularTint * cTint);
    Vector3f F0 = (1.0f - p.metallic) * cSpec0 + p.metallic * p.baseColor;
    Vector3f F = F0 + (Vector3f(1.0f, 1.0f, 1.0f) - F0) * SchlickWeight(VdotH);

    fSpec = (D * G * F) / (4.0f * NdotO * NdotI);

    Vector3f fClearcoat(0.0f, 0.0f, 0.0f);
    if (p.clearcoat > 0.0f) {
        float alphaG = std::lerp(0.1f, 0.001f, p.clearcoatGloss);
        float Dc = Gtr1D(NdotH, alphaG);
        float Gc = Gtr1SmithG(NdotO, NdotI);
        float Fc = 0.04f + (1.0f - 0.04f) * SchlickWeight(VdotH);
        float cVal = 0.25f * p.clearcoat * (Dc * Gc * Fc) / (4.0f * NdotO * NdotI);
        fClearcoat = Vector3f(cVal, cVal, cVal);
    }

    return fDiffuse + fSpec + fClearcoat;
}

Vector3f DisneyPrincipled::Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf, const Point2f& uv) const {
    float NdotO = Dot(wo, n);
    if (NdotO <= 0.0f) {
        *pdf = 0.0f;
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    DisneyParams p = ResolveDisneyParams(params_, uv);
    LobeWeights w = ComputeLobeWeights(p);
    ONB onb(n);

    if (u.x < w.wDiff) {

        float remappedUx = u.x / w.wDiff;
        Point2f uSub(remappedUx, u.y);
        Vector3f localDir = CosineSampleHemisphere(uSub);
        *wi = Normalize(onb.ToWorld(localDir));
    } else if (u.x < w.wDiff + w.wSpec) {

        float remappedUx = (u.x - w.wDiff) / w.wSpec;
        Point2f uSub(remappedUx, u.y);
        float alpha = std::max(0.001f, Sq(p.roughness));
        Vector3f localWh = SampleGgx(uSub, alpha);
        Vector3f wh = Normalize(onb.ToWorld(localWh));
        *wi = Normalize(2.0f * Dot(wo, wh) * wh - wo);
    } else {

        float remappedUx = (u.x - (w.wDiff + w.wSpec)) / w.wClear;
        Point2f uSub(remappedUx, u.y);
        float alphaG = std::lerp(0.1f, 0.001f, p.clearcoatGloss);
        Vector3f localWh = SampleGtr1(uSub, alphaG);
        Vector3f wh = Normalize(onb.ToWorld(localWh));
        *wi = Normalize(2.0f * Dot(wo, wh) * wh - wo);
    }

    if (Dot(*wi, n) <= 0.0f) {
        *pdf = 0.0f;
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    *pdf = Pdf(wo, *wi, n, uv);
    if (*pdf <= 0.0f) {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    return f(wo, *wi, n, uv);
}

float DisneyPrincipled::Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, const Point2f& uv) const {
    float NdotO = Dot(wo, n);
    float NdotI = Dot(wi, n);
    if (NdotO <= 0.0f || NdotI <= 0.0f) {
        return 0.0f;
    }

    Vector3f wh = wo + wi;
    if (LengthSquared(wh) == 0.0f) return 0.0f;
    wh = Normalize(wh);

    float NdotH = Dot(wh, n);
    float VdotH = Dot(wo, wh);
    if (VdotH <= 0.0f) return 0.0f;

    DisneyParams p = ResolveDisneyParams(params_, uv);
    LobeWeights w = ComputeLobeWeights(p);

    float pdfDiff = CosineHemispherePdf(NdotI);

    float alpha = std::max(0.001f, Sq(p.roughness));
    float D = GgxD(NdotH, alpha);
    float pdfSpec = (D * NdotH) / (4.0f * VdotH);

    float alphaG = std::lerp(0.1f, 0.001f, p.clearcoatGloss);
    float Dc = Gtr1D(NdotH, alphaG);
    float pdfClear = (Dc * NdotH) / (4.0f * VdotH);

    return w.wDiff * pdfDiff + w.wSpec * pdfSpec + w.wClear * pdfClear;
}

}
