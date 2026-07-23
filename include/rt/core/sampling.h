#pragma once
#include "rt/core/point2.h"
#include "rt/core/vector3.h"
#include <algorithm>
#include <cmath>
#include <numbers>

// My Ray Tracer's Core Namespace
namespace rt {

    // ================================================
    // MONTE CARLO SAMPLING & PROBABILITY DENSITY (PDF)
    // ================================================

    // Shirley's Concentric Disk Sampling algorithm.
    // Maps uniform square samples u in [0,1]^2 to the unit disk [-1,1]^2.
    // Obscure Choice Rationale: Standard polar mapping (r = sqrt(u1), theta = 2*pi*u2) heavily distorts
    // fractional area near the center! Shirley's concentric mapping maps concentric squares to concentric rings,
    // preserving fractional area and grid adjacency for low-discrepancy patterns.
    inline Point2f ConcentricSampleDisk(const Point2f& u) {
        // Map uniform sample from [0, 1] to [-1, 1]
        Point2f uOffset(2.0f * u.x - 1.0f, 2.0f * u.y - 1.0f);

        // Origin edge case check
        if (uOffset.x == 0.0f && uOffset.y == 0.0f) return Point2f(0.0f, 0.0f);

        float theta, r;
        constexpr float kPiOver4 = std::numbers::pi_v<float> / 4.0f;
        constexpr float kPiOver2 = std::numbers::pi_v<float> / 2.0f;

        // Determine slice wedge quadrant to map square to circle
        if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
            r = uOffset.x;
            theta = kPiOver4 * (uOffset.y / uOffset.x);
        } else {
            r = uOffset.y;
            theta = kPiOver2 - kPiOver4 * (uOffset.x / uOffset.y);
        }
        return r * Point2f(std::cos(theta), std::sin(theta));
    }

    // Cosine-weighted hemisphere sampling via Malley's Method.
    // Generates directions on unit hemisphere weighted by cos(theta) (ideal for Lambertian diffuse reflection!).
    // Projects 2D concentric disk samples straight up onto the 3D unit hemisphere.
    inline Vector3f CosineSampleHemisphere(const Point2f& u) {
        Point2f d = ConcentricSampleDisk(u);
        // z = sqrt(1 - x^2 - y^2), clamped to >= 0 to avoid NaN from floating-point precision loss
        float z = std::sqrt(std::max(0.0f, 1.0f - d.x * d.x - d.y * d.y));
        return Vector3f(d.x, d.y, z);
    }

    // Probability Density Function (PDF) for Cosine-Weighted Hemisphere Sampling:
    // PDF(w) = cos(theta) / pi
    inline float CosineHemispherePdf(float cosTheta) {
        return cosTheta * std::numbers::inv_pi_v<float>;
    }

    // Uniformly samples directions over the entire 3D unit sphere S^2.
    // PDF = 1 / (4 * pi)
    inline Vector3f UniformSampleSphere(const Point2f& u) {
        float z = 1.0f - 2.0f * u.x; // Map u.x in [0,1] to z in [1, -1]
        float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        float phi = 2.0f * std::numbers::pi_v<float> * u.y; // Azimuthal angle in [0, 2*pi]
        return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
    }

    // Probability Density Function (PDF) for Uniform Sphere Sampling:
    // PDF = 1 / (4 * pi)
    inline float UniformSpherePdf() {
        return 1.0f / (4.0f * std::numbers::pi_v<float>);
    }
}
