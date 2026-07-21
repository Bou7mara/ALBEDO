#pragma once
#include "rt/accel/bvh.h"
#include "rt/shapes/shape.h"
#include "rt/lights/diffuse_area_light.h"
#include <memory>
#include <vector>

namespace rt {
    class Scene {
    public:
        void Add(std::shared_ptr<Shape> shape) {
            if (shape->GetBSDF()) {
                // Check if it has non-zero emission
                Vector3f le = shape->GetBSDF()->Le(Vector3f(0, 0, 1), Vector3f(0, 0, 1));
                if (le.x > 0.0f || le.y > 0.0f || le.z > 0.0f) {
                    lights_.push_back(std::make_shared<DiffuseAreaLight>(shape));
                }
            }
            shapes_.push_back(std::move(shape));
        }

        void Build() {
            bvh_ = std::make_unique<BVH>(shapes_);
        }

        bool Intersect(const Ray& ray, SurfaceInteraction* isect) const {
            return bvh_ && bvh_->Intersect(ray, isect);
        }
        
        bool IntersectP(const Ray& ray) const {
            return bvh_ && bvh_->IntersectP(ray);
        }

        const std::vector<std::shared_ptr<Light>>& Lights() const {
            return lights_;
        }

    private:
        std::vector<std::shared_ptr<Shape>> shapes_;
        std::vector<std::shared_ptr<Light>> lights_;
        std::unique_ptr<BVH> bvh_;
    };
}
