#ifndef RT_SHAPES_SHAPE_H
#define RT_SHAPES_SHAPE_H

#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/normal3.h"
#include "rt/core/ray.h"
#include "rt/materials/bsdf.h"
#include <memory>

namespace rt {

class Shape;

struct SurfaceInteraction {
    Point3f p;                      // hit point, in world space
    Normal3f n;                      // geometric surface normal, in world space, always normalized
    Vector3f wo;                     // outgoing direction, pointing back toward ray origin
    float t = 0.0f;                  // parametric distance along the ray
    const Shape* shape = nullptr;    // pointer to the shape hit
};

class Shape {
public:
    explicit Shape(std::shared_ptr<BSDF> bsdf = nullptr)
        : bsdf_(std::move(bsdf)) {}
    virtual ~Shape() = default;

    // Returns true if ray hits this shape within (0, ray.tMax).
    // On success, fills isect and shrinks ray.tMax to the hit distance.
    virtual bool Intersect(const Ray& ray, SurfaceInteraction* isect) const = 0;

    // Cheaper boolean-only test. Default implementation just discards Intersect's output.
    virtual bool IntersectP(const Ray& ray) const {
        SurfaceInteraction isect;
        Ray r = ray;   // local copy so tMax mutation doesn't leak to caller
        return Intersect(r, &isect);
    }

    const BSDF* GetBSDF() const { return bsdf_.get(); }

protected:
    std::shared_ptr<BSDF> bsdf_;
};

} // namespace rt

#endif // RT_SHAPES_SHAPE_H
