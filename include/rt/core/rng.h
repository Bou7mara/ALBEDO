#pragma once
#include "rt/core/point2.h"
#include <random>

namespace rt {

// Minimal uniform random source -- just enough to drive Monte Carlo
// estimation (BSDF sampling, pixel jitter) for this step. This is
// NOT the Sampler abstraction from TOC 8 (stratification,
// low-discrepancy sequences, per-pixel sample streams) -- that's real
// future work once convergence quality actually demands it. This is
// std::mt19937 wrapped just enough to stop every call site from
// hand-rolling <random> boilerplate.
class RNG {
public:
    RNG() : engine_(std::random_device{}()), dist_(0.0f, 1.0f) {}
    explicit RNG(uint32_t seed) : engine_(seed), dist_(0.0f, 1.0f) {}

    float Uniform1D() { return dist_(engine_); }
    Point2f Uniform2D() { return Point2f(dist_(engine_), dist_(engine_)); }

private:
    std::mt19937 engine_;
    std::uniform_real_distribution<float> dist_;
};

}
