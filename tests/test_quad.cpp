#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "rt/shapes/quad.h"
#include "rt/lights/diffuse_area_light.h"
#include "rt/materials/lambertian.h"
#include "rt/core/rng.h"
#include <vector>
#include <cmath>

using namespace rt;
using Catch::Approx;

TEST_CASE("Quad - Center Ray Intersection", "[quad]") {
    // 2x2 XY quad at z = 5
    Point3f p0(-1.0f, -1.0f, 5.0f);
    Vector3f e1(2.0f, 0.0f, 0.0f);
    Vector3f e2(0.0f, 2.0f, 0.0f);
    Quad q(p0, e1, e2);

    REQUIRE(q.Area() == Approx(4.0f));

    // Ray straight at center
    Ray ray(Point3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 1.0f));
    SurfaceInteraction isect;

    REQUIRE(q.Intersect(ray, &isect));
    REQUIRE(isect.t == Approx(5.0f));
    REQUIRE(isect.p.x == Approx(0.0f));
    REQUIRE(isect.p.y == Approx(0.0f));
    REQUIRE(isect.p.z == Approx(5.0f));
    REQUIRE(isect.uv.x == Approx(0.5f));
    REQUIRE(isect.uv.y == Approx(0.5f));
    REQUIRE(isect.shape == &q);
}

TEST_CASE("Quad - Boundary and Edge Hits", "[quad]") {
    Point3f p0(0.0f, 0.0f, 0.0f);
    Vector3f e1(10.0f, 0.0f, 0.0f);
    Vector3f e2(0.0f, 10.0f, 0.0f);
    Quad q(p0, e1, e2);

    SurfaceInteraction isect;

    // Corner u=0, v=0
    Ray r0(Point3f(0.0f, 0.0f, 5.0f), Vector3f(0.0f, 0.0f, -1.0f));
    REQUIRE(q.Intersect(r0, &isect));
    REQUIRE(isect.uv.x == Approx(0.0f).margin(1e-4f));
    REQUIRE(isect.uv.y == Approx(0.0f).margin(1e-4f));

    // Corner u=1, v=1
    Ray r1(Point3f(10.0f, 10.0f, 5.0f), Vector3f(0.0f, 0.0f, -1.0f));
    REQUIRE(q.Intersect(r1, &isect));
    REQUIRE(isect.uv.x == Approx(1.0f).margin(1e-4f));
    REQUIRE(isect.uv.y == Approx(1.0f).margin(1e-4f));
}

TEST_CASE("Quad - Miss Scenarios", "[quad]") {
    Point3f p0(0.0f, 0.0f, 0.0f);
    Vector3f e1(2.0f, 0.0f, 0.0f);
    Vector3f e2(0.0f, 2.0f, 0.0f);
    Quad q(p0, e1, e2);

    SurfaceInteraction isect;

    // Parallel ray
    Ray rParallel(Point3f(1.0f, 1.0f, 5.0f), Vector3f(1.0f, 0.0f, 0.0f));
    REQUIRE_FALSE(q.Intersect(rParallel, &isect));

    // Plane hit outside [0,1]^2
    Ray rOutside(Point3f(5.0f, 5.0f, 5.0f), Vector3f(0.0f, 0.0f, -1.0f));
    REQUIRE_FALSE(q.Intersect(rOutside, &isect));

    // Hit behind ray origin
    Ray rBehind(Point3f(1.0f, 1.0f, -5.0f), Vector3f(0.0f, 0.0f, -1.0f));
    REQUIRE_FALSE(q.Intersect(rBehind, &isect));
}

TEST_CASE("Quad - Degenerate Construction Guards", "[quad]") {
    Point3f p0(0.0f, 0.0f, 0.0f);
    Vector3f zeroE(0.0f, 0.0f, 0.0f);
    Vector3f e1(1.0f, 0.0f, 0.0f);
    Vector3f parallelE(2.0f, 0.0f, 0.0f);

    // Zero edge
    REQUIRE_THROWS_AS(Quad(p0, e1, zeroE), std::invalid_argument);

    // Parallel edges
    REQUIRE_THROWS_AS(Quad(p0, e1, parallelE), std::invalid_argument);
}

