#pragma once
#include "rt/shapes/shape.h"
#include "rt/core/transform.h"

namespace rt {
    // 3D sphere geometry shape centered at origin in object space and positioned in world space via transform
    class Sphere : public Shape {
    public:
        // Constructs sphere with specified object-to-world transform, radius, and optional BSDF material
        Sphere(const Transform& objectToWorld, float radius,
               std::shared_ptr<BSDF> bsdf = nullptr)
            : Shape(std::move(bsdf)),
              objectToWorld_(objectToWorld),
              worldToObject_(objectToWorld.Inverse()),
              radius_(radius) {}

        // Tests ray intersection against the sphere using quadratic formula
        bool Intersect(const Ray& ray, SurfaceInteraction* isect) const override;

        // Calculates axis-aligned bounding box enclosing the sphere in world space
        Bounds3f WorldBound() const override;

        // Samples a point on the sphere surface visible from reference point ref
        ShapeSample Sample(const Point3f& ref, const Point2f& u) const override;

        // Calculates solid angle PDF for sampling direction wi towards sphere surface
        float Pdf(const Point3f& ref, const Vector3f& wi) const override;

        // Calculates total surface area of the sphere (4 * pi * r^2)
        float Area() const override;

    private:
        Transform objectToWorld_; // Transform from sphere local space to world space
        Transform worldToObject_; // Inverse transform from world space to sphere local space
        float radius_;            // Sphere radius
    };
}