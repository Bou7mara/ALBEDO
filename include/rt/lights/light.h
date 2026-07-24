#pragma once
#include "rt/core/vector3.h"
#include "rt/core/point3.h"
#include "rt/core/point2.h"

namespace rt {
    // Abstract base class representing light sources in the scene
    class Light {
    public:
        virtual ~Light() = default;

        // Stores light sampling results returned by Sample_Li
        struct LiSample {
            Vector3f Li;  // Incoming radiance (color and brightness) reaching the surface point
            Vector3f wi;  // Unit vector direction pointing from surface point towards the light
            float pdf;    // Probability density of sampling this direction wi
            float dist;   // Distance from surface point to light sample location
        };

        // Samples incoming light at surface reference point ref using 2D random sample u
        virtual LiSample Sample_Li(const Point3f& ref, const Point2f& u) const = 0;

        // Gets probability density function (PDF) for sampling direction wi from surface point ref
        virtual float Pdf_Li(const Point3f& ref, const Vector3f& wi) const = 0;

        // Calculates total emitted power of light source
        virtual float Power() const = 0;
    };
}

