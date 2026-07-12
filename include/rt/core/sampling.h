#ifndef RT_CORE_SAMPLING_H
#define RT_CORE_SAMPLING_H

#include "rt/core/point2.h"
#include "rt/core/vector3.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace rt {

inline Point2f ConcentricSampleDisk(const Point2f& u) {
    Point2f uOffset(2.0f * u.x - 1.0f, 2.0f * u.y - 1.0f);
    if (uOffset.x == 0.0f && uOffset.y == 0.0f) return Point2f(0.0f, 0.0f);

    float theta, r;
    constexpr float kPiOver4 = std::numbers::pi_v<float> / 4.0f;
    constexpr float kPiOver2 = std::numbers::pi_v<float> / 2.0f;
    if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
        r = uOffset.x;
        theta = kPiOver4 * (uOffset.y / uOffset.x);
    } else {
        r = uOffset.y;
        theta = kPiOver2 - kPiOver4 * (uOffset.x / uOffset.y);
    }
    return r * Point2f(std::cos(theta), std::sin(theta));
}

// Returns a unit direction in a LOCAL frame where +z is "up" (i.e.
// aligned with whatever normal the caller's ONB was built from).
inline Vector3f CosineSampleHemisphere(const Point2f& u) {
    Point2f d = ConcentricSampleDisk(u);
    float z = std::sqrt(std::max(0.0f, 1.0f - d.x * d.x - d.y * d.y));
    return Vector3f(d.x, d.y, z);
}

// cosTheta measured against the same local +z axis CosineSampleHemisphere
// samples around.
inline float CosineHemispherePdf(float cosTheta) {
    return cosTheta * std::numbers::inv_pi_v<float>;
}

} // namespace rt

#endif // RT_CORE_SAMPLING_H
