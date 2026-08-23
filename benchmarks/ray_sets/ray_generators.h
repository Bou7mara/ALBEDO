#pragma once
#include "rt/accel/blas.h"
#include "rt/cam/perspective_camera.h"
#include "rt/core/onb.h"
#include "rt/core/ray.h"
#include "rt/core/sampling.h"
#include <random>
#include <vector>

namespace rt::bench {

enum class RaySetType {
    Primary,
    SecondaryIncoherent
};

inline const char* RaySetTypeName(RaySetType type) {
    switch (type) {
        case RaySetType::Primary: return "Primary (Coherent)";
        case RaySetType::SecondaryIncoherent: return "Secondary (Incoherent)";
    }
    return "Unknown";
}

inline std::vector<Ray> GeneratePrimaryRays(int width, int height, const PerspectiveCamera& camera) {
    std::vector<Ray> rays;
    rays.reserve(width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            rays.push_back(camera.GenerateRay(CameraSample{Point2f(x + 0.5f, y + 0.5f)}));
        }
    }
    return rays;
}

inline std::vector<Ray> GenerateSecondaryRays(const std::vector<Ray>& primaryRays,
                                              const BLAS& targetAS,
                                              int raysPerHit = 2,
                                              uint32_t seed = 777) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> uDist(0.0f, 1.0f);

    std::vector<Ray> secondaryRays;
    secondaryRays.reserve(primaryRays.size() * raysPerHit);

    for (const auto& primRay : primaryRays) {
        Ray r = primRay;
        SurfaceInteraction isect;
        if (targetAS.Intersect(r, &isect)) {
            ONB onb(Vector3f(isect.n));
            for (int i = 0; i < raysPerHit; ++i) {
                Point2f u(uDist(rng), uDist(rng));
                Vector3f localDir = CosineSampleHemisphere(u);
                Vector3f worldDir = Normalize(onb.ToWorld(localDir));
                Point3f origin = isect.p + Vector3f(isect.n) * 1e-3f;
                secondaryRays.push_back(Ray(origin, worldDir));
            }
        }
    }

    return secondaryRays;
}

} // namespace rt::bench
