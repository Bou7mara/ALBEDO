#include "rt/shapes/sphere.h"
#include "rt/core/quadratic.h"

namespace rt {

bool Sphere::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    Ray objectRay = worldToObject_(ray);

    Vector3f oVec(objectRay.o.x, objectRay.o.y, objectRay.o.z);
    float a = LengthSquared(objectRay.d);
    float b = 2.0f * Dot(objectRay.d, oVec);
    float c = LengthSquared(oVec) - radius_ * radius_;

    float t0, t1;
    if (!Quadratic(a, b, c, &t0, &t1)) return false;

    if (t0 > objectRay.tMax || t1 <= 0.0f) return false;
    float tHit = t0;
    if (tHit <= 0.0f) {
        tHit = t1;
        if (tHit > objectRay.tMax) return false;
    }

    Point3f pObject = objectRay(tHit);
    Normal3f nObject(pObject.x / radius_, pObject.y / radius_, pObject.z / radius_);

    ray.tMax = tHit;

    isect->p = objectToWorld_(pObject);
    isect->n = Normalize(objectToWorld_(nObject));
    isect->wo = Normalize(objectToWorld_(-objectRay.d));
    isect->t = tHit;
    isect->shape = this;

    return true;
}

Bounds3f Sphere::WorldBound() const {
    Bounds3f objectBounds(Point3f(-radius_, -radius_, -radius_),
                           Point3f(radius_, radius_, radius_));
    Bounds3f worldBounds;
    for (int corner = 0; corner < 8; ++corner) {
        Point3f p((corner & 1) ? objectBounds.pMax.x : objectBounds.pMin.x,
                   (corner & 2) ? objectBounds.pMax.y : objectBounds.pMin.y,
                   (corner & 4) ? objectBounds.pMax.z : objectBounds.pMin.z);
        worldBounds = Union(worldBounds, objectToWorld_(p));
    }
    return worldBounds;
}

}
