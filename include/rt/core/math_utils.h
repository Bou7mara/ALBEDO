#pragma once
#include <numbers>

// My Ray Tracer's Core Namespace
namespace rt {

    // ==================================
    // MATH UTILITIES & ANGLE CONVERSIONS
    // ==================================

    // comments to pretend 4 lines of code deserve their own header file
    // convert angles from degrees to radians
    // C++20 <numbers> header allows for the precisest float pi
    constexpr float Radians(float degrees) {
        return degrees * std::numbers::pi_v<float> / 180.0f;
    }
}
