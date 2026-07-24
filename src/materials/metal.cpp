#include "rt/materials/metal.h"
#include "rt/materials/fresnel.h"

namespace rt {

    // Mirror metals have no diffuse scattering
    Vector3f Metal::f(const Vector3f&, const Vector3f&, const Vector3f&) const {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    // Calculates exact specular reflection direction wi = Reflect(wo, n)
    Vector3f Metal::Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f&, Vector3f* wi, float* pdf) const {

        *wi = Reflect(wo, n);
        *pdf = 1.0f; // Discrete direction has discrete probability 1.0

        float cosTheta = AbsDot(*wi, n);
        if (cosTheta < 1e-6f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        // Return albedo divided by cosTheta so that when multiplied by cosTheta in path tracer rendering, we get pure albedo_
        return albedo_ / cosTheta;
    }

    // Continuous PDF is 0 for discrete specular delta directions
    float Metal::Pdf(const Vector3f&, const Vector3f&, const Vector3f&) const {
        return 0.0f;
    }
}