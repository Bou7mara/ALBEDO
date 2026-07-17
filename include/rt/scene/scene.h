#pragma once
#include "rt/accel/bvh.h"
#include "rt/shapes/shape.h"
#include <memory>
#include <vector>

namespace rt {
    class Scene {
    public:
        void Add(std::shared_ptr<Shape> shape) {
            shapes_.push_back(std::move(shape));
        }

        void Build() {
            bvh_ = std::make_unique<BVH>(shapes_);
        }

        bool Intersect(const Ray& ray, SurfaceInteraction* isect) const {
            return bvh_ && bvh_->Intersect(ray, isect);
        }

    private:
        std::vector<std::shared_ptr<Shape>> shapes_;
        std::unique_ptr<BVH> bvh_;
    };
}
