#pragma once
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include <limits>

// My Ray Tracer's Core Namespace
namespace rt {

    // =============================================
    // 3D RAY CLASS (PARAMETRIC LINE P(t) = o + t*d)
    // =============================================

    // Core data structure for optical ray tracing.
    // Represents a semi-infinite line starting at origin 'o' and traveling along direction 'd'.
    class Ray {
    public:
        // --- Data Members ---
        Point3f o;   // Ray origin point P(0)
        Vector3f d;  // Ray direction vector (usually normalized, but not strictly forced!)
        
        // Obscure Keyword Rationale: Why is tMax 'mutable'?
        // During BVH traversal, functions accept 'const Ray&' to prevent modifying origin/dir,
        // BUT as closer intersections are found, we shrink tMax in-place to prune distant primitives!
        mutable float tMax; 
        
        float time;  // Shutter time sample [0, 1] used for rendering motion blur!

        // ------------
        // CONSTRUCTORS
        // ------------

        // Default constructor: Zero origin/direction, infinite ray length tMax = infinity
        constexpr Ray()
            : o(), d(), tMax(std::numeric_limits<float>::infinity()), time(0.0f) {}

        // Parameterized constructor
        constexpr Ray(const Point3f& origin, const Vector3f& direction,
                      float tMax_ = std::numeric_limits<float>::infinity(),
                      float time_ = 0.0f)
            : o(origin), d(direction), tMax(tMax_), time(time_) {}

        // --------------------------------------
        // PARAMETRIC EVALUATOR: P(t) = o + t * d
        // --------------------------------------

        // Evaluates 3D position along ray for scalar distance t: ray(5.0f) -> 3D hit point
        constexpr Point3f operator()(float t) const {
            return o + t * d;
        }
    };

    // ===========================
    // GLOBAL NON-MEMBER OPERATORS
    // ===========================

    // Stream output operator: prints Ray details for debugging
    inline std::ostream& operator<<(std::ostream& os, const Ray& r) {
        os << "Ray[o=" << r.o << ", d=" << r.d << ", tMax=" << r.tMax
           << ", time=" << r.time << "]";
        return os;
    }
}
