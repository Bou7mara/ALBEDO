#include "rt/materials/lambertian.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"
#include <numbers>

namespace rt {

Vector3f Lambertian::f(const Vector3f& /*wo*/, const Vector3f& /*wi*/,
                       const Vector3f& /*n*/) const {
    // Ideal diffuse: constant over the hemisphere, direction-independent.
    // wo/wi stay in the signature (unused here) to match the general BSDF
    // interface -- the day a non-Lambertian BSDF lands, this parameter
    // list is already correct.
    return albedo_ * std::numbers::inv_pi_v<float>;
}

Vector3f Lambertian::Sample_f(const Vector3f& /*wo*/, const Vector3f& n,
                               const Point2f& u, Vector3f* wi,
                               float* pdf) const {
    ONB onb(n);
    Vector3f localDir = CosineSampleHemisphere(u);
    *wi = Normalize(onb.ToWorld(localDir));

    // cosTheta is read directly off the PRE-transform local sample
    // (localDir.z), not recomputed via Dot(*wi, n) after transforming.
    // These are algebraically identical -- ONB::ToWorld is an
    // orthonormal linear map, and it preserves the component along w
    // (== n) exactly -- but reading localDir.z skips a redundant dot
    // product on every single sample.
    *pdf = CosineHemispherePdf(localDir.z);

    return f(Vector3f(), *wi, n);
}

} // namespace rt
