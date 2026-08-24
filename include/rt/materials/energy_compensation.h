#pragma once
#include "rt/core/vector3.h"
#include "rt/textures/image2d.h"
#include "rt/materials/fresnel.h"
#include <vector>

namespace rt {

    struct DirectionalAlbedoLUT {
        static constexpr int kResolution = 64;
        Image2D<float> eTable;
        Image2D<float> eAvgTable;

        DirectionalAlbedoLUT();

        float SampleE(float cosTheta, float alpha) const {
            return eTable.Sample(std::clamp(cosTheta, 0.0f, 1.0f), std::clamp(alpha, 0.0f, 1.0f));
        }

        float SampleEAvg(float alpha) const {
            return eAvgTable.Sample(std::clamp(alpha, 0.0f, 1.0f), 0.5f);
        }
    };

    const DirectionalAlbedoLUT& GetDirectionalAlbedoLUT();

    __host__ __device__ inline Vector3f AverageFresnelConductor(const Vector3f& eta, const Vector3f& k) {
        constexpr int kSteps = 32;
        Vector3f sum(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < kSteps; ++i) {
            float mu = (static_cast<float>(i) + 0.5f) / static_cast<float>(kSteps);
            float fr = FrConductor(mu, eta.x, k.x);
            float fg = FrConductor(mu, eta.y, k.y);
            float fb = FrConductor(mu, eta.z, k.z);
            sum += 2.0f * mu * Vector3f(fr, fg, fb) * (1.0f / static_cast<float>(kSteps));
        }
        return sum;
    }

}
