#pragma once
#include "rt/core/point2.h"
#include <random>

namespace rt {

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
