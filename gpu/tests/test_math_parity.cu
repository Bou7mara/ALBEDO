#include <catch2/catch_test_macros.hpp>
#include <cuda_runtime.h>
#include <vector>
#include <cmath>

#include "rt/core/tuple2.h"
#include "rt/core/tuple3.h"
#include "rt/core/vector2.h"
#include "rt/core/vector3.h"
#include "rt/core/point2.h"
#include "rt/core/point3.h"
#include "rt/core/normal3.h"
#include "rt/core/bounds3.h"
#include "rt/core/onb.h"
#include "rt/core/quadratic.h"
#include "rt/core/rng.h"
#include "rt/core/transform.h"

using namespace rt;

namespace {

    constexpr float kEpsilon = 1e-5f;

    __host__ __device__ inline bool ApproxEqual(float a, float b, float eps = kEpsilon) {
        float diff = a - b;
        return (diff < 0.0f ? -diff : diff) <= eps;
    }

    __host__ __device__ inline bool ApproxEqual(const Vector3f& a, const Vector3f& b, float eps = kEpsilon) {
        return ApproxEqual(a.x, b.x, eps) && ApproxEqual(a.y, b.y, eps) && ApproxEqual(a.z, b.z, eps);
    }

    __host__ __device__ inline bool ApproxEqual(const Point3f& a, const Point3f& b, float eps = kEpsilon) {
        return ApproxEqual(a.x, b.x, eps) && ApproxEqual(a.y, b.y, eps) && ApproxEqual(a.z, b.z, eps);
    }

    // ==========================================
    // 1. Vector3 & Tuple3 Tests
    // ==========================================
    __host__ __device__ bool TestVector3Math() {
        Vector3f a(1.0f, 2.0f, 3.0f);
        Vector3f b(4.0f, 5.0f, 6.0f);

        Vector3f sum = a + b;
        if (sum.x != 5.0f || sum.y != 7.0f || sum.z != 9.0f) return false;

        Vector3f diff = b - a;
        if (diff.x != 3.0f || diff.y != 3.0f || diff.z != 3.0f) return false;

        Vector3f neg = -a;
        if (neg.x != -1.0f || neg.y != -2.0f || neg.z != -3.0f) return false;

        Vector3f scaled = a * 2.0f;
        if (scaled.x != 2.0f || scaled.y != 4.0f || scaled.z != 6.0f) return false;

        Vector3f scaledLeft = 2.0f * a;
        if (scaledLeft.x != 2.0f || scaledLeft.y != 4.0f || scaledLeft.z != 6.0f) return false;

        Vector3f div = a / 2.0f;
        if (div.x != 0.5f || div.y != 1.0f || div.z != 1.5f) return false;

        float dot = Dot(a, b);
        if (dot != 32.0f) return false;

        Vector3f cross = Cross(Vector3f(1, 0, 0), Vector3f(0, 1, 0));
        if (cross.x != 0.0f || cross.y != 0.0f || cross.z != 1.0f) return false;

        Vector3f unit = Normalize(Vector3f(0.0f, 3.0f, 4.0f));
        if (!ApproxEqual(unit, Vector3f(0.0f, 0.6f, 0.8f))) return false;
        if (!ApproxEqual(Length(unit), 1.0f)) return false;
        if (!ApproxEqual(LengthSquared(unit), 1.0f)) return false;

        Vector3f elemMult = a * b;
        if (elemMult.x != 4.0f || elemMult.y != 10.0f || elemMult.z != 18.0f) return false;

        if (MinComponent(a) != 1.0f || MaxComponent(a) != 3.0f) return false;
        if (MaxDimension(Vector3f(1, 5, 2)) != 1) return false;

        Vector3f perm = Permute(a, 2, 0, 1);
        if (perm.x != 3.0f || perm.y != 1.0f || perm.z != 2.0f) return false;

        return true;
    }

