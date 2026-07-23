#pragma once
#include "rt/core/point2.h"
#include <random>

// My Ray Tracer's Core Namespace
namespace rt {

    // ==========================================
    // PSEUDO-RANDOM NUMBER GENERATOR CLASS (RNG)
    // ==========================================

    // Generates uniformly distributed random floating-point numbers in range [0.0, 1.0).
    // Encapsulates Mersenne Twister engine (std::mt19937) for Monte Carlo path tracing sample generation.
    class RNG {
    public:
        // ------------
        // CONSTRUCTORS
        // ------------

        // Default constructor: Seeds using non-deterministic hardware entropy (std::random_device)
        RNG() : engine_(std::random_device{}()), dist_(0.0f, 1.0f) {}

        // Parameterized constructor: Explicit seed for deterministic, repeatable rendering runs!
        explicit RNG(uint32_t seed) : engine_(seed), dist_(0.0f, 1.0f) {}

        // -------------------------
        // RANDOM SAMPLING INTERFACE
        // -------------------------

        // Returns uniform 1D scalar sample in range [0.0, 1.0)
        float Uniform1D() { return dist_(engine_); }

        // Returns uniform 2D point sample (u1, u2) in square [0.0, 1.0)^2
        Point2f Uniform2D() { return Point2f(dist_(engine_), dist_(engine_)); }

    private:
        // --- Private Engine Members ---
        std::mt19937 engine_;                         // Mersenne Twister 19937 PRNG engine
        std::uniform_real_distribution<float> dist_; // Uniform float distribution [0.0, 1.0)
    };
}
