#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "rt/materials/sellmeier.h"
#include "rt/spectral/spectrum.h"
#include "rt/materials/dielectric.h"
#include "rt/core/rng.h"
#include <cmath>
#include <vector>

using namespace rt;
using Catch::Approx;

TEST_CASE("Sellmeier equation accuracy against optical reference values", "[spectral][sellmeier]") {
    // 589.3 nm is Sodium D-line, standard reference for IOR
    constexpr float kSodiumDLineUm = 0.5893f;

    // 1. Diamond: n ~ 2.417
    float nDiamond = SellmeierIOR(kDiamondSellmeier, kSodiumDLineUm);
    REQUIRE(nDiamond == Approx(2.417f).margin(0.002f));

    // 2. Schott BK7 Optical Glass: n ~ 1.5168
    float nBK7 = SellmeierIOR(kBK7Sellmeier, kSodiumDLineUm);
    REQUIRE(nBK7 == Approx(1.5168f).margin(0.002f));

    // 3. Sapphire (ordinary ray): n ~ 1.768
    float nSapphire = SellmeierIOR(kSapphireSellmeier, kSodiumDLineUm);
    REQUIRE(nSapphire == Approx(1.768f).margin(0.003f));

    // 4. Physical dispersion ordering: blue (450 nm) > green (550 nm) > red (650 nm)
    float nBlue = SellmeierIOR(kDiamondSellmeier, 0.450f);
    float nGreen = SellmeierIOR(kDiamondSellmeier, 0.550f);
    float nRed = SellmeierIOR(kDiamondSellmeier, 0.650f);

    REQUIRE(nBlue > nGreen);
    REQUIRE(nGreen > nRed);
    REQUIRE(nBlue - nRed > 0.034f); // Diamond has substantial dispersion (Abbe ~ 55)
}

