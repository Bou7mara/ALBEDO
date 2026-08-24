#include "rt/accel/tlas.h"
#include <unordered_set>

namespace rt {

TLAS::TLAS(std::vector<std::shared_ptr<Instance>> instances,
           int maxPrimsInNode,
           int numThreads)
    : instances_(std::move(instances)) {
    Build(numThreads, maxPrimsInNode);
}

void TLAS::Add(std::shared_ptr<Instance> instance) {
    if (instance) {
        instances_.push_back(std::move(instance));
    }
}

void TLAS::Build(int numThreads, int maxPrimsInNode) {
    if (instances_.empty()) {
        bvh_.reset();
        return;
    }

    std::vector<std::shared_ptr<Shape>> shapes;
    shapes.reserve(instances_.size());
    for (const auto& inst : instances_) {
        shapes.push_back(inst);
    }

    bvh_ = std::make_unique<BVH>(std::move(shapes), maxPrimsInNode, BVH::SplitMethod::SAH, numThreads);
}

bool TLAS::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    if (!bvh_) return false;
    return bvh_->Intersect(ray, isect);
}

bool TLAS::IntersectP(const Ray& ray) const {
    if (!bvh_) return false;
    return bvh_->IntersectP(ray);
}

Bounds3f TLAS::WorldBound() const {
    return bvh_ ? bvh_->WorldBound() : Bounds3f();
}

size_t TLAS::UniqueBLASCount() const {
    std::unordered_set<const BLAS*> uniqueBlas;
    for (const auto& inst : instances_) {
        if (inst && inst->GetBLAS()) {
            uniqueBlas.insert(inst->GetBLAS().get());
        }
    }
    return uniqueBlas.size();
}

TLAS::Stats TLAS::GetStats() const {
    Stats stats;
    stats.numInstances = instances_.size();
    stats.numUniqueBLAS = UniqueBLASCount();
    return stats;
}

}
