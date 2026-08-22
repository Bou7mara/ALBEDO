#include "rt/shapes/triangle.h"
#include <cmath>
#include <algorithm>
#include <memory>
#include <vector>

namespace rt {

    bool Triangle::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
        const Point3f& v0 = V0();
        const Point3f& v1 = V1();
        const Point3f& v2 = V2();

        const Vector3f e1 = v1 - v0;
        const Vector3f e2 = v2 - v0;

        const Vector3f pVec = Cross(ray.d, e2);
        const float det = Dot(e1, pVec);

        if (std::abs(det) < kParallelEpsilon) return false;

        const float invDet = 1.0f / det;

        const Vector3f tVec = ray.o - v0;
        const float u = Dot(tVec, pVec) * invDet;
        if (u < 0.0f || u > 1.0f) return false;

        const Vector3f qVec = Cross(tVec, e1);
        const float v = Dot(ray.d, qVec) * invDet;
        if (v < 0.0f || u + v > 1.0f) return false;

        const float t = Dot(e2, qVec) * invDet;
        if (t <= 1e-4f || t > ray.tMax) return false;

        const float w = 1.0f - u - v;

        ray.tMax = t;

        isect->p  = ray(t);
        isect->n  = Normal3f(Normalize(Cross(e1, e2)));
        isect->ns = HasShadingNormals()
                        ? Normal3f(Normalize(w * N0() + u * N1() + v * N2()))
                        : isect->n;
        isect->uv = HasUVs()
                        ? Point2f(w * UV0().x + u * UV1().x + v * UV2().x,
                                  w * UV0().y + u * UV1().y + v * UV2().y)
                        : Point2f(0.0f, 0.0f);
        isect->wo    = -ray.d;
        isect->t     = t;
        isect->shape = this;
        return true;
    }

    bool Triangle::IntersectP(const Ray& ray) const {
        const Point3f& v0 = V0();
        const Point3f& v1 = V1();
        const Point3f& v2 = V2();

        const Vector3f e1 = v1 - v0;
        const Vector3f e2 = v2 - v0;

        const Vector3f pVec = Cross(ray.d, e2);
        const float det = Dot(e1, pVec);
        if (std::abs(det) < kParallelEpsilon) return false;

        const float invDet = 1.0f / det;
        const Vector3f tVec = ray.o - v0;
        const float u = Dot(tVec, pVec) * invDet;
        if (u < 0.0f || u > 1.0f) return false;

        const Vector3f qVec = Cross(tVec, e1);
        const float v = Dot(ray.d, qVec) * invDet;
        if (v < 0.0f || u + v > 1.0f) return false;

        const float t = Dot(e2, qVec) * invDet;
        return t > 1e-4f && t <= ray.tMax;
    }

    Bounds3f Triangle::WorldBound() const {
        Bounds3f b(V0());
        b = Union(b, V1());
        b = Union(b, V2());
        return b;
    }

    float Triangle::Area() const {
        return 0.5f * Length(Cross(V1() - V0(), V2() - V0()));
    }

    ShapeSample Triangle::Sample(const Point3f& ref, const Point2f& u) const {
        float b0 = u.x;
        float b1 = u.y;
        if (b0 + b1 > 1.0f) {
            b0 = 1.0f - b0;
            b1 = 1.0f - b1;
        }
        const float b2 = 1.0f - b0 - b1;

        const Point3f p = Point3f(
            b2 * V0().x + b0 * V1().x + b1 * V2().x,
            b2 * V0().y + b0 * V1().y + b1 * V2().y,
            b2 * V0().z + b0 * V1().z + b1 * V2().z);

        const Normal3f n = Normal3f(Normalize(Cross(V1() - V0(), V2() - V0())));

        const float area = Area();
        if (area <= 0.0f) return ShapeSample{p, n, 0.0f};

        Vector3f wi = p - ref;
        float distSq = LengthSquared(wi);
        if (distSq == 0.0f) return ShapeSample{p, n, 0.0f};
        Vector3f wiNorm = wi / std::sqrt(distSq);

        float cosTheta = AbsDot(n, -wiNorm);
        if (cosTheta < 1e-6f) return ShapeSample{p, n, 0.0f};

        float pdf = distSq / (cosTheta * area);
        return ShapeSample{p, n, pdf};
    }

    float Triangle::Pdf(const Point3f& ref, const Vector3f& wi) const {
        Ray ray(ref, wi);
        SurfaceInteraction isect;
        if (!Intersect(ray, &isect)) return 0.0f;

        const float area = Area();
        if (area <= 0.0f) return 0.0f;

        float distSq = isect.t * isect.t * LengthSquared(wi);
        float cosTheta = AbsDot(isect.n, -wi);
        if (cosTheta < 1e-6f) return 0.0f;

        return distSq / (cosTheta * area);
    }

    std::vector<std::shared_ptr<Shape>> MakeTriangleMesh(
        std::shared_ptr<TriangleMesh> mesh, std::shared_ptr<BSDF> bsdf) {
        std::vector<std::shared_ptr<Shape>> triangles;
        triangles.reserve(mesh->TriangleCount());
        for (int i = 0; i < mesh->TriangleCount(); ++i) {
            triangles.push_back(std::make_shared<Triangle>(mesh, i, bsdf));
        }
        return triangles;
    }
}