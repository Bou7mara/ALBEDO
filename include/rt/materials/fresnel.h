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

    inline float FrConductor(float cosThetaI, float eta, float k) {
        float cosThetaI2 = cosThetaI * cosThetaI;
        float sinThetaI2 = std::max(0.0f, 1.0f - cosThetaI2);
        float eta2 = eta * eta;
        float k2 = k * k;

        float t0 = eta2 - k2 - sinThetaI2;
        float a2plusb2 = std::sqrt(std::max(0.0f, t0 * t0 + 4.0f * eta2 * k2));
        float t1 = a2plusb2 + cosThetaI2;
        float a = std::sqrt(std::max(0.0f, 0.5f * (a2plusb2 + t0)));
        float t2 = 2.0f * a * cosThetaI;
        float Rperp2 = (t1 - t2) / (t1 + t2);

        float t3 = cosThetaI2 * a2plusb2 + sinThetaI2 * sinThetaI2;
        float t4 = t2 * sinThetaI2;
        float Rparl2 = Rperp2 * (t3 - t4) / (t3 + t4);

        return 0.5f * (Rparl2 + Rperp2);
    }
}