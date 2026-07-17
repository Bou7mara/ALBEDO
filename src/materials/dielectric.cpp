#include "rt/materials/dielectric.h"
#include "rt/materials/fresnel.h"

namespace rt {

Vector3f Dielectric::f(const Vector3f&, const Vector3f&, const Vector3f&) const {
    return Vector3f(0.0f, 0.0f, 0.0f);
}

Vector3f Dielectric::Sample_f(const Vector3f& wo, const Vector3f& n,
                              const Point2f& u, Vector3f* wi, float* pdf) const {

    bool entering = Dot(wo, n) > 0.0f;
    float etaI = entering ? 1.0f : ior_;
    float etaT = entering ? ior_ : 1.0f;
    float cosThetaI = AbsDot(wo, n);
    float R = FrDielectric(cosThetaI, etaI, etaT);
    if (u.x < R) {

        *wi = Reflect(wo, n);
        *pdf = R;

        float cosThetaWi = AbsDot(*wi, n);
        if (cosThetaWi < 1e-6f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        return Vector3f(R, R, R) / cosThetaWi;
    } else {

        Vector3f nf = entering ? n : -n;
        float eta = etaI / etaT;

        Refract(wo, nf, eta, wi);
        *pdf = 1.0f - R;

        float cosThetaWi = AbsDot(*wi, n);
        if (cosThetaWi < 1e-6f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        return Vector3f(1.0f - R, 1.0f - R, 1.0f - R) / (eta * eta) / cosThetaWi;
    }
}
}