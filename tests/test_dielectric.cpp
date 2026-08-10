#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <numbers>
#include "rt/materials/dielectric.h"

using namespace rt;
using Catch::Approx;

TEST_CASE("Dielectric f() is always zero, matching a delta distribution", "[dielectric]") {
    Dielectric d(1.5f);
    Vector3f result = d.f(Vector3f(0, 1, 0), Vector3f(0, -1, 0), Vector3f(0, 1, 0));
    REQUIRE(result == Vector3f(0.0f, 0.0f, 0.0f));
}

TEST_CASE("Sample_f with u.x near 0 takes the reflection branch at normal incidence", "[dielectric]") {
    Dielectric d(1.5f);
    Vector3f n(0, 1, 0);
    Vector3f wo(0, 1, 0);
    Vector3f wi;
    float pdf;
    Vector3f f = d.Sample_f(wo, n, Point2f(0.0f, 0.5f), &wi, &pdf);

    REQUIRE(wi.y == Approx(1.0f).margin(1e-4));
    REQUIRE(pdf == Approx(0.04f).margin(1e-3));

    float cosTheta = AbsDot(wi, n);
    Vector3f collapsed = f * cosTheta / pdf;
    REQUIRE(collapsed.x == Approx(1.0f).margin(1e-3));
    REQUIRE(collapsed.y == Approx(1.0f).margin(1e-3));
    REQUIRE(collapsed.z == Approx(1.0f).margin(1e-3));
}

TEST_CASE("Sample_f with u.x near 1 takes the transmission branch and applies the eta^2 radiance term", "[dielectric][regression]") {
    Dielectric d(1.5f);
    Vector3f n(0, 1, 0);
    Vector3f wo(0, 1, 0);
    Vector3f wi;
    float pdf;
    Vector3f f = d.Sample_f(wo, n, Point2f(0.99f, 0.5f), &wi, &pdf);

    REQUIRE(wi.y == Approx(-1.0f).margin(1e-3));
    REQUIRE(pdf == Approx(0.96f).margin(1e-3));

    float cosTheta = AbsDot(wi, n);
    Vector3f collapsed = f * cosTheta / pdf;
    float expected = 1.0f / ((1.0f / 1.5f) * (1.0f / 1.5f));
    REQUIRE(collapsed.x == Approx(expected).margin(1e-2));
    REQUIRE(collapsed.y == Approx(expected).margin(1e-2));
    REQUIRE(collapsed.z == Approx(expected).margin(1e-2));
}

TEST_CASE("Total internal reflection forces the reflect branch regardless of u.x", "[dielectric][regression]") {
    Dielectric d(1.5f);
    Vector3f n(0, 1, 0);
    float rad = 60.0f * std::numbers::pi_v<float> / 180.0f;
    Vector3f wo = Normalize(Vector3f(std::sin(rad), -std::cos(rad), 0.0f));
    Vector3f wi;
    float pdf;
    d.Sample_f(wo, n, Point2f(0.99f, 0.5f), &wi, &pdf);

    REQUIRE(pdf == Approx(1.0f).margin(1e-4));
    REQUIRE(Dot(wi, n) == Approx(Dot(wo, n)).margin(1e-4));
}

#include "rt/materials/fresnel.h"

TEST_CASE("Sample_f direction is always unit length", "[dielectric]") {
    Dielectric d(1.5f);
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(0.3f, 0.8f, 0.2f));
    Vector3f wi;
    float pdf;
    for (float ux : {0.0f, 0.5f, 0.99f}) {
        d.Sample_f(wo, n, Point2f(ux, 0.5f), &wi, &pdf);
        REQUIRE(Length(wi) == Approx(1.0f).margin(1e-4));
    }
}

TEST_CASE("CauchyIOR returns reference IOR when B=0 or at reference wavelength", "[dielectric][dispersion]") {
    REQUIRE(CauchyIOR(2.42f, 0.0f, 0.465f) == Approx(2.42f));
    REQUIRE(CauchyIOR(2.42f, 0.0f, 0.630f) == Approx(2.42f));
    REQUIRE(CauchyIOR(2.42f, 0.0131f, kRefWavelengthUm) == Approx(2.42f));
}

TEST_CASE("Dispersion physical ordering: blue bends more than red for B > 0", "[dielectric][dispersion]") {
    float iorR = CauchyIOR(2.42f, 0.0131f, 0.630f);
    float iorG = CauchyIOR(2.42f, 0.0131f, 0.532f);
    float iorB = CauchyIOR(2.42f, 0.0131f, 0.465f);

    REQUIRE(iorB > iorG);
    REQUIRE(iorG > iorR);
}

