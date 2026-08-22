#pragma once

#include "rt/core/vector3.h"
#include "rt/core/point2.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"
#include "rt/materials/fresnel.h"
#include "rt/materials/microfacet.h"

#include <algorithm>
#include <cmath>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rtx {

    enum class MaterialKind : uint32_t {
        Lambertian = 0,
        Metal = 1,
        Dielectric = 2,
        MicrofacetDielectric = 3,
        MicrofacetConductor = 4,
        Emissive = 5,
        Disney = 6
    };

    struct DeviceMaterial {
        MaterialKind kind = MaterialKind::Lambertian;
        union {
            struct { rt::Vector3f albedo; float roughness; } lambertian;
            struct { rt::Vector3f albedo; } metal;
            struct { float ior; rt::Vector3f tint; float dispersion; } dielectric;
            struct { float alpha; float ior; } microfacetDielectric;
            struct { float alpha; rt::Vector3f eta; rt::Vector3f k; rt::Vector3f tint; } microfacetConductor;
            struct { rt::Vector3f radiance; } emissive;
            struct {
                rt::Vector3f baseColor;
                float metallic;
                float subsurface;
                float specular;
                float roughness;
                float specularTint;
                float anisotropic;
                float sheen;
                float sheenTint;
                float clearcoat;
                float clearcoatGloss;
            } disney;
        };

        __host__ __device__ constexpr DeviceMaterial()
            : kind(MaterialKind::Lambertian), lambertian{rt::Vector3f(0.0f, 0.0f, 0.0f), 0.0f} {}

        __host__ __device__ static DeviceMaterial MakeLambertian(const rt::Vector3f& albedo, float roughness = 0.0f) {
            DeviceMaterial m{};
            m.kind = MaterialKind::Lambertian;
            m.lambertian.albedo = albedo;
            m.lambertian.roughness = roughness;
            return m;
        }

        __host__ __device__ static DeviceMaterial MakeMetal(const rt::Vector3f& albedo) {
            DeviceMaterial m{};
            m.kind = MaterialKind::Metal;
            m.metal.albedo = albedo;
            return m;
        }

        __host__ __device__ static DeviceMaterial MakeDielectric(float ior, const rt::Vector3f& tint = rt::Vector3f(1.0f, 1.0f, 1.0f), float dispersion = 0.0f) {
            DeviceMaterial m{};
            m.kind = MaterialKind::Dielectric;
            m.dielectric.ior = ior;
            m.dielectric.tint = tint;
            m.dielectric.dispersion = dispersion;
            return m;
        }

        __host__ __device__ static DeviceMaterial MakeMicrofacetDielectric(float roughness, float ior) {
            DeviceMaterial m{};
            m.kind = MaterialKind::MicrofacetDielectric;
            m.microfacetDielectric.alpha = rt::AlphaFromRoughness(roughness);
            m.microfacetDielectric.ior = ior;
            return m;
        }

        __host__ __device__ static DeviceMaterial MakeMicrofacetConductor(float roughness, const rt::Vector3f& eta, const rt::Vector3f& k, const rt::Vector3f& tint = rt::Vector3f(1.0f, 1.0f, 1.0f)) {
            DeviceMaterial m{};
            m.kind = MaterialKind::MicrofacetConductor;
            m.microfacetConductor.alpha = rt::AlphaFromRoughness(roughness);
            m.microfacetConductor.eta = eta;
            m.microfacetConductor.k = k;
            m.microfacetConductor.tint = tint;
            return m;
        }

        __host__ __device__ static DeviceMaterial MakeEmissive(const rt::Vector3f& radiance) {
            DeviceMaterial m{};
            m.kind = MaterialKind::Emissive;
            m.emissive.radiance = radiance;
            return m;
        }

        __host__ __device__ static DeviceMaterial MakeDisney(const rt::Vector3f& baseColor,
                                                             float metallic = 0.0f,
                                                             float subsurface = 0.0f,
                                                             float specular = 0.5f,
                                                             float roughness = 0.5f,
                                                             float specularTint = 0.0f,
                                                             float anisotropic = 0.0f,
                                                             float sheen = 0.0f,
                                                             float sheenTint = 0.5f,
                                                             float clearcoat = 0.0f,
                                                             float clearcoatGloss = 1.0f) {
            DeviceMaterial m{};
            m.kind = MaterialKind::Disney;
            m.disney.baseColor = baseColor;
            m.disney.metallic = metallic;
            m.disney.subsurface = subsurface;
            m.disney.specular = specular;
            m.disney.roughness = roughness;
            m.disney.specularTint = specularTint;
            m.disney.anisotropic = anisotropic;
            m.disney.sheen = sheen;
            m.disney.sheenTint = sheenTint;
            m.disney.clearcoat = clearcoat;
            m.disney.clearcoatGloss = clearcoatGloss;
            return m;
        }
    };

    __host__ __device__ inline float DisneySchlickWeight(float cosTheta) {
        float m = fmaxf(0.0f, fminf(1.0f, 1.0f - cosTheta));
        float m2 = m * m;
        return m2 * m2 * m;
    }

    __host__ __device__ inline float DisneyLuminance(const rt::Vector3f& c) {
        return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
    }

    __host__ __device__ inline float DisneyGtr1D(float NdotH, float a) {
        if (a >= 1.0f) return rt::kInvPi;
        float a2 = a * a;
        float t = 1.0f + (a2 - 1.0f) * NdotH * NdotH;
        return (a2 - 1.0f) / (rt::kPi * logf(a2) * t);
    }

    __host__ __device__ inline float DisneyGtr1SmithG(float NdotV, float NdotL) {
        return rt::SmithG(NdotV, NdotL, 0.25f);
    }

    __host__ __device__ inline rt::Vector3f DisneySampleGtr1(const rt::Point2f& u, float alpha) {
        float alpha2 = alpha * alpha;
        float cosTheta2 = (alpha2 < 1.0f) ? ((1.0f - powf(alpha2, 1.0f - u.x)) / (1.0f - alpha2)) : (1.0f - u.x);
        cosTheta2 = fmaxf(0.0f, fminf(1.0f, cosTheta2));
        float cosTheta = sqrtf(cosTheta2);
        float sinTheta = sqrtf(fmaxf(0.0f, 1.0f - cosTheta2));
        float phi = 2.0f * rt::kPi * u.y;
        return rt::Vector3f(sinTheta * cosf(phi), sinTheta * sinf(phi), cosTheta);
    }

    struct DisneyLobeWeights {
        float wDiff;
        float wSpec;
        float wClear;
    };

    __host__ __device__ inline DisneyLobeWeights ComputeDisneyLobeWeights(const DeviceMaterial& mat) {
        float diffWeight = (1.0f - mat.disney.metallic) * (1.0f - mat.disney.subsurface * 0.5f);
        float specWeight = fmaxf(0.05f, mat.disney.specular + mat.disney.metallic);
        float clearWeight = 0.25f * mat.disney.clearcoat;
        float total = diffWeight + specWeight + clearWeight;
        if (total <= 0.0f) return {1.0f, 0.0f, 0.0f};
        return {diffWeight / total, specWeight / total, clearWeight / total};
    }

    __host__ __device__ inline rt::Vector3f EvaluateBsdf(const DeviceMaterial& mat,
                                                         const rt::Vector3f& wo,
                                                         const rt::Vector3f& wi,
                                                         const rt::Vector3f& n) {
        switch (mat.kind) {
            case MaterialKind::Lambertian: {
                if (Dot(wi, n) <= 0.0f || Dot(wo, n) <= 0.0f) {
                    return rt::Vector3f(0.0f, 0.0f, 0.0f);
                }
                if (mat.lambertian.roughness <= 1e-4f) {
                    return mat.lambertian.albedo * rt::kInvPi;
                }
                float cosThetaO = rt::AbsDot(wo, n);
                float cosThetaI = rt::AbsDot(wi, n);
                float sinThetaO = std::sqrt(fmaxf(0.0f, 1.0f - cosThetaO * cosThetaO));
                float sinThetaI = std::sqrt(fmaxf(0.0f, 1.0f - cosThetaI * cosThetaI));

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
                    rt::Vector3f vO = Normalize(wo - cosThetaO * n);
                    rt::Vector3f vI = Normalize(wi - cosThetaI * n);
                    cosPhiDiff = Dot(vO, vI);
                }

                float sigma2 = mat.lambertian.roughness * mat.lambertian.roughness;
                float A = 1.0f - (sigma2 / (2.0f * (sigma2 + 0.33f)));
                float B = 0.45f * sigma2 / (sigma2 + 0.09f);

                float orenNayar = A + B * fmaxf(0.0f, cosPhiDiff) * sinAlpha * tanBeta;
                return mat.lambertian.albedo * (rt::kInvPi * orenNayar);
            }
            case MaterialKind::Metal:
            case MaterialKind::Dielectric:
            case MaterialKind::Emissive:
                return rt::Vector3f(0.0f, 0.0f, 0.0f);

            case MaterialKind::MicrofacetDielectric: {
                float NdotV = AbsDot(wo, n);
                float NdotL = AbsDot(wi, n);
                if (NdotV < 1e-4f || NdotL < 1e-4f) return rt::Vector3f(0.0f, 0.0f, 0.0f);

                rt::Vector3f wh = wo + wi;
                if (LengthSquared(wh) == 0.0f) return rt::Vector3f(0.0f, 0.0f, 0.0f);
                wh = Normalize(wh);

                float NdotH = AbsDot(wh, n);
                float D = rt::GgxD(NdotH, mat.microfacetDielectric.alpha);
                float Gterm = rt::SmithG(NdotV, NdotL, mat.microfacetDielectric.alpha);
                float VdotH = AbsDot(wo, wh);
                float f = rt::FrDielectric(VdotH, 1.0f, mat.microfacetDielectric.ior);
                return D * Gterm * rt::Vector3f(f, f, f);
            }

            case MaterialKind::MicrofacetConductor: {
                float NdotV = AbsDot(wo, n);
                float NdotL = AbsDot(wi, n);
                if (NdotV < 1e-4f || NdotL < 1e-4f) return rt::Vector3f(0.0f, 0.0f, 0.0f);

                rt::Vector3f wh = wo + wi;
                if (LengthSquared(wh) == 0.0f) return rt::Vector3f(0.0f, 0.0f, 0.0f);
                wh = Normalize(wh);

                float NdotH = AbsDot(wh, n);
                float D = rt::GgxD(NdotH, mat.microfacetConductor.alpha);
                float Gterm = rt::SmithG(NdotV, NdotL, mat.microfacetConductor.alpha);
                float VdotH = AbsDot(wo, wh);

                float fr = rt::FrConductor(VdotH, mat.microfacetConductor.eta.x, mat.microfacetConductor.k.x);
                float fg = rt::FrConductor(VdotH, mat.microfacetConductor.eta.y, mat.microfacetConductor.k.y);
                float fb = rt::FrConductor(VdotH, mat.microfacetConductor.eta.z, mat.microfacetConductor.k.z);
                rt::Vector3f F = mat.microfacetConductor.tint * rt::Vector3f(fr, fg, fb);
                return D * Gterm * F;
            }

            case MaterialKind::Disney: {
                float NdotO = Dot(wo, n);
                float NdotI = Dot(wi, n);
                if (NdotO <= 0.0f || NdotI <= 0.0f) {
                    return rt::Vector3f(0.0f, 0.0f, 0.0f);
                }

                rt::Vector3f wh = wo + wi;
                if (LengthSquared(wh) == 0.0f) return rt::Vector3f(0.0f, 0.0f, 0.0f);
                wh = Normalize(wh);

                float NdotH = Dot(wh, n);
                float VdotH = Dot(wo, wh);

                float lum = DisneyLuminance(mat.disney.baseColor);
                rt::Vector3f cTint = (lum > 0.0f) ? (mat.disney.baseColor / lum) : rt::Vector3f(1.0f, 1.0f, 1.0f);

                // 1. Diffuse & Subsurface
                rt::Vector3f fDiffuse(0.0f, 0.0f, 0.0f);
                if (mat.disney.metallic < 1.0f) {
                    float Fo = DisneySchlickWeight(NdotO);
                    float Fi = DisneySchlickWeight(NdotI);
                    float Fd90 = 0.5f + 2.0f * mat.disney.roughness * VdotH * VdotH;
                    float Fd = (1.0f + (Fd90 - 1.0f) * Fo) * (1.0f + (Fd90 - 1.0f) * Fi);

                    float Fss90 = mat.disney.roughness * VdotH * VdotH;
                    float Fss = (1.0f + (Fss90 - 1.0f) * Fo) * (1.0f + (Fss90 - 1.0f) * Fi);
                    float ss = 1.25f * (Fss * (1.0f / (NdotO + NdotI) - 0.5f) + 0.5f);

                    float diffuseFactor = (1.0f - mat.disney.subsurface) * Fd + mat.disney.subsurface * ss;
                    fDiffuse = mat.disney.baseColor * (rt::kInvPi * diffuseFactor * (1.0f - mat.disney.metallic));

                    if (mat.disney.sheen > 0.0f) {
                        float Fh = DisneySchlickWeight(VdotH);
                        rt::Vector3f cSheen = (1.0f - mat.disney.sheenTint) * rt::Vector3f(1.0f, 1.0f, 1.0f) + mat.disney.sheenTint * cTint;
                        rt::Vector3f fSheen = mat.disney.sheen * cSheen * Fh * (1.0f - mat.disney.metallic);
                        fDiffuse += fSheen;
                    }
                }

                // 2. Specular (GGX)
                float alpha = fmaxf(0.001f, mat.disney.roughness * mat.disney.roughness);
                float D = rt::GgxD(NdotH, alpha);
                float G = rt::SmithG(NdotO, NdotI, alpha);

                rt::Vector3f cSpec0 = 0.08f * mat.disney.specular * ((1.0f - mat.disney.specularTint) * rt::Vector3f(1.0f, 1.0f, 1.0f) + mat.disney.specularTint * cTint);
                rt::Vector3f F0 = (1.0f - mat.disney.metallic) * cSpec0 + mat.disney.metallic * mat.disney.baseColor;
                rt::Vector3f F = F0 + (rt::Vector3f(1.0f, 1.0f, 1.0f) - F0) * DisneySchlickWeight(VdotH);

                rt::Vector3f fSpec = (D * G * F) / (4.0f * NdotO * NdotI);

                // 3. Clearcoat (GTR1)
                rt::Vector3f fClearcoat(0.0f, 0.0f, 0.0f);
                if (mat.disney.clearcoat > 0.0f) {
                    float alphaG = (1.0f - mat.disney.clearcoatGloss) * 0.1f + mat.disney.clearcoatGloss * 0.001f;
                    float Dc = DisneyGtr1D(NdotH, alphaG);
                    float Gc = DisneyGtr1SmithG(NdotO, NdotI);
                    float Fc = 0.04f + (1.0f - 0.04f) * DisneySchlickWeight(VdotH);
                    float cVal = 0.25f * mat.disney.clearcoat * (Dc * Gc * Fc) / (4.0f * NdotO * NdotI);
                    fClearcoat = rt::Vector3f(cVal, cVal, cVal);
                }

                return fDiffuse + fSpec + fClearcoat;
            }
        }
        return rt::Vector3f(0.0f, 0.0f, 0.0f);
    }

    __host__ __device__ float PdfBsdf(const DeviceMaterial& mat,
                                      const rt::Vector3f& wo,
                                      const rt::Vector3f& wi,
                                      const rt::Vector3f& n);

    __host__ __device__ inline rt::Vector3f SampleBsdf(const DeviceMaterial& mat,
                                                       const rt::Vector3f& wo,
                                                       const rt::Vector3f& n,
                                                       const rt::Point2f& u,
                                                       rt::Vector3f* wi,
                                                       float* pdf) {
        switch (mat.kind) {
            case MaterialKind::Lambertian: {
                rt::ONB onb(n);
                rt::Vector3f localDir = rt::CosineSampleHemisphere(u);
                *wi = Normalize(onb.ToWorld(localDir));
                *pdf = rt::CosineHemispherePdf(localDir.z);
                return EvaluateBsdf(mat, wo, *wi, n);
            }

            case MaterialKind::Metal: {
                *wi = rt::Reflect(wo, n);
                *pdf = 1.0f;
                float cosThetaWi = AbsDot(*wi, n);
                if (cosThetaWi < 1e-6f || Dot(*wi, n) <= 0.0f) {
                    *pdf = 0.0f;
                    return rt::Vector3f(0.0f, 0.0f, 0.0f);
                }
                return mat.metal.albedo / cosThetaWi;
            }

            case MaterialKind::Dielectric: {
                bool entering = Dot(wo, n) > 0.0f;
                float etaI = entering ? 1.0f : mat.dielectric.ior;
                float etaT = entering ? mat.dielectric.ior : 1.0f;
                float R = rt::FrDielectric(AbsDot(wo, n), etaI, etaT);

                if (u.x < R) {
                    *wi = rt::Reflect(wo, n);
                    *pdf = R;
                    float cosThetaWi = AbsDot(*wi, n);
                    if (cosThetaWi < 1e-6f) return rt::Vector3f(0.0f, 0.0f, 0.0f);
                    return rt::Vector3f(R, R, R) / cosThetaWi;
                }

                *pdf = 1.0f - R;

                if (mat.dielectric.dispersion <= 0.0f) {
                    float eta = etaI / etaT;
                    rt::Vector3f nf = entering ? n : -n;
                    if (!rt::Refract(wo, nf, eta, wi)) {
                        *wi = rt::Reflect(wo, n);
                        float cosThetaWi = AbsDot(*wi, n);
                        if (cosThetaWi < 1e-6f) return rt::Vector3f(0.0f, 0.0f, 0.0f);
                        return mat.dielectric.tint / cosThetaWi;
                    }

                    float cosThetaWi = AbsDot(*wi, n);
                    if (cosThetaWi < 1e-6f) return rt::Vector3f(0.0f, 0.0f, 0.0f);
                    rt::Vector3f T = mat.dielectric.tint * (1.0f - R);
                    return T / (eta * eta) / cosThetaWi;
                }

                constexpr float kChannelWavelengthUm[3] = { 0.630f, 0.532f, 0.465f };
                int channel = std::min(2, static_cast<int>(u.y * 3.0f));
                float lambdaUm = kChannelWavelengthUm[channel];
                float iorChannel = rt::CauchyIOR(mat.dielectric.ior, mat.dielectric.dispersion, lambdaUm);
                float etaI_c = entering ? 1.0f : iorChannel;
                float etaT_c = entering ? iorChannel : 1.0f;
                float eta = etaI_c / etaT_c;
                rt::Vector3f nf = entering ? n : -n;

                if (!rt::Refract(wo, nf, eta, wi)) {
                    *wi = rt::Reflect(wo, n);
                    float cosThetaWi = AbsDot(*wi, n);
                    if (cosThetaWi < 1e-6f) return rt::Vector3f(0.0f, 0.0f, 0.0f);
                    rt::Vector3f result(0.0f, 0.0f, 0.0f);
                    result[channel] = 3.0f / cosThetaWi;
                    return result;
                }

                float cosThetaWi = AbsDot(*wi, n);
                if (cosThetaWi < 1e-6f) return rt::Vector3f(0.0f, 0.0f, 0.0f);
                rt::Vector3f result(0.0f, 0.0f, 0.0f);
                result[channel] = mat.dielectric.tint[channel] * (1.0f - R) * 3.0f / (eta * eta) / cosThetaWi;
                return result;
            }

            case MaterialKind::MicrofacetDielectric: {
                rt::ONB onb(n);
                rt::Vector3f localWo = rt::Vector3f(Dot(wo, onb.u), Dot(wo, onb.v), Dot(wo, onb.w));
                rt::Vector3f localWh = rt::SampleGgxVndf(localWo, mat.microfacetDielectric.alpha, u.x, u.y);
                rt::Vector3f wh = onb.ToWorld(localWh);
                *wi = rt::Reflect(wo, wh);

                if (Dot(*wi, n) <= 0.0f) {
                    *pdf = 0.0f;
                    return rt::Vector3f(0.0f, 0.0f, 0.0f);
                }
                float NdotV = AbsDot(wo, n);
                float NdotH = AbsDot(wh, n);
                *pdf = rt::GgxVndfPdf(NdotV, NdotH, mat.microfacetDielectric.alpha);
                return EvaluateBsdf(mat, wo, *wi, n);
            }

            case MaterialKind::MicrofacetConductor: {
                rt::ONB onb(n);
                rt::Vector3f localWo = rt::Vector3f(Dot(wo, onb.u), Dot(wo, onb.v), Dot(wo, onb.w));
                rt::Vector3f localWh = rt::SampleGgxVndf(localWo, mat.microfacetConductor.alpha, u.x, u.y);
                rt::Vector3f wh = onb.ToWorld(localWh);
                *wi = rt::Reflect(wo, wh);

                if (Dot(*wi, n) <= 0.0f) {
                    *pdf = 0.0f;
                    return rt::Vector3f(0.0f, 0.0f, 0.0f);
                }
                float NdotV = AbsDot(wo, n);
                float NdotH = AbsDot(wh, n);
                *pdf = rt::GgxVndfPdf(NdotV, NdotH, mat.microfacetConductor.alpha);
                return EvaluateBsdf(mat, wo, *wi, n);
            }

            case MaterialKind::Disney: {
                float NdotO = Dot(wo, n);
                if (NdotO <= 0.0f) {
                    *pdf = 0.0f;
                    return rt::Vector3f(0.0f, 0.0f, 0.0f);
                }

                DisneyLobeWeights w = ComputeDisneyLobeWeights(mat);
                rt::ONB onb(n);

                if (u.x < w.wDiff) {
                    float remappedUx = u.x / w.wDiff;
                    rt::Point2f uSub(remappedUx, u.y);
                    rt::Vector3f localDir = rt::CosineSampleHemisphere(uSub);
                    *wi = Normalize(onb.ToWorld(localDir));
                } else if (u.x < w.wDiff + w.wSpec) {
                    float remappedUx = (u.x - w.wDiff) / w.wSpec;
                    rt::Point2f uSub(remappedUx, u.y);
                    float alpha = fmaxf(0.001f, mat.disney.roughness * mat.disney.roughness);
                    rt::Vector3f localWh = rt::SampleGgx(uSub, alpha);
                    rt::Vector3f wh = Normalize(onb.ToWorld(localWh));
                    *wi = Normalize(2.0f * Dot(wo, wh) * wh - wo);
                } else {
                    float remappedUx = (u.x - (w.wDiff + w.wSpec)) / w.wClear;
                    rt::Point2f uSub(remappedUx, u.y);
                    float alphaG = (1.0f - mat.disney.clearcoatGloss) * 0.1f + mat.disney.clearcoatGloss * 0.001f;
                    rt::Vector3f localWh = DisneySampleGtr1(uSub, alphaG);
                    rt::Vector3f wh = Normalize(onb.ToWorld(localWh));
                    *wi = Normalize(2.0f * Dot(wo, wh) * wh - wo);
                }

                if (Dot(*wi, n) <= 0.0f) {
                    *pdf = 0.0f;
                    return rt::Vector3f(0.0f, 0.0f, 0.0f);
                }

                *pdf = PdfBsdf(mat, wo, *wi, n);
                if (*pdf <= 0.0f) {
                    return rt::Vector3f(0.0f, 0.0f, 0.0f);
                }

                return EvaluateBsdf(mat, wo, *wi, n);
            }

            case MaterialKind::Emissive: {
                *wi = rt::Vector3f(0.0f, 0.0f, 0.0f);
                *pdf = 0.0f;
                return rt::Vector3f(0.0f, 0.0f, 0.0f);
            }
        }
        *wi = rt::Vector3f(0.0f, 0.0f, 0.0f);
        *pdf = 0.0f;
        return rt::Vector3f(0.0f, 0.0f, 0.0f);
    }

    __host__ __device__ inline float PdfBsdf(const DeviceMaterial& mat,
                                             const rt::Vector3f& wo,
                                             const rt::Vector3f& wi,
                                             const rt::Vector3f& n) {
        switch (mat.kind) {
            case MaterialKind::Lambertian:
                return (Dot(wi, n) > 0.0f && Dot(wo, n) > 0.0f) ? AbsDot(wi, n) * rt::kInvPi : 0.0f;

            case MaterialKind::Metal:
            case MaterialKind::Dielectric:
            case MaterialKind::Emissive:
                return 0.0f;

            case MaterialKind::MicrofacetDielectric: {
                if (Dot(wo, n) * Dot(wi, n) <= 0.0f) return 0.0f;
                rt::Vector3f wh = wo + wi;
                if (LengthSquared(wh) == 0.0f) return 0.0f;
                wh = Normalize(wh);
                return rt::GgxVndfPdf(AbsDot(wo, n), AbsDot(wh, n), mat.microfacetDielectric.alpha);
            }

            case MaterialKind::MicrofacetConductor: {
                if (Dot(wo, n) * Dot(wi, n) <= 0.0f) return 0.0f;
                rt::Vector3f wh = wo + wi;
                if (LengthSquared(wh) == 0.0f) return 0.0f;
                wh = Normalize(wh);
                return rt::GgxVndfPdf(AbsDot(wo, n), AbsDot(wh, n), mat.microfacetConductor.alpha);
            }

            case MaterialKind::Disney: {
                float NdotO = Dot(wo, n);
                float NdotI = Dot(wi, n);
                if (NdotO <= 0.0f || NdotI <= 0.0f) return 0.0f;

                rt::Vector3f wh = wo + wi;
                if (LengthSquared(wh) == 0.0f) return 0.0f;
                wh = Normalize(wh);

                float NdotH = Dot(wh, n);
                float VdotH = Dot(wo, wh);
                if (VdotH <= 0.0f) return 0.0f;

                DisneyLobeWeights w = ComputeDisneyLobeWeights(mat);

                float pdfDiff = rt::CosineHemispherePdf(NdotI);

                float alpha = fmaxf(0.001f, mat.disney.roughness * mat.disney.roughness);
                float D = rt::GgxD(NdotH, alpha);
                float pdfSpec = (D * NdotH) / (4.0f * VdotH);

                float alphaG = (1.0f - mat.disney.clearcoatGloss) * 0.1f + mat.disney.clearcoatGloss * 0.001f;
                float Dc = DisneyGtr1D(NdotH, alphaG);
                float pdfClear = (Dc * NdotH) / (4.0f * VdotH);

                return w.wDiff * pdfDiff + w.wSpec * pdfSpec + w.wClear * pdfClear;
            }
        }
        return 0.0f;
    }

    __host__ __device__ inline rt::Vector3f EvaluateEmission(const DeviceMaterial& mat,
                                                             const rt::Vector3f& wo,
                                                             const rt::Vector3f& n) {
        if (mat.kind == MaterialKind::Emissive && Dot(wo, n) > 0.0f) {
            return mat.emissive.radiance;
        }
        return rt::Vector3f(0.0f, 0.0f, 0.0f);
    }

} // namespace rtx
