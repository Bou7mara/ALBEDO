#pragma once
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/normal3.h"
#include "rt/core/ray.h"
#include "rt/core/bounds3.h"
#include <cmath>
#include <cstring>
#include <array>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    class Transform {
    public:
        float m[4][4];
        float mInv[4][4];

        Transform() = default;

        explicit Transform(const float mat[4][4]);

        __host__ __device__ Transform(const float mat[4][4], const float matInv[4][4]) {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    m[i][j] = mat[i][j];
                    mInv[i][j] = matInv[i][j];
                }
            }
        }

        __host__ __device__ static Transform Identity() {
            float mat[4][4] = {
                {1.0f, 0.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f},
                {0.0f, 0.0f, 0.0f, 1.0f}
            };
            return Transform(mat, mat);
        }

        __host__ __device__ static Transform Translate(const Vector3f& delta) {
            float mat[4][4] = {
                {1.0f, 0.0f, 0.0f, delta.x},
                {0.0f, 1.0f, 0.0f, delta.y},
                {0.0f, 0.0f, 1.0f, delta.z},
                {0.0f, 0.0f, 0.0f, 1.0f}
            };
            float inv[4][4] = {
                {1.0f, 0.0f, 0.0f, -delta.x},
                {0.0f, 1.0f, 0.0f, -delta.y},
                {0.0f, 0.0f, 1.0f, -delta.z},
                {0.0f, 0.0f, 0.0f, 1.0f}
            };
            return Transform(mat, inv);
        }

        __host__ __device__ static Transform Scale(float sx, float sy, float sz) {
            float mat[4][4] = {
                {sx, 0.0f, 0.0f, 0.0f},
                {0.0f, sy, 0.0f, 0.0f},
                {0.0f, 0.0f, sz, 0.0f},
                {0.0f, 0.0f, 0.0f, 1.0f}
            };
            float inv[4][4] = {
                {1.0f / sx, 0.0f, 0.0f, 0.0f},
                {0.0f, 1.0f / sy, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f / sz, 0.0f},
                {0.0f, 0.0f, 0.0f, 1.0f}
            };
            return Transform(mat, inv);
        }

        static Transform RotateX(float thetaDeg);
        static Transform RotateY(float thetaDeg);
        static Transform RotateZ(float thetaDeg);

        static Transform LookAt(const Point3f& eye, const Point3f& look, const Vector3f& up);

        __host__ std::array<float, 12> ToOptixRowMajor3x4() const {
            return {
                m[0][0], m[0][1], m[0][2], m[0][3],
                m[1][0], m[1][1], m[1][2], m[1][3],
                m[2][0], m[2][1], m[2][2], m[2][3]
            };
        }

        __host__ __device__ Transform Inverse() const {
            return Transform(mInv, m);
        }

        __host__ __device__ bool SwapsHandedness() const {
            float det =
                m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
            return det < 0.0f;
        }

        __host__ __device__ Point3f operator()(const Point3f& p) const {
            float x = p.x, y = p.y, z = p.z;
            float xp = m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
            float yp = m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
            float zp = m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];

            float wp = m[3][0] * x + m[3][1] * y + m[3][2] * z + m[3][3];
            if (wp == 1.0f) return Point3f(xp, yp, zp);
            return Point3f(xp, yp, zp) / wp;
        }

        __host__ __device__ Vector3f operator()(const Vector3f& v) const {
            float x = v.x, y = v.y, z = v.z;
            return Vector3f(
                m[0][0] * x + m[0][1] * y + m[0][2] * z,
                m[1][0] * x + m[1][1] * y + m[1][2] * z,
                m[2][0] * x + m[2][1] * y + m[2][2] * z
            );
        }

        __host__ __device__ Normal3f operator()(const Normal3f& n) const {
            float x = n.x, y = n.y, z = n.z;
            float rx = mInv[0][0] * x + mInv[1][0] * y + mInv[2][0] * z;
            float ry = mInv[0][1] * x + mInv[1][1] * y + mInv[2][1] * z;
            float rz = mInv[0][2] * x + mInv[1][2] * y + mInv[2][2] * z;
            return Normal3f(rx, ry, rz);
        }

        __host__ __device__ Ray operator()(const Ray& r) const {
            Point3f newO = (*this)(r.o);
            Vector3f newD = (*this)(r.d);
            return Ray(newO, newD, r.tMax, r.time);
        }

        __host__ __device__ Bounds3f operator()(const Bounds3f& b) const {
            Bounds3f out;
            out = Union(out, (*this)(Point3f(b.minPt.x, b.minPt.y, b.minPt.z)));
            out = Union(out, (*this)(Point3f(b.maxPt.x, b.minPt.y, b.minPt.z)));
            out = Union(out, (*this)(Point3f(b.minPt.x, b.maxPt.y, b.minPt.z)));
            out = Union(out, (*this)(Point3f(b.maxPt.x, b.maxPt.y, b.minPt.z)));
            out = Union(out, (*this)(Point3f(b.minPt.x, b.minPt.y, b.maxPt.z)));
            out = Union(out, (*this)(Point3f(b.maxPt.x, b.minPt.y, b.maxPt.z)));
            out = Union(out, (*this)(Point3f(b.minPt.x, b.maxPt.y, b.maxPt.z)));
            out = Union(out, (*this)(Point3f(b.maxPt.x, b.maxPt.y, b.maxPt.z)));
            return out;
        }

        __host__ __device__ Transform operator*(const Transform& other) const {
            float newM[4][4], newMInv[4][4];
            Multiply4x4(m, other.m, newM);
            Multiply4x4(other.mInv, mInv, newMInv);
            return Transform(newM, newMInv);
        }

    private:
        __host__ __device__ static void SetIdentity(float mat[4][4]) {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    mat[i][j] = (i == j) ? 1.0f : 0.0f;
        }

        __host__ __device__ static void Multiply4x4(const float a[4][4], const float b[4][4], float out[4][4]) {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j) {
                    out[i][j] = 0.0f;
                    for (int k = 0; k < 4; ++k)
                        out[i][j] += a[i][k] * b[k][j];
                }
        }

        static void Inverse4x4(const float mat[4][4], float out[4][4]);
    };

}
