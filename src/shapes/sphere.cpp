#include "rt/shapes/sphere.h"
#include "rt/core/quadratic.h"

namespace rt {

bool Sphere::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    // Transform the ray into object space
    Ray objectRay = worldToObject_(ray);

    // Compute quadratic coefficients
    Vector3f oVec(objectRay.o.x, objectRay.o.y, objectRay.o.z);
    float a = LengthSquared(objectRay.d);
    float b = 2.0f * Dot(objectRay.d, oVec);
    float c = LengthSquared(oVec) - radius_ * radius_;

    float t0, t1;
    if (!Quadratic(a, b, c, &t0, &t1)) return false;

    // Find the smallest root within range (0, objectRay.tMax]
    if (t0 > objectRay.tMax || t1 <= 0.0f) return false;
    float tHit = t0;
    if (tHit <= 0.0f) {
        tHit = t1;
        if (tHit > objectRay.tMax) return false;
    }

    Point3f pObject = objectRay(tHit);
    Normal3f nObject(pObject.x / radius_, pObject.y / radius_, pObject.z / radius_);

    // Mutate world-space ray tMax to tHit
    ray.tMax = tHit;

    // Populate SurfaceInteraction in world space
    isect->p = objectToWorld_(pObject);
    isect->n = Normalize(objectToWorld_(nObject));
    // Transform incoming ray direction to world space explicitly
    isect->wo = Normalize(objectToWorld_(Vector3f(-objectRay.d.x, -objectRay.d.y, -objectRay.d.z)));
    isect->t = tHit;
    isect->shape = this;

    return true;
}

} // namespace rt
