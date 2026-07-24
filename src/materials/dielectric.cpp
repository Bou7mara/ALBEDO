#include "rt/materials/dielectric.h"
#include "rt/materials/fresnel.h"

namespace rt {

// Smooth glass has no general diffuse scattering
Vector3f Dielectric::f(const Vector3f&, const Vector3f&, const Vector3f&) const {
    return Vector3f(0.0f, 0.0f, 0.0f);
}

// Samples reflection or refraction depending on Fresnel reflectance R
Vector3f Dielectric::Sample_f(const Vector3f& wo, const Vector3f& n,
                              const Point2f& u, Vector3f* wi, float* pdf) const {

    // Check if ray is entering or exiting the glass volume
    bool entering = Dot(wo, n) > 0.0f;
    float etaI = entering ? 1.0f : ior_;
    float etaT = entering ? ior_ : 1.0f;
    float cosThetaI = AbsDot(wo, n);
    
    // Calculate Fresnel reflectance fraction (0.0 to 1.0)
    float R = FrDielectric(cosThetaI, etaI, etaT);
    
    // Choose specular reflection if random sample u.x is less than Fresnel reflectance R
    if (u.x < R) {

        *wi = Reflect(wo, n);
        *pdf = R;

        float cosThetaWi = AbsDot(*wi, n);
        if (cosThetaWi < 1e-6f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        return Vector3f(R, R, R) / cosThetaWi;
    } else {

        // Otherwise choose specular refraction (transmission) through the glass
        Vector3f nf = entering ? n : -n;
        float eta = etaI / etaT;

        Refract(wo, nf, eta, wi);
        *pdf = 1.0f - R;

        float cosThetaWi = AbsDot(*wi, n);
        if (cosThetaWi < 1e-6f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        // Apply non-symmetric radiance scaling factor (1 / eta^2) for refraction into media with different IOR
        return Vector3f(1.0f - R, 1.0f - R, 1.0f - R) / (eta * eta) / cosThetaWi;
    }
}

// Continuous PDF is 0 for discrete delta specular directions
float Dielectric::Pdf(const Vector3f&, const Vector3f&, const Vector3f&) const {
    return 0.0f;
}
}