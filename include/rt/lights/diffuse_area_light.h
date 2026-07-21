#pragma once
#include "rt/lights/light.h"
#include "rt/shapes/shape.h"
#include <memory>

namespace rt {
    class DiffuseAreaLight : public Light {
    public:
        DiffuseAreaLight(std::shared_ptr<Shape> shape) : shape_(std::move(shape)) {}

        LiSample Sample_Li(const Point3f& ref, const Point2f& u) const override;
        float Pdf_Li(const Point3f& ref, const Vector3f& wi) const override;
        float Power() const override;

        const Shape* GetShape() const { return shape_.get(); }

    private:
        std::shared_ptr<Shape> shape_;
    };
}
