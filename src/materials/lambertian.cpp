#include "rt/materials/lambertian.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"
import std;

namespace rt {

Vector3f Lambertian::f(const Vector3f&, const Vector3f&, const Vector3f&) const {
    return albedo_ * std::numbers::inv_pi_v<float>;
}

Vector3f Lambertian::Sample_f(const Vector3f&, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const {
    ONB onb(n);
    Vector3f localDir = CosineSampleHemisphere(u);
    *wi = Normalize(onb.ToWorld(localDir));
    *pdf = CosineHemispherePdf(localDir.z);
    return f(Vector3f(), *wi, n);
}

float Lambertian::Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const {
    if (Dot(wo, n) * Dot(wi, n) <= 0.0f) {
        return 0.0f;
    }
    return CosineHemispherePdf(AbsDot(wi, n));
}
}
