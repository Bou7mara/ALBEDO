#include "rt/materials/metal.h"
#include "rt/materials/fresnel.h"

namespace rt {

    Vector3f Metal::f(const Vector3f&, const Vector3f&, const Vector3f&, const Point2f&) const {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }

    Vector3f Metal::Sample_f(const Vector3f& wo, const Vector3f& n, const Point2f&, Vector3f* wi, float* pdf, const Point2f& uv) const {

        *wi = Reflect(wo, n);
        *pdf = 1.0f;

        float cosTheta = AbsDot(*wi, n);
        if (cosTheta < 1e-6f) {
            return Vector3f(0.0f, 0.0f, 0.0f);
        }

        Vector3f effectiveAlbedo = albedoTexture_ ? albedoTexture_->Sample(uv.x, uv.y) : albedo_;
        return effectiveAlbedo / cosTheta;
    }

    float Metal::Pdf(const Vector3f&, const Vector3f&, const Vector3f&, const Point2f&) const {
        return 0.0f;
    }
}