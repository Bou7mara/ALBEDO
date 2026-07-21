#pragma once
#include "rt/core/vector3.h"
#include <cmath>

// Ray Tracer Core Namespace
namespace rt {
    // =============================================
    // ORTHONORMAL BASIS CLASS (ONB / SHADING FRAME)
    // =============================================

    // Represents a local 3D coordinate frame defined by 3 unit vectors (u, v, w).
    // Heavily used in Monte Carlo path tracing to transform local hemisphere samples into world directions!
    struct ONB {
        // --- Data Members ---
        Vector3f u, v, w; // Basis vectors (w = normal, u = tangent, v = bitangent)

        // ------------
        // CONSTRUCTORS
        // ------------

        // Constructs an orthonormal basis from a single unit normal vector 'n'.
        // Obscure Math Rationale: Uses Duff et al. (2017) "Building an Orthonormal Basis, Revisited".
        // Eliminates branch conditionals (if), square roots (sqrt), and pole singularities at (0,0,-1)!
        explicit ONB(const Vector3f& n) {
            w = n;
            // Preserves sign of w.z (+1.0f or -1.0f) to avoid divide-by-zero when normal points straight down
            float sign = std::copysign(1.0f, w.z);
            float a = -1.0f / (sign + w.z); // Duff et al. magic constant
            float b = w.x * w.y * a;
            
            // Construct tangent u and bitangent v orthogonal to w
            u = Vector3f(1.0f + sign * w.x * w.x * a, sign * b, -sign * w.x);
            v = Vector3f(b, sign + w.y * w.y * a, -w.y);
        }

        // --------------------------
        // COORDINATE TRANSFORMATIONS
        // --------------------------

        // Converts a vector from local shading space (where Z is normal) to world space coordinates.
        // World Vector = local.x * u + local.y * v + local.z * w
        Vector3f ToWorld(const Vector3f& local) const {
            return local.x * u + local.y * v + local.z * w;
        }
    };
}