TEST_CASE("Quad - Uniform Area Sampling Distribution", "[quad]") {
    Point3f p0(0.0f, 0.0f, 0.0f);
    Vector3f e1(4.0f, 0.0f, 0.0f);
    Vector3f e2(0.0f, 4.0f, 0.0f);
    Quad q(p0, e1, e2);

    const int N = 10000;
    const int binsX = 10;
    const int binsY = 10;
    std::vector<int> counts(binsX * binsY, 0);

    RNG rng(42);
    for (int i = 0; i < N; ++i) {
        Point2f u(rng.Uniform1D(), rng.Uniform1D());
        ShapeSample s = q.Sample(u);
        int bx = std::clamp(static_cast<int>(u.x * binsX), 0, binsX - 1);
        int by = std::clamp(static_cast<int>(u.y * binsY), 0, binsY - 1);
        counts[by * binsX + bx]++;
    }

    float expectedPerBin = static_cast<float>(N) / static_cast<float>(binsX * binsY); // 100
    for (int c : counts) {
        REQUIRE(static_cast<float>(c) == Approx(expectedPerBin).margin(35.0f));
    }
}

TEST_CASE("Quad - Sample and Pdf Consistency", "[quad]") {
    Point3f p0(-2.0f, -2.0f, 5.0f);
    Vector3f e1(4.0f, 0.0f, 0.0f);
    Vector3f e2(0.0f, 4.0f, 0.0f);
    Quad q(p0, e1, e2);

    Point3f ref(0.0f, 0.0f, 0.0f);
    RNG rng(12345);

    const int N = 10000;
    double pdfIntegral = 0.0;

    for (int i = 0; i < N; ++i) {
        Point2f u(rng.Uniform1D(), rng.Uniform1D());
        ShapeSample s = q.Sample(ref, u);
        REQUIRE(s.pdf > 0.0f);

        Vector3f wi = Normalize(s.p - ref);
        float pdfEval = q.Pdf(ref, wi);

        REQUIRE(pdfEval == Approx(s.pdf).margin(1e-3f));
        pdfIntegral += (1.0 / s.pdf);
    }

    // Monte Carlo integration of 1 over solid angle subtended by area-sampled quad:
    // E[1/pdf_solidAngle] = \int_{\Omega} 1 d\omega = \text{subtended solid angle}
    // Subtended solid angle of 4x4 quad at distance 5 is ~0.55 rad
    double avgSolidAngle = pdfIntegral / N;
    REQUIRE(avgSolidAngle > 0.4);
    REQUIRE(avgSolidAngle < 0.7);
}

TEST_CASE("Quad - DiffuseAreaLight Integration", "[quad]") {
    Point3f p0(-1.0f, -1.0f, 3.0f);
    Vector3f e1(2.0f, 0.0f, 0.0f);
    Vector3f e2(0.0f, 2.0f, 0.0f);
    auto emissiveBSDF = std::make_shared<Lambertian>(Vector3f(0, 0, 0)); // non-zero emission tested via bsdf Le
    auto quadShape = std::make_shared<Quad>(p0, e1, e2, emissiveBSDF);

    DiffuseAreaLight light(quadShape);
    REQUIRE(light.GetShape() == quadShape.get());

    Point3f ref(0.0f, 0.0f, 0.0f);
    Point2f u(0.5f, 0.5f);
    Light::LiSample sample = light.Sample_Li(ref, u);

    REQUIRE(sample.pdf > 0.0f);
    REQUIRE(sample.dist == Approx(3.0f).margin(1e-3f));
    REQUIRE(sample.wi.z == Approx(1.0f).margin(1e-3f));

    float pdfLi = light.Pdf_Li(ref, sample.wi);
    REQUIRE(pdfLi == Approx(sample.pdf).margin(1e-3f));
}
