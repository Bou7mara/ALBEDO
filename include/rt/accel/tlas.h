#pragma once
#include "rt/accel/blas.h"
#include "rt/accel/bvh.h"
#include "rt/shapes/instance.h"
#include <memory>
#include <vector>

namespace rt {

class TLAS : public BLAS {
public:
    struct Stats {
        size_t numInstances = 0;
        size_t numUniqueBLAS = 0;
    };

    TLAS() = default;
    explicit TLAS(std::vector<std::shared_ptr<Instance>> instances,
                  int maxPrimsInNode = 4,
                  int numThreads = 0);

    void Add(std::shared_ptr<Instance> instance);
    void Build(int numThreads = 0, int maxPrimsInNode = 4);

    bool Intersect(const Ray& ray, SurfaceInteraction* isect) const override;
    bool IntersectP(const Ray& ray) const override;
    Bounds3f WorldBound() const override;

    size_t InstanceCount() const { return instances_.size(); }
    size_t UniqueBLASCount() const;

    const std::vector<std::shared_ptr<Instance>>& Instances() const { return instances_; }
    const BVH* GetBVH() const { return bvh_.get(); }

    Stats GetStats() const;

private:
    std::vector<std::shared_ptr<Instance>> instances_;
    std::unique_ptr<BVH> bvh_;
};

} // namespace rt