    // ==========================================
    // 2. Vector2 & Tuple2 Tests
    // ==========================================
    __host__ __device__ bool TestVector2Math() {
        Vector2f a(2.0f, 3.0f);
        Vector2f b(4.0f, 5.0f);

        Vector2f sum = a + b;
        if (sum.x != 6.0f || sum.y != 8.0f) return false;

        Vector2f diff = b - a;
        if (diff.x != 2.0f || diff.y != 2.0f) return false;

        float dot = Dot(a, b);
        if (dot != 23.0f) return false;

        Vector2f norm = Normalize(Vector2f(3.0f, 4.0f));
        if (!ApproxEqual(norm.x, 0.6f) || !ApproxEqual(norm.y, 0.8f)) return false;

        return true;
    }

    // ==========================================
    // 3. Point3 & Point2 Tests
    // ==========================================
    __host__ __device__ bool TestPointMath() {
        Point3f p1(1.0f, 2.0f, 3.0f);
        Point3f p2(4.0f, 6.0f, 8.0f);
        Vector3f v(1.0f, 1.0f, 1.0f);

        Point3f pAdd = p1 + v;
        if (pAdd.x != 2.0f || pAdd.y != 3.0f || pAdd.z != 4.0f) return false;

        Vector3f pDiff = p2 - p1;
        if (pDiff.x != 3.0f || pDiff.y != 4.0f || pDiff.z != 5.0f) return false;

        if (DistanceSquared(Point3f(0, 0, 0), Point3f(0, 3, 4)) != 25.0f) return false;
        if (Distance(Point3f(0, 0, 0), Point3f(0, 3, 4)) != 5.0f) return false;

        Point3f lerped = Lerp(0.5f, Point3f(0, 0, 0), Point3f(2, 4, 6));
        if (!ApproxEqual(lerped, Point3f(1, 2, 3))) return false;

        Point2f p2d1(1.0f, 2.0f);
        Point2f p2d2(4.0f, 6.0f);
        if (Distance(p2d1, p2d2) != 5.0f) return false;

        return true;
    }

    // ==========================================
    // 4. Normal3 Tests
    // ==========================================
    __host__ __device__ bool TestNormal3Math() {
        Normal3f n(0.0f, 1.0f, 0.0f);
        Vector3f v(0.0f, -2.0f, 0.0f);

        if (Dot(n, v) != -2.0f) return false;
        if (AbsDot(n, v) != 2.0f) return false;

        Normal3f flipped = FaceForward(n, v);
        if (flipped.y != -1.0f) return false;

        Vector3f converted = static_cast<Vector3f>(n);
        if (converted.x != 0.0f || converted.y != 1.0f || converted.z != 0.0f) return false;

        return true;
    }

    // ==========================================
    // 5. Bounds3 Tests
    // ==========================================
    __host__ __device__ bool TestBounds3Math() {
        Bounds3f b1(Point3f(-1.0f, -1.0f, -1.0f), Point3f(1.0f, 1.0f, 1.0f));
        
        if (b1.SurfaceArea() != 24.0f) return false;
        if (!ApproxEqual(b1.Centroid(), Point3f(0, 0, 0))) return false;
        if (!ApproxEqual(b1.Diagonal(), Vector3f(2, 2, 2))) return false;

        Bounds3f b2(Point3f(0.0f, 0.0f, 0.0f), Point3f(2.0f, 3.0f, 4.0f));
        Bounds3f bUnion = Union(b1, b2);
        if (!ApproxEqual(bUnion.minPt, Point3f(-1, -1, -1)) || !ApproxEqual(bUnion.maxPt, Point3f(2, 3, 4))) return false;

        Ray hitRay(Point3f(0.0f, 0.0f, -5.0f), Vector3f(0.0f, 0.0f, 1.0f));
        float t0 = 0.0f, t1 = 0.0f;
        if (!b1.IntersectP(hitRay, &t0, &t1)) return false;
        if (!ApproxEqual(t0, 4.0f) || !ApproxEqual(t1, 6.0f)) return false;

        Ray missRay(Point3f(5.0f, 5.0f, -5.0f), Vector3f(0.0f, 0.0f, 1.0f));
        if (b1.IntersectP(missRay)) return false;

        return true;
    }

