#pragma once
#include "rt/core/vector3.h"
#include <cmath>
#include <algorithm>
#include <utility>

namespace rt {
    inline Vector3f Reflect(const Vector3f& wo, const Vector3f& n) {
        return Normalize(2.0f * Dot(wo, n) * n - wo);
    }

    inline bool Refract(const Vector3f& wi, const Vector3f& n, float eta, Vector3f* wt) {
        float cosThetaI = Dot(n, wi);
        float sin2ThetaI = std::max(0.0f, 1.0f - (cosThetaI * cosThetaI));
        float sin2ThetaT = (eta * eta) * sin2ThetaI;
        if (sin2ThetaT >= 1.0f) {
            return false;
        }
        float cosThetaT = std::sqrt(1.0f - sin2ThetaT);
        *wt = eta * (-wi) + (eta * cosThetaI - cosThetaT) * n;
        return true;
    }

    inline float FrDielectric(float cosThetaI, float etaI, float etaT) {
        cosThetaI = std::clamp(cosThetaI, -1.0f, 1.0f);
        if (cosThetaI < 0.0f) {
            std::swap(etaI, etaT);
            cosThetaI = -cosThetaI;
        }
        float sinThetaI = std::sqrt(std::max(0.0f, 1.0f - cosThetaI * cosThetaI));
        float sinThetaT = (etaI / etaT) * sinThetaI;
        if (sinThetaT >= 1.0f) {
            return 1.0f;
        }
        float cosThetaT = std::sqrt(std::max(0.0f, 1.0f - sinThetaT * sinThetaT));
        float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) / ((etaT * cosThetaI) + (etaI * cosThetaT));
        float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) / ((etaI * cosThetaI) + (etaT * cosThetaT));
        return (Rparl * Rparl + Rperp * Rperp) / 2.0f;
    }
}