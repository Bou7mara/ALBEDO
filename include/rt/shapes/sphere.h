#ifndef RT_SHAPES_SPHERE_H
#define RT_SHAPES_SPHERE_H

#include "rt/shapes/shape.h"
#include "rt/core/transform.h"

namespace rt {

class Sphere : public Shape {
public:
    Sphere(const Transform& objectToWorld, float radius,
           std::shared_ptr<BSDF> bsdf = nullptr, bool isBumpy = false)
        : Shape(std::move(bsdf)),
          objectToWorld_(objectToWorld),
          worldToObject_(objectToWorld.Inverse()),
          radius_(radius),
          isBumpy_(isBumpy) {}

    bool Intersect(const Ray& ray, SurfaceInteraction* isect) const override;

private:
    Transform objectToWorld_;
    Transform worldToObject_;
    float radius_;
    bool isBumpy_;
};

} // namespace rt

#endif // RT_SHAPES_SPHERE_H
