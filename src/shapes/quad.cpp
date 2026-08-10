#include "rt/shapes/quad.h"
#include <cmath>

namespace rt {

Quad::Quad(const Point3f& p0, const Vector3f& e1, const Vector3f& e2,
           std::shared_ptr<BSDF> bsdf)
    : Shape(std::move(bsdf)), p0_(p0), e1_(e1), e2_(e2) {
    Vector3f normalVec = Cross(e1_, e2_);
    area_ = Length(normalVec);
    if (area_ <= 1e-7f) {
        throw std::invalid_argument("Quad has zero or near-zero area (degenerate edges)");
    }
    n_ = Normal3f(normalVec.x / area_, normalVec.y / area_, normalVec.z / area_);
}

Quad Quad::FromCorners(const Point3f& p0, const Point3f& p1,
                       const Point3f& p2, const Point3f& p3,
                       std::shared_ptr<BSDF> bsdf) {
    Vector3f e1 = p1 - p0;
    Vector3f e2 = p3 - p0;
    Point3f expectedP2 = p0 + e1 + e2;
    if (Length(p2 - expectedP2) > 1e-3f) {
        throw std::invalid_argument("Corners do not form a planar parallelogram (p2 != p0 + e1 + e2)");
    }
    return Quad(p0, e1, e2, std::move(bsdf));
}

bool Quad::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
    float denom = Dot(ray.d, n_);
    if (std::abs(denom) < 1e-8f) return false;

    float t = Dot(p0_ - ray.o, n_) / denom;
    if (t <= 1e-4f || t > ray.tMax) return false;

    Point3f pHit = ray(t);
    Vector3f d = pHit - p0_;

    float a = Dot(e1_, e1_);
    float b = Dot(e1_, e2_);
    float c = Dot(e2_, e2_);
    float d1 = Dot(d, e1_);
    float d2 = Dot(d, e2_);

    float det = a * c - b * b;
    if (std::abs(det) < 1e-8f) return false;

    float u = (d1 * c - d2 * b) / det;
    float v = (d2 * a - d1 * b) / det;

    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) return false;

    ray.tMax = t;

    isect->p = pHit;
    isect->n = FaceForward(n_, -ray.d);
    isect->ns = isect->n;
    isect->uv = Point2f(u, v);
    isect->wo = Normalize(-ray.d);
    isect->t = t;
    isect->shape = this;

    return true;
}

Bounds3f Quad::WorldBound() const {
    Point3f c0 = p0_;
    Point3f c1 = p0_ + e1_;
    Point3f c2 = p0_ + e1_ + e2_;
    Point3f c3 = p0_ + e2_;

    Bounds3f b(c0, c1);
    b = Union(b, c2);
    b = Union(b, c3);

    for (int i = 0; i < 3; ++i) {
        if (b.maxPt[i] - b.minPt[i] < 1e-4f) {
            b.minPt[i] -= 1e-4f;
            b.maxPt[i] += 1e-4f;
        }
    }
    return b;
}

ShapeSample Quad::Sample(const Point2f& u) const {
    ShapeSample sample;
    sample.p = p0_ + u.x * e1_ + u.y * e2_;
    sample.n = n_;
    sample.pdf = 1.0f / area_;
    return sample;
}

ShapeSample Quad::Sample(const Point3f& ref, const Point2f& u) const {
    Point3f pSample = p0_ + u.x * e1_ + u.y * e2_;
    ShapeSample sample;
    sample.p = pSample;
    sample.n = n_;

    Vector3f wi = pSample - ref;
    float distSq = LengthSquared(wi);
    if (distSq == 0.0f) {
        sample.pdf = 0.0f;
        return sample;
    }
    Vector3f wiNorm = wi / std::sqrt(distSq);

    float cosTheta = AbsDot(n_, -wiNorm);
    if (cosTheta < 1e-6f) {
        sample.pdf = 0.0f;
    } else {
        sample.pdf = distSq / (cosTheta * area_);
    }

    return sample;
}

float Quad::Pdf(const Point3f& ref, const Vector3f& wi) const {
    Ray ray(ref, wi);
    SurfaceInteraction isect;
    if (!Intersect(ray, &isect)) {
        return 0.0f;
    }

    float distSq = isect.t * isect.t * LengthSquared(wi);
    float cosTheta = AbsDot(isect.n, -wi);
    if (cosTheta < 1e-6f) {
        return 0.0f;
    }

    return distSq / (cosTheta * area_);
}

float Quad::Area() const {
    return area_;
}

} // namespace rt
