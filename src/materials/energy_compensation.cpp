#include "rt/materials/energy_compensation.h"
#include "rt/materials/microfacet.h"
#include "rt/core/rng.h"
#include <algorithm>
#include <cmath>

namespace rt {

    DirectionalAlbedoLUT::DirectionalAlbedoLUT()
        : eTable(kResolution, kResolution, WrapMode::Clamp),
          eAvgTable(kResolution, 1, WrapMode::Clamp) {
        constexpr int N = 2048;

        for (int y = 0; y < kResolution; ++y) {
            float alpha = (static_cast<float>(y) + 0.5f) / static_cast<float>(kResolution);
            alpha = std::max(alpha, 1e-3f);

            for (int x = 0; x < kResolution; ++x) {
                float mu = (static_cast<float>(x) + 0.5f) / static_cast<float>(kResolution);
                mu = std::clamp(mu, 1e-4f, 1.0f);

                Vector3f wo = Normalize(Vector3f(std::sqrt(std::max(0.0f, 1.0f - mu * mu)), 0.0f, mu));

                float sum = 0.0f;
                RNG rng(12345 + y * kResolution + x);

                float NdotV = mu;
                float alpha2 = alpha * alpha;
                float lambdaV = std::sqrt(std::max(0.0f, NdotV * NdotV * (1.0f - alpha2) + alpha2));
                float G1 = (2.0f * NdotV) / (NdotV + lambdaV);

                for (int k = 0; k < N; ++k) {
                    Point2f u = rng.Uniform2D();
                    Vector3f h = SampleGgxVndf(wo, alpha, u.x, u.y);
                    Vector3f wi = Reflect(wo, h);

                    if (wi.z > 0.0f) {
                        float NdotL = wi.z;
                        float G2 = 4.0f * NdotV * NdotL * SmithG(NdotV, NdotL, alpha);
                        sum += G2 / G1;
                    }
                }

                float E = std::clamp(sum / static_cast<float>(N), 0.0f, 1.0f);
                eTable.Set(x, y, E);
            }

            float sumEAvg = 0.0f;
            for (int x = 0; x < kResolution; ++x) {
                float mu = (static_cast<float>(x) + 0.5f) / static_cast<float>(kResolution);
                float E = eTable.Get(x, y);
                sumEAvg += 2.0f * mu * E * (1.0f / static_cast<float>(kResolution));
            }
            eAvgTable.Set(y, 0, std::clamp(sumEAvg, 0.0f, 1.0f));
        }
    }

    const DirectionalAlbedoLUT& GetDirectionalAlbedoLUT() {
        static const DirectionalAlbedoLUT lut;
        return lut;
    }

}
