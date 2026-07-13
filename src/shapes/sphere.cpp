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
    // NOTE: This relies on callers offsetting ray origins away from the surface to prevent
    // self-intersection (shadow acne); see TOC 3.1.1 for the general fix.
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
    isect->wo = Normalize(objectToWorld_(-objectRay.d));
    isect->t = tHit;
    isect->shape = this;

    return true;
}

Bounds3f Sphere::WorldBound() const {
    // Transform all 8 corners of the object-space axis-aligned box
    // and union them -- correct for ANY affine transform (including
    // non-uniform scale, where a sphere genuinely becomes an
    // ellipsoid and a hand-derived "center +/- radius" shortcut would
    // be wrong). Not maximally tight for a plain rotation of a sphere
    // -- rotating a ball doesn't change its extent, but the 8-corner
    // method doesn't know that -- yet it's still a valid, safe
    // over-approximation, and BVH correctness only needs bounds that
    // are conservative, never bounds that are minimal. This is the
    // same general-purpose technique pbrt uses as its Shape default.
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

} // namespace rt
