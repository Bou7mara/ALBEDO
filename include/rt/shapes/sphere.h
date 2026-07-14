#pragma once
#include "rt/shapes/shape.h"
#include "rt/core/transform.h"

namespace rt {
    class Sphere : public Shape {
    public:
        Sphere(const Transform& objectToWorld, float radius,
               std::shared_ptr<BSDF> bsdf = nullptr)
            : Shape(std::move(bsdf)),
              objectToWorld_(objectToWorld),
              worldToObject_(objectToWorld.Inverse()),
              radius_(radius) {}

        bool Intersect(const Ray& ray, SurfaceInteraction* isect) const override;
        Bounds3f WorldBound() const override;

    private:
        Transform objectToWorld_;
        Transform worldToObject_;
        float radius_;
    };
}