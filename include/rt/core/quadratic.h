#pragma once
#include <cmath>
#include <utility>

// My Ray Tracer's Core Namespace
namespace rt {

    // =============================================
    // STABLE QUADRATIC SOLVER (a*t^2 + b*t + c = 0)
    // =============================================

    // Solves quadratic equation a*t^2 + b*t + c = 0 for real roots t0 and t1 (t0 <= t1)
    // Returns false if discriminant < 0 (no real solutions/intersections)
    //
    // Obscure Choice & Stability Rationale:
    // Standard textbook formula t = (-b +/- sqrt(b^2 - 4ac)) / (2a) is wont work here
    // When b ~ sqrt(b^2 - 4ac), subtracting two nearly identical floating point numbers causes
    // cancellation (precision loss), which in turn causes rays to miss spheres/cylinders
    // I compute double precision intermediate 'q' = -0.5 * (b + sign(b)*sqrt(D)),
    // then solve r0 = q / a and r1 = c / q, guaranteeing maximum numerical precision
    inline bool Quadratic(float a, float b, float c, float* t0, float* t1) {
        // Upgrade inputs to 64-bit double precision for internal discriminant calculation
        double discrim = double(b) * double(b) - 4.0 * double(a) * double(c);
        if (discrim < 0.0) return false; // Negative discriminant = ray misses shape completely

        double rootDiscrim = std::sqrt(discrim);

        // Compute stable intermediate q avoiding cancellation
        double q = (b < 0) ? -0.5 * (b - rootDiscrim) : -0.5 * (b + rootDiscrim);
        double r0 = q / a;
        double r1 = c / q;
        
        // Ensure t0 is always the nearer root (t0 <= t1)
        if (r0 > r1) std::swap(r0, r1);

        *t0 = static_cast<float>(r0);
        *t1 = static_cast<float>(r1);
        return true;
    }
}
