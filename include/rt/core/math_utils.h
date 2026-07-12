#ifndef RT_CORE_MATH_UTILS_H
#define RT_CORE_MATH_UTILS_H

#include <numbers>

namespace rt {

constexpr float Radians(float degrees) {
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

} // namespace rt

#endif // RT_CORE_MATH_UTILS_H
