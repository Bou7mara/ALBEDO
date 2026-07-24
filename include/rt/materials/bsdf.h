#pragma once
#include "rt/core/vector3.h"
#include "rt/core/point2.h"

namespace rt {
 
    // Bidirectional Scattering Distribution Function (BSDF) base class
    // Defines how light scatters when striking a material surface (reflection, refraction, emission)
    class BSDF {
    public:
        virtual ~BSDF() = default;

        // Evaluates material reflection function f(wo, wi) for outgoing direction wo and incoming direction wi
        virtual Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const = 0;

        // Samples a scattered light direction wi given outgoing direction wo, surface normal n, and 2D random sample u
        // Returns the scattered color/attenuation value, and updates wi and pdf pointers
        virtual Vector3f Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const = 0;

        // Calculates the probability density function (PDF) for sampling direction wi given outgoing direction wo
        virtual float Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const = 0;

        // Returns emitted radiance Le if the material is a light source (defaults to black for non-emitting materials)
        virtual Vector3f Le(const Vector3f&, const Vector3f&) const {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }
    };
}

