#include "rt/lights/diffuse_area_light.h"

namespace rt {
    Light::LiSample DiffuseAreaLight::Sample_Li(const Point3f& ref, const Point2f& u) const {
        ShapeSample shapeSample = shape_->Sample(ref, u);
        LiSample sample;
        if (shapeSample.pdf == 0.0f) {
            sample.pdf = 0.0f;
            return sample;
        }

        Vector3f wi = shapeSample.p - ref;
        float distSq = LengthSquared(wi);
        if (distSq == 0.0f) {
            sample.pdf = 0.0f;
            return sample;
        }

        sample.dist = std::sqrt(distSq);
        sample.wi = wi / sample.dist;
        sample.pdf = shapeSample.pdf;

        const BSDF* bsdf = shape_->GetBSDF();
        if (bsdf) {
            sample.Li = bsdf->Le(-sample.wi, Vector3f(shapeSample.n));
        } else {
            sample.Li = Vector3f(0.0f, 0.0f, 0.0f);
        }

        return sample;
    }

    float DiffuseAreaLight::Pdf_Li(const Point3f& ref, const Vector3f& wi) const {
        return shape_->Pdf(ref, wi);
    }

    float DiffuseAreaLight::Power() const {
        const BSDF* bsdf = shape_->GetBSDF();
        if (!bsdf) return 0.0f;
        Vector3f le = bsdf->Le(Vector3f(0, 0, 1), Vector3f(0, 0, 1));
        float lum = 0.2126f * le.x + 0.7152f * le.y + 0.0722f * le.z;
        return lum * shape_->Area();
    }
}
