#pragma once
#include "rt/accel/bvh.h"
#include "rt/core/transform.h"
#include "rt/shapes/shape.h"
#include <memory>

namespace rt {

    class MeshInstance : public Shape {
    public:
        MeshInstance(std::shared_ptr<BVH> blas,
                     const Transform& objectToWorld,
                     std::shared_ptr<BSDF> bsdf = nullptr)
            : Shape(std::move(bsdf)),
              blas_(std::move(blas)),
              objectToWorld_(objectToWorld),
              worldToObject_(objectToWorld.Inverse()) {}

        bool Intersect(const Ray& ray, SurfaceInteraction* isect) const override;
        bool IntersectP(const Ray& ray) const override;
        Bounds3f WorldBound() const override;

        ShapeSample Sample(const Point3f& ref, const Point2f& u) const override {
            (void)ref; (void)u;
            return ShapeSample{Point3f(0, 0, 0), Normal3f(0, 0, 0), 0.0f};
        }
        float Pdf(const Point3f& ref, const Vector3f& wi) const override {
            (void)ref; (void)wi;
            return 0.0f;
        }
        float Area() const override {
            return 0.0f;
        }

        const std::shared_ptr<BVH>& GetBLAS() const { return blas_; }
        const Transform& GetTransform() const { return objectToWorld_; }

    private:
        std::shared_ptr<BVH> blas_;
        Transform objectToWorld_;
        Transform worldToObject_;
    };

} // namespace rt
