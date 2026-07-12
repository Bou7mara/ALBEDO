#ifndef RT_SCENE_SCENE_H
#define RT_SCENE_SCENE_H

#include "rt/shapes/shape.h"
#include <memory>
#include <vector>

namespace rt {

class Scene {
public:
    void Add(std::shared_ptr<Shape> shape) {
        shapes_.push_back(std::move(shape));
    }

    // Returns true if ray hits anything in the scene within (0, ray.tMax).
    // On success, isect holds the CLOSEST hit -- achieved by relying on
    // each Shape::Intersect shrinking ray.tMax as it finds a hit, so
    // later shapes in the scan can only report something strictly closer.
    bool Intersect(const Ray& ray, SurfaceInteraction* isect) const {
        bool hitAnything = false;
        for (const auto& shape : shapes_) {
            if (shape->Intersect(ray, isect)) {
                hitAnything = true;
            }
        }
        return hitAnything;
    }

private:
    std::vector<std::shared_ptr<Shape>> shapes_;
};

} // namespace rt

#endif // RT_SCENE_SCENE_H
