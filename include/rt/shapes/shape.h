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

    struct SurfaceInteraction {
        Point3f p;
        Normal3f n;
        Vector3f wo;
        float t = 0.0f;
        const Shape* shape = nullptr;
    };

    class Shape {
    public:
        explicit Shape(std::shared_ptr<BSDF> bsdf = nullptr)
            : bsdf_(std::move(bsdf)) {}
        virtual ~Shape() = default;

        virtual bool Intersect(const Ray& ray, SurfaceInteraction* isect) const = 0;
        virtual Bounds3f WorldBound() const = 0;

        virtual bool IntersectP(const Ray& ray) const {
            SurfaceInteraction isect;
            Ray r = ray;
            return Intersect(r, &isect);
        }

        const BSDF* GetBSDF() const { return bsdf_.get(); }

    protected:
        std::shared_ptr<BSDF> bsdf_;
    };
}