#include "rt/lights/diffuse_area_light.h"

namespace rt {
    // Samples a point on the area light surface from reference point ref using 2D random sample u
    Light::LiSample DiffuseAreaLight::Sample_Li(const Point3f& ref, const Point2f& u) const {
        // Sample point and normal on shape surface
        ShapeSample shapeSample = shape_->Sample(ref, u);
        LiSample sample;
        if (shapeSample.pdf == 0.0f) {
            sample.pdf = 0.0f;
            return sample;
        }

        // Calculate vector direction and distance squared from reference point to light sample point
        Vector3f wi = shapeSample.p - ref;
        float distSq = LengthSquared(wi);
        if (distSq == 0.0f) {
            sample.pdf = 0.0f;
            return sample;
        }

        // Calculate distance and unit direction vector
        sample.dist = std::sqrt(distSq);
        sample.wi = wi / sample.dist;
        sample.pdf = shapeSample.pdf;
        
        // Query light emission Le from shape material (BSDF)
        const BSDF* bsdf = shape_->GetBSDF();
        if (bsdf) {
            sample.Li = bsdf->Le(-sample.wi, Vector3f(shapeSample.n));
        } else {
            sample.Li = Vector3f(0.0f, 0.0f, 0.0f);
        }

        return sample;
    }

    // Calculates probability density function (PDF) for sampling direction wi towards light shape
    float DiffuseAreaLight::Pdf_Li(const Point3f& ref, const Vector3f& wi) const {
        return shape_->Pdf(ref, wi);
    }

    // Calculates total power emitted by area light across its surface area
    float DiffuseAreaLight::Power() const {
        const BSDF* bsdf = shape_->GetBSDF();
        if (!bsdf) return 0.0f;
        // Evaluate luminance using standard ITU-R BT.709 coefficients (0.2126 R + 0.7152 G + 0.0722 B)
        Vector3f le = bsdf->Le(Vector3f(0, 0, 1), Vector3f(0, 0, 1));
        float lum = 0.2126f * le.x + 0.7152f * le.y + 0.0722f * le.z;
        return lum * shape_->Area();
    }
}

