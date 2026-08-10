#pragma once
#include "rt/shapes/shape.h"
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/normal3.h"
#include "rt/core/bounds3.h"
#include <memory>
#include <stdexcept>

namespace rt {

    class Quad : public Shape {
    public:
        Quad(const Point3f& p0, const Vector3f& e1, const Vector3f& e2,
             std::shared_ptr<BSDF> bsdf = nullptr);

        static Quad FromCorners(const Point3f& p0, const Point3f& p1,
                                const Point3f& p2, const Point3f& p3,
                                std::shared_ptr<BSDF> bsdf = nullptr);

        bool Intersect(const Ray& ray, SurfaceInteraction* isect) const override;
        Bounds3f WorldBound() const override;

        ShapeSample Sample(const Point2f& u) const;
        ShapeSample Sample(const Point3f& ref, const Point2f& u) const override;
        float Pdf(const Point3f& ref, const Vector3f& wi) const override;
        float Area() const override;

        const Point3f& P0() const { return p0_; }
        const Vector3f& E1() const { return e1_; }
        const Vector3f& E2() const { return e2_; }
        const Normal3f& Normal() const { return n_; }

    private:
        Point3f p0_;
        Vector3f e1_;
        Vector3f e2_;
        Normal3f n_;
        float area_;
    };

} // namespace rt