TEST_CASE("Wyman CIE 1931 fit and SpectralToRgb white point integration", "[spectral][wyman]") {
    // Flat unit spectral radiance (1.0 across all wavelengths) should integrate to linear sRGB (1.0, 1.0, 1.0)
    constexpr int N = 200;
    Vector3f accumulatedRgb(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < N; ++i) {
        float u = (static_cast<float>(i) + 0.5f) / static_cast<float>(N);
        HeroWavelengths hw = SampleHeroWavelengths(u);
        float spectralL[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        accumulatedRgb += SpectralToRgb(hw, spectralL);
    }
    Vector3f meanRgb = accumulatedRgb / static_cast<float>(N);

    REQUIRE(meanRgb.x == Approx(1.0f).margin(0.02f));
    REQUIRE(meanRgb.y == Approx(1.0f).margin(0.02f));
    REQUIRE(meanRgb.z == Approx(1.0f).margin(0.02f));
}

TEST_CASE("Hero wavelength stratification across full visible spectrum", "[spectral][hero]") {
    RNG rng(42);
    for (int iter = 0; iter < 100; ++iter) {
        float u = rng.Uniform1D();
        HeroWavelengths hw = SampleHeroWavelengths(u);

        for (int i = 0; i < 4; ++i) {
            REQUIRE(hw.lambda[i] >= kLambdaMin);
            REQUIRE(hw.lambda[i] <= kLambdaMax);
        }

        // Spacing between companion wavelengths is exactly (730 - 380) / 4 = 87.5 nm modulo 350
        float d1 = std::abs(hw.lambda[1] - hw.lambda[0]);
        float d2 = std::abs(hw.lambda[2] - hw.lambda[1]);
        float d3 = std::abs(hw.lambda[3] - hw.lambda[2]);

        auto ModDist = [](float d) { return (d > 175.0f) ? (350.0f - d) : d; };
        REQUIRE(ModDist(d1) == Approx(87.5f).margin(1e-4f));
        REQUIRE(ModDist(d2) == Approx(87.5f).margin(1e-4f));
        REQUIRE(ModDist(d3) == Approx(87.5f).margin(1e-4f));
    }
}

TEST_CASE("RGB-to-Spectrum upsampling partition of unity", "[spectral][upsampling]") {
    // For any constant gray level c, RgbToSpectrum(Vector3f(c, c, c), lambda) == c for all lambda
    for (float c : { 0.0f, 0.25f, 0.5f, 0.73f, 1.0f }) {
        Vector3f gray(c, c, c);
        for (float lambda = 380.0f; lambda <= 730.0f; lambda += 10.0f) {
            float s = RgbToSpectrum(gray, lambda);
            REQUIRE(s == Approx(c).margin(1e-5f));
        }
    }
}

TEST_CASE("Achromatic (zero-dispersion) Dielectric Hero-Wavelength Parity", "[spectral][achromatic]") {
    Dielectric dNonSpectral(1.5f);
    Dielectric dSpectral(SellmeierCoefficients::MakeConstant(1.5f));

    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo = Normalize(Vector3f(0.3f, 0.8f, 0.2f));

    RNG rng(1234);
    constexpr int N = 1000;

    for (int i = 0; i < N; ++i) {
        Point2f u = rng.Uniform2D();
        float uHero = rng.Uniform1D();
        HeroWavelengths hw = SampleHeroWavelengths(uHero);

        Vector3f wiNonSpectral;
        float pdfNonSpectral;
        Vector3f fNonSpectral = dNonSpectral.Sample_f(wo, n, u, &wiNonSpectral, &pdfNonSpectral);

        Vector3f wiSpectral;
        float pdfHero;
        float weights[4];
        bool ok = dSpectral.Sample_HeroWavelengths(wo, n, u, hw, &wiSpectral, &pdfHero, weights);

        REQUIRE(ok);
        REQUIRE(wiSpectral.x == Approx(wiNonSpectral.x).margin(1e-4f));
        REQUIRE(wiSpectral.y == Approx(wiNonSpectral.y).margin(1e-4f));
        REQUIRE(wiSpectral.z == Approx(wiNonSpectral.z).margin(1e-4f));
        REQUIRE(pdfHero == Approx(pdfNonSpectral).margin(1e-4f));

        // For zero dispersion, all 4 wavelengths receive the exact same throughput weight
        for (int k = 0; k < 4; ++k) {
            REQUIRE(weights[k] == Approx(weights[0]).margin(1e-4f));
        }
    }
}

TEST_CASE("Hero-wavelength sampling reduces spectral variance over single-wavelength sampling", "[spectral][variance]") {
    Dielectric diamond(kDiamondSellmeier);

    Vector3f n(0.0f, 1.0f, 0.0f);
    Vector3f wo = Normalize(Vector3f(0.85f, -0.25f, 0.0f));

    constexpr int kPaths = 3000;

    // 1. Single wavelength per path: 1 random wavelength converted to RGB
    RNG rngSingle(54321);
    double singleVarSum = 0.0;
    Vector3f singleMean(0.0f, 0.0f, 0.0f);
    std::vector<Vector3f> singleColors(kPaths);

    for (int i = 0; i < kPaths; ++i) {
        float uHero = rngSingle.Uniform1D();
        HeroWavelengths hw = SampleHeroWavelengths(uHero);
        Point2f u = rngSingle.Uniform2D();

        Vector3f wi;
        float pdfHero;
        float weights[4];
        diamond.Sample_HeroWavelengths(wo, n, u, hw, &wi, &pdfHero, weights);

        // Single wavelength estimate: only 1 wavelength sample evaluated
        float factor = kLambdaRange / kCieYIntegral;
        Vector3f xyz(CieX(hw.lambda[0]) * weights[0] * factor,
                     CieY(hw.lambda[0]) * weights[0] * factor,
                     CieZ(hw.lambda[0]) * weights[0] * factor);
        Vector3f rgb = XyzToSrgb(xyz);
        singleColors[i] = rgb;
        singleMean += rgb;
    }
    singleMean = singleMean / static_cast<float>(kPaths);
    for (int i = 0; i < kPaths; ++i) {
        Vector3f diff = singleColors[i] - singleMean;
        singleVarSum += Dot(diff, diff);
    }
    double singleVar = singleVarSum / kPaths;

    // 2. 4-Hero-Wavelength sampling: 4 stratified hero wavelengths converted to RGB
    RNG rngHero(54321);
    double heroVarSum = 0.0;
    Vector3f heroMean(0.0f, 0.0f, 0.0f);
    std::vector<Vector3f> heroColors(kPaths);

    for (int i = 0; i < kPaths; ++i) {
        float uHero = rngHero.Uniform1D();
        HeroWavelengths hw = SampleHeroWavelengths(uHero);
        Point2f u = rngHero.Uniform2D();

        Vector3f wi;
        float pdfHero;
        float weights[4];
        diamond.Sample_HeroWavelengths(wo, n, u, hw, &wi, &pdfHero, weights);

        // 4-Hero stratified estimate
        Vector3f rgb = SpectralToRgb(hw, weights);
        heroColors[i] = rgb;
        heroMean += rgb;
    }
    heroMean = heroMean / static_cast<float>(kPaths);
    for (int i = 0; i < kPaths; ++i) {
        Vector3f diff = heroColors[i] - heroMean;
        heroVarSum += Dot(diff, diff);
    }
    double heroVar = heroVarSum / kPaths;

    INFO("Single Wavelength RGB Variance: " << singleVar << ", Hero Wavelength RGB Variance: " << heroVar);
    REQUIRE(heroVar < singleVar);
    REQUIRE(heroMean.x == Approx(singleMean.x).margin(0.1f));
    REQUIRE(heroMean.y == Approx(singleMean.y).margin(0.1f));
    REQUIRE(heroMean.z == Approx(singleMean.z).margin(0.1f));
}
