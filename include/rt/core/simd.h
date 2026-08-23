#pragma once

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define ALBEDO_HAS_SSE 1
#if defined(__AVX2__) || defined(__AVX__)
#define ALBEDO_HAS_AVX2 1
#else
#define ALBEDO_HAS_AVX2 0
#endif
#else
#define ALBEDO_HAS_SSE 0
#define ALBEDO_HAS_AVX2 0
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace rt {

template <int N>
inline void InitDegenerateBox(float minP[N], float maxP[N], int lane) {
    constexpr float kInf = std::numeric_limits<float>::infinity();
    minP[lane] = kInf;
    maxP[lane] = -kInf;
}

} // namespace rt
