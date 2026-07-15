#pragma once
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/normal3.h"
#include "rt/core/ray.h"
#include <cmath>
#include <cstring>

namespace rt {
    class Transform {
    public:
        float m[4][4];
        float mInv[4][4];

        Transform() {
            SetIdentity(m);
            SetIdentity(mInv);
        }

        explicit Transform(const float mat[4][4]) {
            std::memcpy(m, mat, sizeof(m));
            Inverse4x4(m, mInv);
        }

        Transform(const float mat[4][4], const float matInv[4][4]) {
            std::memcpy(m, mat, sizeof(m));
            std::memcpy(mInv, matInv, sizeof(mInv));
        }

        static Transform Identity() { return Transform(); }

        static Transform Translate(const Vector3f& delta) {
            float mat[4][4] = {
                {1,0,0, delta.x},
                {0,1,0, delta.y},
                {0,0,1, delta.z},
                {0,0,0, 1}
            };
            float inv[4][4] = {
                {1,0,0,-delta.x},
                {0,1,0,-delta.y},
                {0,0,1,-delta.z},
                {0,0,0, 1}
            };
            return Transform(mat, inv);
        }

        static Transform Scale(float sx, float sy, float sz) {
            float mat[4][4] = {
                {sx,0,0,0}, {0,sy,0,0}, {0,0,sz,0}, {0,0,0,1}
            };
            float inv[4][4] = {
                {1/sx,0,0,0}, {0,1/sy,0,0}, {0,0,1/sz,0}, {0,0,0,1}
            };
            return Transform(mat, inv);
        }

        static Transform RotateX(float thetaDeg);
        static Transform RotateY(float thetaDeg);
        static Transform RotateZ(float thetaDeg);

        static Transform LookAt(const Point3f& eye, const Point3f& look, const Vector3f& up);

        Transform Inverse() const {
            return Transform(mInv, m);
        }

        bool SwapsHandedness() const {
            float det =
                m[0][0] * (m[1][1]*m[2][2] - m[1][2]*m[2][1]) -
                m[0][1] * (m[1][0]*m[2][2] - m[1][2]*m[2][0]) +
                m[0][2] * (m[1][0]*m[2][1] - m[1][1]*m[2][0]);
            return det < 0.0f;
        }

        Point3f operator()(const Point3f& p) const {
            float x = p.x, y = p.y, z = p.z;
            float xp = m[0][0]*x + m[0][1]*y + m[0][2]*z + m[0][3];
            float yp = m[1][0]*x + m[1][1]*y + m[1][2]*z + m[1][3];
            float zp = m[2][0]*x + m[2][1]*y + m[2][2]*z + m[2][3];
            float wp = m[3][0]*x + m[3][1]*y + m[3][2]*z + m[3][3];
            if (wp == 1.0f) return Point3f(xp, yp, zp);
            return Point3f(xp, yp, zp) / wp;
        }

        Vector3f operator()(const Vector3f& v) const {
            float x = v.x, y = v.y, z = v.z;
            return Vector3f(
                m[0][0]*x + m[0][1]*y + m[0][2]*z,
                m[1][0]*x + m[1][1]*y + m[1][2]*z,
                m[2][0]*x + m[2][1]*y + m[2][2]*z
            );
        }

        Normal3f operator()(const Normal3f& n) const {
            float x = n.x, y = n.y, z = n.z;
            return Normal3f(
                mInv[0][0]*x + mInv[1][0]*y + mInv[2][0]*z,
                mInv[0][1]*x + mInv[1][1]*y + mInv[2][1]*z,
                mInv[0][2]*x + mInv[1][2]*y + mInv[2][2]*z
            );
        }

        Ray operator()(const Ray& r) const {
            Point3f newO = (*this)(r.o);
            Vector3f newD = (*this)(r.d);
            return Ray(newO, newD, r.tMax, r.time);
        }

        Transform operator*(const Transform& other) const {
            float newM[4][4], newMInv[4][4];
            Multiply4x4(m, other.m, newM);
            Multiply4x4(other.mInv, mInv, newMInv);   // reversed order
            return Transform(newM, newMInv);
        }

    private:
        static void SetIdentity(float mat[4][4]) {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    mat[i][j] = (i == j) ? 1.0f : 0.0f;
        }

        static void Multiply4x4(const float a[4][4], const float b[4][4], float out[4][4]) {
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
