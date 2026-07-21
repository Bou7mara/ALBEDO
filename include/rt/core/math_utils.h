#pragma once
#include <numbers>

// Ray Tracer Core Namespace
namespace rt {
    // ============================================
    // MATHEMATICAL UTILITIES & ANGLE CONVERSIONS
    // ============================================

    // Converts angles from degrees to radians.
    // Humans think in degrees (0..360), standard C++ math functions think in radians (0..2pi).
    // Uses C++20 <numbers> header for max precision floating-point pi constant!
    constexpr float Radians(float degrees) {
        return degrees * std::numbers::pi_v<float> / 180.0f;
    }
}
