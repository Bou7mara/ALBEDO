#include "rt/materials/lambertian.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"
#include <numbers>

namespace rt {

Vector3f Lambertian::f(const Vector3f& /*wo*/, const Vector3f& /*wi*/,
                       const Vector3f& /*n*/) const {
    return albedo_ * std::numbers::inv_pi_v<float>;
}

Vector3f Lambertian::Sample_f(const Vector3f& /*wo*/, const Vector3f& n,
                               const Point2f& u, Vector3f* wi,
                               float* pdf) const {
    ONB onb(n);
    Vector3f localDir = CosineSampleHemisphere(u);
    *wi = Normalize(onb.ToWorld(localDir));
    *pdf = CosineHemispherePdf(localDir.z);
    return f(Vector3f(), *wi, n);
}
}
