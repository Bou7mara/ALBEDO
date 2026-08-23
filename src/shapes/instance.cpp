#include "rt/shapes/instance.h"

namespace rt {

Instance::Instance(std::shared_ptr<BLAS> blas,
                   const Transform& objectToWorld,
                   std::shared_ptr<BSDF> bsdf)
    : Shape(std::move(bsdf)),
      blas_(std::move(blas)),
      objectToWorld_(objectToWorld),
      worldToObject_(objectToWorld.Inverse()) {
    if (blas_) {
        worldBounds_ = objectToWorld_(blas_->WorldBound());
    }
}

bool Instance::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    if (!blas_) return false;
    Ray objRay = worldToObject_(ray);
    if (!blas_->Intersect(objRay, isect)) return false;

    ray.tMax = objRay.tMax;
    isect->p = objectToWorld_(isect->p);
    isect->n = Normalize(objectToWorld_(isect->n));
    isect->ns = Normalize(objectToWorld_(isect->ns));
    isect->wo = -ray.d;
    isect->shape = this;
    return true;
}

bool Instance::IntersectP(const Ray& ray) const {
    if (!blas_) return false;
    Ray objRay = worldToObject_(ray);
    return blas_->IntersectP(objRay);
}

Bounds3f Instance::WorldBound() const {
    return worldBounds_;
}

} // namespace rt
