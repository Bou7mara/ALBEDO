#pragma once
#include <numbers>

namespace rt {

constexpr float Radians(float degrees) {
    return degrees * std::numbers::pi_v<float> / 180.0f;
}
}
