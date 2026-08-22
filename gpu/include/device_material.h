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
        Emissive = 5
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
    };

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
        }
        return rt::Vector3f(0.0f, 0.0f, 0.0f);
    }

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
                float cosThetaI = AbsDot(wo, n);
                float R = rt::FrDielectric(cosThetaI, etaI, etaT);

                if (u.x < R) {
                    *wi = rt::Reflect(wo, n);
                    *pdf = R;
                    float cosThetaWi = AbsDot(*wi, n);
                    if (cosThetaWi < 1e-6f) return rt::Vector3f(0.0f, 0.0f, 0.0f);
                    return rt::Vector3f(R, R, R) / cosThetaWi;
                }

                *pdf = 1.0f - R;
                if (mat.dielectric.dispersion <= 0.0f) {
                    rt::Vector3f nf = entering ? n : -n;
                    float eta = etaI / etaT;
                    rt::Refract(wo, nf, eta, wi);
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
