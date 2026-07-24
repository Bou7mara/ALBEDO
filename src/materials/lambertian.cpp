#include "rt/materials/lambertian.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"
#include <numbers>

namespace rt {

// Calculates Lambertian BRDF value f = albedo / pi
Vector3f Lambertian::f(const Vector3f&, const Vector3f&, const Vector3f&) const {
    return albedo_ * std::numbers::inv_pi_v<float>;
}

// Samples a cosine-weighted direction on the hemisphere around normal n
Vector3f Lambertian::Sample_f(const Vector3f&, const Vector3f& n, const Point2f& u, Vector3f* wi, float* pdf) const {
    // Construct local shading frame aligned with surface normal n
    ONB onb(n);
    // Sample direction in local hemisphere space (where Z axis is normal)
    Vector3f localDir = CosineSampleHemisphere(u);
    // Transform sample direction to world space
    *wi = Normalize(onb.ToWorld(localDir));
    // Calculate PDF for cosine-weighted sampling
    *pdf = CosineHemispherePdf(localDir.z);
    return f(Vector3f(), *wi, n);
}

// Calculates PDF for sampling direction wi
float Lambertian::Pdf(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const {
    // Return 0 if directions are on opposite sides of the surface
    if (Dot(wo, n) * Dot(wi, n) <= 0.0f) {
        return 0.0f;
    }
    return CosineHemispherePdf(AbsDot(wi, n));
}
}

