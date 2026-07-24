#pragma once
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/normal3.h"
#include "rt/core/ray.h"
#include "rt/materials/bsdf.h"
#include "rt/core/bounds3.h"
#include <memory>

namespace rt {
    class Shape;

    // Structure storing details of a ray-surface intersection hit
    struct SurfaceInteraction {
        Point3f p;                   // 3D hit point location in world space
        Normal3f n;                  // Surface normal vector at hit point
        Vector3f wo;                 // Outgoing ray direction pointing back towards ray origin (-ray.d)
        float t = 0.0f;              // Ray distance parameter t at hit point
        const Shape* shape = nullptr;// Pointer to the intersected shape object
    };

    // Structure storing results of sampling a point on a shape surface
    struct ShapeSample {
        Point3f p;  // Sampled 3D point location on the shape surface
        Normal3f n; // Surface normal vector at sampled point
        float pdf;  // Probability density function value for sampling this point
    };

    // Abstract base class for 3D geometric shapes (such as spheres, triangles, or quads)
    class Shape {
    public:
        // Constructs a shape attached to an optional material (BSDF)
        explicit Shape(std::shared_ptr<BSDF> bsdf = nullptr)
            : bsdf_(std::move(bsdf)) {}
        virtual ~Shape() = default;

        // Samples a point on the shape surface visible from reference point ref using 2D random sample u
        virtual ShapeSample Sample(const Point3f& ref, const Point2f& u) const = 0;

        // Calculates PDF for sampling direction wi from reference point ref towards the shape
        virtual float Pdf(const Point3f& ref, const Vector3f& wi) const = 0;

        // Calculates total surface area of the 3D shape
        virtual float Area() const = 0;

        // Tests ray intersection against the shape and updates surface interaction details if hit
        virtual bool Intersect(const Ray& ray, SurfaceInteraction* isect) const = 0;

        // Gets world-space bounding box enclosing the shape
        virtual Bounds3f WorldBound() const = 0;

        // Fast occlusion check to see if ray hits the shape
        virtual bool IntersectP(const Ray& ray) const {
            SurfaceInteraction isect;
            Ray r = ray;
            return Intersect(r, &isect);
        }

        // Gets raw pointer to shape's material BSDF
        const BSDF* GetBSDF() const { return bsdf_.get(); }

    protected:
        std::shared_ptr<BSDF> bsdf_; // Material associated with the shape
    };
}