    // ==========================================
    // 6. ONB Tests
    // ==========================================
    __host__ __device__ bool TestONBMath() {
        Vector3f normal = Normalize(Vector3f(0.3f, 0.4f, 0.866f));
        ONB onb(normal);

        if (!ApproxEqual(Length(onb.u), 1.0f) || !ApproxEqual(Length(onb.v), 1.0f) || !ApproxEqual(Length(onb.w), 1.0f)) return false;
        if (!ApproxEqual(Dot(onb.u, onb.v), 0.0f)) return false;
        if (!ApproxEqual(Dot(onb.u, onb.w), 0.0f)) return false;
        if (!ApproxEqual(Dot(onb.v, onb.w), 0.0f)) return false;

        Vector3f localZ(0, 0, 1);
        Vector3f worldZ = onb.ToWorld(localZ);
        if (!ApproxEqual(worldZ, normal)) return false;

        return true;
    }

    // ==========================================
    // 7. Quadratic Tests
    // ==========================================
    __host__ __device__ bool TestQuadraticMath() {
        float t0 = 0.0f, t1 = 0.0f;
        if (!Quadratic(1.0f, -5.0f, 6.0f, &t0, &t1)) return false;
        if (!ApproxEqual(t0, 2.0f) || !ApproxEqual(t1, 3.0f)) return false;

        if (Quadratic(1.0f, 0.0f, 1.0f, &t0, &t1)) return false;

        if (!Quadratic(1.0f, -2.0f, 1.0f, &t0, &t1)) return false;
        if (!ApproxEqual(t0, 1.0f) || !ApproxEqual(t1, 1.0f)) return false;

        return true;
    }

    __host__ __device__ bool TestTransformMath() {
        Transform t = Transform::Translate(Vector3f(2.0f, 3.0f, 4.0f));
        Point3f p(1.0f, 1.0f, 1.0f);
        Point3f pTransformed = t(p);
        if (!ApproxEqual(pTransformed, Point3f(3.0f, 4.0f, 5.0f))) return false;

        Vector3f v(1.0f, 2.0f, 3.0f);
        Vector3f vTransformed = t(v);
        if (!ApproxEqual(vTransformed, v)) return false;

        Transform s = Transform::Scale(2.0f, 3.0f, 4.0f);
        Point3f pScaled = s(p);
        if (!ApproxEqual(pScaled, Point3f(2.0f, 3.0f, 4.0f))) return false;

        Normal3f n(0.0f, 1.0f, 0.0f);
        Normal3f nScaled = s(n);
        if (!ApproxEqual(nScaled.x, 0.0f) || !ApproxEqual(nScaled.y, 1.0f / 3.0f) || !ApproxEqual(nScaled.z, 0.0f)) return false;

        Transform combined = t * s;
        Point3f pCombined = combined(p);
        if (!ApproxEqual(pCombined.x, 4.0f) || !ApproxEqual(pCombined.y, 6.0f) || !ApproxEqual(pCombined.z, 8.0f)) return false;

        return true;
    }

    // ==========================================
    // 9. RNG Determinism Test
    // ==========================================
    __host__ __device__ bool TestRNGDeterminism() {
        RNG rng1(1337);
        RNG rng2(1337);

        for (int i = 0; i < 100; ++i) {
            float val1 = rng1.Uniform1D();
            float val2 = rng2.Uniform1D();
            if (val1 != val2) return false;
            if (val1 < 0.0f || val1 >= 1.0f) return false;

            Point2f p1 = rng1.Uniform2D();
            Point2f p2 = rng2.Uniform2D();
            if (p1.x != p2.x || p1.y != p2.y) return false;
            if (p1.x < 0.0f || p1.x >= 1.0f || p1.y < 0.0f || p1.y >= 1.0f) return false;
        }
        return true;
    }

    constexpr int kNumTestCases = 9;

} // namespace

// ==========================================
// Host-side Catch2 Test Cases
// ==========================================
TEST_CASE("Vector3 arithmetic (host)", "[math][parity]") {
    REQUIRE(TestVector3Math());
}

TEST_CASE("Vector2 arithmetic (host)", "[math][parity]") {
    REQUIRE(TestVector2Math());
}

TEST_CASE("Point arithmetic (host)", "[math][parity]") {
    REQUIRE(TestPointMath());
}

