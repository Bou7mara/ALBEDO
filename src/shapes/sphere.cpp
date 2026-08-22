#include "rt/shapes/sphere.h"
#include "rt/core/quadratic.h"
#include "rt/core/onb.h"
#include "rt/core/sampling.h"
#include <numbers>

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
        Point3f p((corner & 1) ? objectBounds.maxPt.x : objectBounds.minPt.x,
                   (corner & 2) ? objectBounds.maxPt.y : objectBounds.minPt.y,
                   (corner & 4) ? objectBounds.maxPt.z : objectBounds.minPt.z);
        worldBounds = Union(worldBounds, objectToWorld_(p));
    }
    return worldBounds;
}

ShapeSample Sphere::Sample(const Point3f& ref, const Point2f& u) const {
    Point3f center = objectToWorld_(Point3f(0, 0, 0));
    float worldRadius = Length(objectToWorld_(Vector3f(radius_, 0, 0)));
    Vector3f wc = center - ref;
    float d2 = LengthSquared(wc);
    float r2 = worldRadius * worldRadius;

    ShapeSample sample;
    
    if (d2 <= r2) {
        Vector3f objDir = UniformSampleSphere(u);
        Point3f pObj(objDir.x * radius_, objDir.y * radius_, objDir.z * radius_);
        Normal3f nObj(objDir.x, objDir.y, objDir.z);
        sample.p = objectToWorld_(pObj);
        sample.n = Normalize(objectToWorld_(nObj));
        
        Vector3f wi = sample.p - ref;
        float distSq = LengthSquared(wi);
        if (distSq == 0.0f) { sample.pdf = 0.0f; return sample; }
        wi = wi / std::sqrt(distSq);
        float cosTheta = AbsDot(sample.n, -wi);
        float areaPdf = 1.0f / (4.0f * std::numbers::pi_v<float> * r2);
        sample.pdf = areaPdf * distSq / cosTheta;
        return sample;
    }

    float sinThetaMax2 = r2 / d2;
    float cosThetaMax = std::sqrt(std::max(0.0f, 1.0f - sinThetaMax2));

    float cosTheta = (1.0f - u.x) + u.x * cosThetaMax;
    float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = u.y * 2.0f * std::numbers::pi_v<float>;

    float d = std::sqrt(d2);
    float ds = d * cosTheta - std::sqrt(std::max(0.0f, r2 - d * d * sinTheta * sinTheta));
    float cosAlpha = (d * d + r2 - ds * ds) / (2.0f * d * worldRadius);
    float sinAlpha = std::sqrt(std::max(0.0f, 1.0f - cosAlpha * cosAlpha));

    ONB onbCenter(Normalize(-wc));
    Vector3f nWorld = onbCenter.ToWorld(Vector3f(sinAlpha * std::cos(phi), sinAlpha * std::sin(phi), cosAlpha));
    
    sample.n = Normal3f(nWorld.x, nWorld.y, nWorld.z);
    sample.p = center + worldRadius * nWorld;
    sample.pdf = 1.0f / (2.0f * std::numbers::pi_v<float> * (1.0f - cosThetaMax));
    
    return sample;
}

float Sphere::Pdf(const Point3f& ref, const Vector3f& wi) const {
    Point3f center = objectToWorld_(Point3f(0, 0, 0));
    float worldRadius = Length(objectToWorld_(Vector3f(radius_, 0, 0)));
    Vector3f wc = center - ref;
    float d2 = LengthSquared(wc);
    float r2 = worldRadius * worldRadius;

    if (d2 <= r2) {
        SurfaceInteraction isect;
        if (!Intersect(Ray(ref, wi), &isect)) return 0.0f;
        float areaPdf = 1.0f / (4.0f * std::numbers::pi_v<float> * r2);
        float distSq = isect.t * isect.t;
        float cosTheta = AbsDot(isect.n, -wi);
        return areaPdf * distSq / cosTheta;
    }

    float sinThetaMax2 = r2 / d2;
    float cosThetaMax = std::sqrt(std::max(0.0f, 1.0f - sinThetaMax2));
    
    float cosTheta = Dot(Normalize(wc), wi);
    if (cosTheta < cosThetaMax) {
        return 0.0f;
    }

    return 1.0f / (2.0f * std::numbers::pi_v<float> * (1.0f - cosThetaMax));
}

float Sphere::Area() const {
    float worldRadius = Length(objectToWorld_(Vector3f(radius_, 0, 0)));
    return 4.0f * std::numbers::pi_v<float> * worldRadius * worldRadius;
}

}
