#include "rt/shapes/mesh_instance.h"

namespace rt {

bool MeshInstance::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    if (!blas_) return false;
    Ray objRay = worldToObject_(ray);
    if (!blas_->Intersect(objRay, isect)) return false;

    isect->p = objectToWorld_(isect->p);
    isect->n = Normalize(objectToWorld_(isect->n));
    isect->ns = Normalize(objectToWorld_(isect->ns));
    isect->wo = -ray.d;
    isect->shape = this;
    return true;
}

bool MeshInstance::IntersectP(const Ray& ray) const {
    if (!blas_) return false;
    Ray objRay = worldToObject_(ray);
    return blas_->IntersectP(objRay);
}

Bounds3f MeshInstance::WorldBound() const {
    if (!blas_) return Bounds3f();
    return objectToWorld_(blas_->WorldBound());
}

} // namespace rt
