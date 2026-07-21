#pragma once
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/normal3.h"
#include "rt/core/ray.h"
#include <cmath>
#include <cstring>

// Ray Tracer Core Namespace
namespace rt {
    // ===========================================
    // 4X4 HOMOGENEOUS TRANSFORMATION MATRIX CLASS
    // ===========================================

    // Encapsulates 3D spatial transformations (translation, rotation, scale, camera view).
    // Maintains both forward matrix 'm' and inverse matrix 'mInv' in tandem!
    class Transform {
    public:
        // --- Data Members ---
        float m[4][4];    // Forward 4x4 transformation matrix
        float mInv[4][4]; // Cached Inverse 4x4 matrix (prevents runtime matrix inversion bottleneck!)

        // ------------
        // CONSTRUCTORS
        // ------------

        // Default constructor: Identity transformation (no movement/rotation/scaling)
        Transform() {
            SetIdentity(m);
            SetIdentity(mInv);
        }

        // Construct from forward matrix mat; computes inverse automatically
        explicit Transform(const float mat[4][4]) {
            std::memcpy(m, mat, sizeof(m));
            Inverse4x4(m, mInv);
        }

        // Construct directly with both forward matrix and known inverse matrix (fast path!)
        Transform(const float mat[4][4], const float matInv[4][4]) {
            std::memcpy(m, mat, sizeof(m));
            std::memcpy(mInv, matInv, sizeof(mInv));
        }

        // ------------------------
        // FACTORY CREATION METHODS
        // ------------------------

        // Factory: Returns Identity transform
        static Transform Identity() { return Transform(); }

        // Factory: Translation transformation (moves origin by delta)
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

        // Factory: Non-uniform scale transformation
        static Transform Scale(float sx, float sy, float sz) {
            float mat[4][4] = {
                {sx,0,0,0}, {0,sy,0,0}, {0,0,sz,0}, {0,0,0,1}
            };
            float inv[4][4] = {
                {1/sx,0,0,0}, {0,1/sy,0,0}, {0,0,1/sz,0}, {0,0,0,1}
            };
            return Transform(mat, inv);
        }

        // Rotation factories around primary axes (angles in degrees)
        static Transform RotateX(float thetaDeg);
        static Transform RotateY(float thetaDeg);
        static Transform RotateZ(float thetaDeg);

        // Factory: Constructs camera LookAt view transform (eye point, target point, up vector)
        static Transform LookAt(const Point3f& eye, const Point3f& look, const Vector3f& up);

        // ----------------------
        // TRANSFORMATION QUERIES
        // ----------------------

        // Swaps forward and inverse matrices in O(1) time without recomputing inverse!
        Transform Inverse() const {
            return Transform(mInv, m);
        }

        // Checks if matrix determinant < 0 (i.e. transformation switches right-handed system to left-handed).
        // Essential for correcting triangle vertex order and surface normal orientation after negative scaling!
        bool SwapsHandedness() const {
            float det =
                m[0][0] * (m[1][1]*m[2][2] - m[1][2]*m[2][1]) -
                m[0][1] * (m[1][0]*m[2][2] - m[1][2]*m[2][0]) +
                m[0][2] * (m[1][0]*m[2][1] - m[1][1]*m[2][0]);
            return det < 0.0f;
        }

        // ----------------------------------------
        // OPERATOR OVERLOADS (APPLYING TRANSFORMS)
        // ----------------------------------------

        // Transforms a 3D Point (affected by translation, rotation, scaling, and perspective divide)
        Point3f operator()(const Point3f& p) const {
            float x = p.x, y = p.y, z = p.z;
            float xp = m[0][0]*x + m[0][1]*y + m[0][2]*z + m[0][3];
            float yp = m[1][0]*x + m[1][1]*y + m[1][2]*z + m[1][3];
            float zp = m[2][0]*x + m[2][1]*y + m[2][2]*z + m[2][3];
            
            // Homogeneous w coordinate (1.0 for affine, != 1.0 for perspective projection)
            float wp = m[3][0]*x + m[3][1]*y + m[3][2]*z + m[3][3];
            if (wp == 1.0f) return Point3f(xp, yp, zp);
            return Point3f(xp, yp, zp) / wp; // Perspective divide!
        }

        // Transforms a 3D Vector (affected by rotation and scale, BUT NOT translation!)
        Vector3f operator()(const Vector3f& v) const {
            float x = v.x, y = v.y, z = v.z;
            return Vector3f(
                m[0][0]*x + m[0][1]*y + m[0][2]*z,
                m[1][0]*x + m[1][1]*y + m[1][2]*z,
                m[2][0]*x + m[2][1]*y + m[2][2]*z
            );
        }

        // Transforms a Surface Normal3f (Multiplied by transposed Inverse Matrix (mInv)^T!)
        // Notice transposed array lookup: mInv[col][row] instead of m[row][col]!
        Normal3f operator()(const Normal3f& n) const {
            float x = n.x, y = n.y, z = n.z;
            return Normal3f(
                mInv[0][0]*x + mInv[1][0]*y + mInv[2][0]*z,
                mInv[0][1]*x + mInv[1][1]*y + mInv[2][1]*z,
                mInv[0][2]*x + mInv[1][2]*y + mInv[2][2]*z
            );
        }

        // Transforms a Ray (transforms both origin point and direction vector)
        Ray operator()(const Ray& r) const {
            Point3f newO = (*this)(r.o);
            Vector3f newD = (*this)(r.d);
            return Ray(newO, newD, r.tMax, r.time);
        }

        // Transform composition: (T1 * T2)(p) = T1(T2(p)). Note reverse inverse multiplication!
        Transform operator*(const Transform& other) const {
            float newM[4][4], newMInv[4][4];
            Multiply4x4(m, other.m, newM);
            Multiply4x4(other.mInv, mInv, newMInv); // (A * B)^-1 = B^-1 * A^-1
            return Transform(newM, newMInv);
        }

    private:
        // Internal helper: Sets 4x4 matrix to identity
        static void SetIdentity(float mat[4][4]) {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    mat[i][j] = (i == j) ? 1.0f : 0.0f;
        }

        // Internal helper: Standard 4x4 matrix multiplication
        static void Multiply4x4(const float a[4][4], const float b[4][4], float out[4][4]) {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j) {
                    out[i][j] = 0.0f;
                    for (int k = 0; k < 4; ++k)
                        out[i][j] += a[i][k] * b[k][j];
                }
        }

        // Internal helper: Solves 4x4 matrix inversion using Gauss-Jordan elimination
        static void Inverse4x4(const float mat[4][4], float out[4][4]);
    };
}
