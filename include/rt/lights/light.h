#pragma once
#include "rt/core/vector3.h"
#include "rt/core/point3.h"
#include "rt/core/point2.h"

namespace rt {
    class Light {
    public:
        virtual ~Light() = default;

        struct LiSample {
            Vector3f Li;
            Vector3f wi;
            float pdf;
            float dist;
        };

        virtual LiSample Sample_Li(const Point3f& ref, const Point2f& u) const = 0;
        virtual float Pdf_Li(const Point3f& ref, const Vector3f& wi) const = 0;
        virtual float Power() const = 0;
    };
}
