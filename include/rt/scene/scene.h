#pragma once
#include "rt/accel/bvh.h"
#include "rt/shapes/shape.h"
#include "rt/lights/diffuse_area_light.h"
#include <memory>
#include <vector>

namespace rt {
    // 3D Scene class holding shapes, lights, acceleration structure, and power-weighted light sampling distributions
    class Scene {
    public:
        // Adds a 3D shape to the scene and automatically registers it as a light source if its material emits radiance
        void Add(std::shared_ptr<Shape> shape) {
            if (shape->GetBSDF()) {
                Vector3f le = shape->GetBSDF()->Le(Vector3f(0, 0, 1), Vector3f(0, 0, 1));
                if (le.x > 0.0f || le.y > 0.0f || le.z > 0.0f) {
                    lights_.push_back(std::make_shared<DiffuseAreaLight>(shape));
                }
            }
            shapes_.push_back(std::move(shape));
        }

        // Builds the BVH acceleration structure and power-weighted CDF/PMF light sampling arrays
        void Build() {
            bvh_ = std::make_unique<BVH>(shapes_);

            lightPowers_.clear();
            lightCdf_.clear();
            lightPmf_.clear();
            totalLightPower_ = 0.0f;

            // Calculate total power emitted by each light source
            for (const auto& light : lights_) {
                float p = light->Power();
                lightPowers_.push_back(p);
                totalLightPower_ += p;
            }

            // Build power-weighted Probability Mass Function (PMF) and Cumulative Distribution Function (CDF)
            if (totalLightPower_ > 0.0f) {
                float cumulative = 0.0f;
                for (float p : lightPowers_) {
                    float pmf = p / totalLightPower_;
                    lightPmf_.push_back(pmf);
                    cumulative += pmf;
                    lightCdf_.push_back(cumulative);
                }
            } else if (!lights_.empty()) {
                // Fallback to uniform light sampling if total light power is zero
                float uniformPmf = 1.0f / static_cast<float>(lights_.size());
                float cumulative = 0.0f;
                for (size_t i = 0; i < lights_.size(); ++i) {
                    lightPmf_.push_back(uniformPmf);
                    cumulative += uniformPmf;
                    lightCdf_.push_back(cumulative);
                }
            }
        }

        // Tests ray intersection against the scene BVH tree
        bool Intersect(const Ray& ray, SurfaceInteraction* isect) const {
            return bvh_ && bvh_->Intersect(ray, isect);
        }
        
        // Fast shadow ray occlusion test against the scene BVH tree
        bool IntersectP(const Ray& ray) const {
            return bvh_ && bvh_->IntersectP(ray);
        }

        // Gets list of all light sources in the scene
        const std::vector<std::shared_ptr<Light>>& Lights() const {
            return lights_;
        }

        // Samples a single light source from the scene proportional to its power using random scalar u
        const Light* SampleLight(float u, int* lightIdx, float* pmf) const {
            if (lights_.empty()) {
                if (lightIdx) *lightIdx = -1;
                if (pmf) *pmf = 0.0f;
                return nullptr;
            }

            int idx = 0;
            int count = static_cast<int>(lights_.size());
            while (idx < count - 1 && u > lightCdf_[idx]) {
                idx++;
            }

            if (lightIdx) *lightIdx = idx;
            if (pmf) *pmf = lightPmf_[idx];
            return lights_[idx].get();
        }

        // Calculates PMF probability of selecting a specific shape's area light
        float LightPmf(const Shape* shape) const {
            for (size_t i = 0; i < lights_.size(); ++i) {
                auto areaLight = std::dynamic_pointer_cast<DiffuseAreaLight>(lights_[i]);
                if (areaLight && areaLight->GetShape() == shape) {
                    return lightPmf_[i];
                }
            }
            return 0.0f;
        }

    private:
        std::vector<std::shared_ptr<Shape>> shapes_; // List of all geometry shapes in scene
        std::vector<std::shared_ptr<Light>> lights_; // List of all light sources in scene
        std::vector<float> lightPowers_;             // Emitted power of each light
        std::vector<float> lightPmf_;                // Probability mass function for light sampling
        std::vector<float> lightCdf_;                // Cumulative distribution function for light sampling
        float totalLightPower_ = 0.0f;               // Sum of power emitted by all lights
        std::unique_ptr<BVH> bvh_;                   // BVH spatial acceleration structure
    };
}

