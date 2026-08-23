#pragma once
#include <cmath>
#include <algorithm>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    struct SellmeierCoefficients {
        float B[3];
        float C[3]; // in um^2
        bool hasDispersion;

        static constexpr SellmeierCoefficients MakeConstant(float ior) {
            return SellmeierCoefficients{
                { ior, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f },
                false
            };
        }

        static constexpr SellmeierCoefficients MakeCauchy(float iorAtRef, float dispersionB) {
            return SellmeierCoefficients{
                { iorAtRef, dispersionB, 0.0f },
                { 0.0f, 0.0f, 0.0f },
                (dispersionB > 0.0f)
            };
        }
    };

    // Published Diamond Sellmeier coefficients (Peter 1923 / Edwards & Ochoa 1981)
    // n^2 = 1 + 0.3306 * lambda^2 / (lambda^2 - 0.175^2) + 4.3356 * lambda^2 / (lambda^2 - 0.106^2)
    // Yields n ~ 2.4173 at 589.3 nm (Sodium D-line).
    inline constexpr SellmeierCoefficients kDiamondSellmeier{
        { 0.3306f, 4.3356f, 0.0f },
        { 0.030625f, 0.011236f, 0.0f },
        true
    };

    // Published Schott BK7 Optical Glass Sellmeier coefficients
    // Yields n ~ 1.5168 at 589.3 nm.
    inline constexpr SellmeierCoefficients kBK7Sellmeier{
        { 1.03961212f, 0.231792344f, 1.01046945f },
        { 0.00600069867f, 0.0200179144f, 103.560653f },
        true
    };

    // Published Sapphire (Al2O3 ordinary ray) Sellmeier coefficients
    // Yields n ~ 1.768 at 589.3 nm.
    inline constexpr SellmeierCoefficients kSapphireSellmeier{
        { 1.43134930f, 0.65054713f, 5.3414021f },
        { 0.0052799261f, 0.0142382647f, 325.017834f },
        true
    };

    // Published Fused Silica (SiO2) Sellmeier coefficients
    // Yields n ~ 1.4585 at 589.3 nm.
    inline constexpr SellmeierCoefficients kFusedSilicaSellmeier{
        { 0.6961663f, 0.4079426f, 0.8974794f },
        { 0.004679148f, 0.01351206f, 97.934002f },
        true
    };

    __host__ __device__ inline float SellmeierIOR(const SellmeierCoefficients& s, float lambdaUm) {
        if (!s.hasDispersion) {
            return s.B[0];
        }
        // If C coefficients are non-zero, evaluate 3-term Sellmeier equation
        if (s.C[0] != 0.0f || s.C[1] != 0.0f || s.C[2] != 0.0f) {
            float lambda2 = lambdaUm * lambdaUm;
            float n2 = 1.0f;
            for (int i = 0; i < 3; ++i) {
                if (s.B[i] != 0.0f) {
                    float denom = lambda2 - s.C[i];
                    if (std::abs(denom) > 1e-6f) {
                        n2 += (s.B[i] * lambda2) / denom;
                    }
                }
            }
            return std::sqrt(std::max(1.0f, n2));
        }
        // Otherwise, evaluate Cauchy parameterized form
        constexpr float kRefLambdaUm = 0.5893f;
        float A = s.B[0] - s.B[1] / (kRefLambdaUm * kRefLambdaUm);
        return A + s.B[1] / (lambdaUm * lambdaUm);
    }

} // namespace rt
