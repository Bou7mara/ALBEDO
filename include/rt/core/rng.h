#pragma once
#include "rt/core/point2.h"
#include <stdint.h>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    class RNG {
    public:
        constexpr __host__ __device__ RNG() : state_(0x853c49e6748fea9bULL), inc_(0xda3e39cb94b95bdbULL) {}

        constexpr __host__ __device__ explicit RNG(uint64_t seed, uint64_t seq = 1ULL) {
            state_ = 0ULL;
            inc_ = (seq << 1u) | 1u;
            UniformUint32();
            state_ += seed;
            UniformUint32();
        }

        constexpr __host__ __device__ uint32_t UniformUint32() {
            uint64_t oldstate = state_;
            state_ = oldstate * 6364136223846793005ULL + inc_;
            uint32_t xorshifted = static_cast<uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
            uint32_t rot = static_cast<uint32_t>(oldstate >> 59u);
            return (xorshifted >> rot) | (xorshifted << ((~rot + 1u) & 31));
        }

        __host__ __device__ float Uniform1D() {
            return static_cast<float>(UniformUint32() >> 8) * (1.0f / 16777216.0f);
        }

        __host__ __device__ Point2f Uniform2D() {
            float u1 = Uniform1D();
            float u2 = Uniform1D();
            return Point2f(u1, u2);
        }

    private:
        uint64_t state_{0x853c49e6748fea9bULL};
        uint64_t inc_{0xda3e39cb94b95bdbULL};
    };

} // namespace rt