TEST_CASE("Dispersive Dielectric channel selection is uniformly distributed", "[dielectric][dispersion]") {
    Dielectric d(2.42f, Vector3f(1.0f, 1.0f, 1.0f), 0.0131f);
    Vector3f n(0, 1, 0);
    Vector3f wo(0, 1, 0);
    Vector3f wi;
    float pdf;

    int counts[3] = {0, 0, 0};
    const int N = 3000;
    for (int i = 0; i < N; ++i) {
        float uy = (i + 0.5f) / static_cast<float>(N);
        Vector3f f = d.Sample_f(wo, n, Point2f(0.99f, uy), &wi, &pdf);
        if (f.x > 0.0f) counts[0]++;
        if (f.y > 0.0f) counts[1]++;
        if (f.z > 0.0f) counts[2]++;
    }

    REQUIRE(counts[0] == N / 3);
    REQUIRE(counts[1] == N / 3);
    REQUIRE(counts[2] == N / 3);
}

TEST_CASE("Dispersive Dielectric Monte Carlo average matches non-dispersive value as B -> 0", "[dielectric][dispersion]") {
    Dielectric dNormal(2.42f);
    Dielectric dTinyB(2.42f, Vector3f(1.0f, 1.0f, 1.0f), 1e-6f);

    Vector3f n(0, 1, 0);
    Vector3f wo(0, 1, 0);
    Vector3f wiNormal, wiDispersive;
    float pdfNormal, pdfDispersive;

    Vector3f fNormal = dNormal.Sample_f(wo, n, Point2f(0.99f, 0.5f), &wiNormal, &pdfNormal);
    Vector3f collapsedNormal = fNormal * AbsDot(wiNormal, n) / pdfNormal;

    Vector3f collapsedSum(0.0f, 0.0f, 0.0f);
    const int N = 3000;
    for (int i = 0; i < N; ++i) {
        float uy = (i + 0.5f) / static_cast<float>(N);
        Vector3f fDispersive = dTinyB.Sample_f(wo, n, Point2f(0.99f, uy), &wiDispersive, &pdfDispersive);
        collapsedSum += fDispersive * AbsDot(wiDispersive, n) / pdfDispersive;
    }
    Vector3f collapsedAvg = collapsedSum / static_cast<float>(N);

    REQUIRE(collapsedAvg.x == Approx(collapsedNormal.x).margin(1e-2f));
    REQUIRE(collapsedAvg.y == Approx(collapsedNormal.y).margin(1e-2f));
    REQUIRE(collapsedAvg.z == Approx(collapsedNormal.z).margin(1e-2f));
}

TEST_CASE("Per-channel TIR fallback triggers when shifted IOR causes TIR", "[dielectric][dispersion]") {
    Dielectric dDisp(1.200f, Vector3f(1.0f, 1.0f, 1.0f), 0.08f);

    float iorBlue = CauchyIOR(1.200f, 0.08f, 0.465f);
    REQUIRE(iorBlue > 1.200f);

    float rad = 53.0f * std::numbers::pi_v<float> / 180.0f;
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(std::sin(rad), -std::cos(rad), 0.0f));

    Vector3f wi;
    float pdf;

    Vector3f fBlue = dDisp.Sample_f(wo, n, Point2f(0.99f, 0.99f), &wi, &pdf);

    REQUIRE(fBlue.z > 0.0f);
    REQUIRE(fBlue.x == 0.0f);
    REQUIRE(fBlue.y == 0.0f);
    REQUIRE(Dot(wi, n) == Approx(Dot(wo, n)).margin(1e-3f));
}

TEST_CASE("Dispersive Sample_f direction is always unit length", "[dielectric][dispersion]") {
    Dielectric d(2.42f, Vector3f(1.0f, 1.0f, 1.0f), 0.0131f);
    Vector3f n(0, 1, 0);
    Vector3f wo = Normalize(Vector3f(0.3f, 0.8f, 0.2f));
    Vector3f wi;
    float pdf;
    for (float ux : {0.0f, 0.5f, 0.99f}) {
        for (float uy : {0.1f, 0.5f, 0.9f}) {
            d.Sample_f(wo, n, Point2f(ux, uy), &wi, &pdf);
            REQUIRE(Length(wi) == Approx(1.0f).margin(1e-4f));
        }
    }
}