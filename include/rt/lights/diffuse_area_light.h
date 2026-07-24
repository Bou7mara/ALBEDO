#pragma once
#include "rt/lights/light.h"
#include "rt/shapes/shape.h"
#include <memory>

namespace rt {
    // Diffuse area light source emitted by a 3D geometry shape (such as a sphere or quad)
    class DiffuseAreaLight : public Light {
    public:
        // Constructs area light attached to a specific shape
        DiffuseAreaLight(std::shared_ptr<Shape> shape) : shape_(std::move(shape)) {}

        // Samples a point on the shape surface and returns incoming light radiance
        LiSample Sample_Li(const Point3f& ref, const Point2f& u) const override;

        // Calculates probability density function (PDF) for sampling direction wi towards the light shape
        float Pdf_Li(const Point3f& ref, const Vector3f& wi) const override;

        // Calculates total power emitted by the light shape over its surface area
        float Power() const override;

        // Gets raw pointer to underlying geometry shape
        const Shape* GetShape() const { return shape_.get(); }

    private:
        std::shared_ptr<Shape> shape_; // Geometry shape emitting light
    };
}