TEST_CASE("Normal3 arithmetic (host)", "[math][parity]") {
    REQUIRE(TestNormal3Math());
}

TEST_CASE("Bounds3 geometry (host)", "[math][parity]") {
    REQUIRE(TestBounds3Math());
}

TEST_CASE("ONB coordinate frame (host)", "[math][parity]") {
    REQUIRE(TestONBMath());
}

TEST_CASE("Quadratic solver (host)", "[math][parity]") {
    REQUIRE(TestQuadraticMath());
}

TEST_CASE("Transform affine operations (host)", "[math][parity][transform]") {
    Transform t = Transform::Translate(Vector3f(2.0f, 3.0f, 4.0f));
    Point3f p(1.0f, 1.0f, 1.0f);
    Point3f pTransformed = t(p);
    REQUIRE(ApproxEqual(pTransformed, Point3f(3.0f, 4.0f, 5.0f)));

    Vector3f v(1.0f, 2.0f, 3.0f);
    Vector3f vTransformed = t(v);
    REQUIRE(ApproxEqual(vTransformed, v));

    Transform s = Transform::Scale(2.0f, 3.0f, 4.0f);
    Point3f pScaled = s(p);
    REQUIRE(ApproxEqual(pScaled, Point3f(2.0f, 3.0f, 4.0f)));

    Normal3f n(0.0f, 1.0f, 0.0f);
    Normal3f nScaled = s(n);
    INFO("n.x=" << n.x << " n.y=" << n.y << " n.z=" << n.z);
    INFO("s.mInv[1][1]=" << s.mInv[1][1]);
    INFO("nScaled.x=" << nScaled.x << " nScaled.y=" << nScaled.y << " nScaled.z=" << nScaled.z);
    REQUIRE(ApproxEqual(nScaled.y, 1.0f / 3.0f));

    Transform combined = t * s;
    Point3f pCombined = combined(p);
    REQUIRE(ApproxEqual(pCombined.x, 4.0f));
    REQUIRE(ApproxEqual(pCombined.y, 6.0f));
    REQUIRE(ApproxEqual(pCombined.z, 8.0f));
}

TEST_CASE("RNG determinism (host)", "[math][parity]") {
    REQUIRE(TestRNGDeterminism());
}

// ==========================================
// Device Kernel & GPU-gated Test Case
// ==========================================
__global__ void RunMathTestsOnDevice(int* results) {
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        results[0] = TestVector3Math() ? 1 : 0;
        results[1] = TestVector2Math() ? 1 : 0;
        results[2] = TestPointMath() ? 1 : 0;
        results[3] = TestNormal3Math() ? 1 : 0;
        results[4] = TestBounds3Math() ? 1 : 0;
        results[5] = TestONBMath() ? 1 : 0;
        results[6] = TestQuadraticMath() ? 1 : 0;
        results[7] = TestTransformMath() ? 1 : 0;
        results[8] = TestRNGDeterminism() ? 1 : 0;
    }
}

TEST_CASE("GPU Device Math Parity Suite", "[gpu][math][parity]") {
    int* d_results = nullptr;
    cudaError_t err = cudaMalloc(&d_results, kNumTestCases * sizeof(int));
    REQUIRE(err == cudaSuccess);

    RunMathTestsOnDevice<<<1, 1>>>(d_results);
    err = cudaDeviceSynchronize();
    REQUIRE(err == cudaSuccess);

    std::vector<int> hostResults(kNumTestCases, 0);
    err = cudaMemcpy(hostResults.data(), d_results, kNumTestCases * sizeof(int), cudaMemcpyDeviceToHost);
    REQUIRE(err == cudaSuccess);

    cudaFree(d_results);

    const char* testNames[kNumTestCases] = {
        "Vector3Math",
        "Vector2Math",
        "PointMath",
        "Normal3Math",
        "Bounds3Math",
        "ONBMath",
        "QuadraticMath",
        "TransformMath",
        "RNGDeterminism"
    };

    for (int i = 0; i < kNumTestCases; ++i) {
        INFO("Checking parity for: " << testNames[i]);
        REQUIRE(hostResults[i] == 1);
    }
}
