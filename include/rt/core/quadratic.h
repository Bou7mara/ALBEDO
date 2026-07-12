#ifndef RT_CORE_QUADRATIC_H
#define RT_CORE_QUADRATIC_H

#include <cmath>
#include <utility>

namespace rt {

// Robust quadratic solver: avoids catastrophic cancellation by always
// computing q = -0.5*(b +/- sqrt(disc)) with the sign chosen to match
// b, then deriving both roots from q rather than the naive formula
// applied twice. Computes the discriminant in double precision even
// when inputs are float, since a*c and b*b can be nearly equal for
// near-tangent rays and the subtraction needs the extra bits.
inline bool Quadratic(float a, float b, float c, float* t0, float* t1) {
    double discrim = double(b) * double(b) - 4.0 * double(a) * double(c);
    if (discrim < 0.0) return false;
    double rootDiscrim = std::sqrt(discrim);

    double q = (b < 0) ? -0.5 * (b - rootDiscrim) : -0.5 * (b + rootDiscrim);
    double r0 = q / a;
    double r1 = c / q;
    if (r0 > r1) std::swap(r0, r1);

    *t0 = static_cast<float>(r0);
    *t1 = static_cast<float>(r1);
    return true;
}

} // namespace rt

#endif // RT_CORE_QUADRATIC_H
