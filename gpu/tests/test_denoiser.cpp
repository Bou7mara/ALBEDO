#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "gpu_renderer.h"
#include "rt/scene/showcase.h"
#include "rt/io/png_writer.h"
#include <cmath>
#include <vector>
#include <iostream>
#include <numeric>

using Catch::Matchers::WithinAbs;

TEST_CASE("OptiX AI Denoiser reduces variance at low SPP", "[gpu][denoiser]") {
    constexpr int kWidth = 64;
    constexpr int kHeight = 64;
    constexpr int kSpp = 4;

    rt::ShowcaseSetup setup = rt::CreateCornellBoxShowcaseScene(kWidth, kHeight, kSpp);

    std::vector<rt::Vector3f> rawFramebuffer;
    std::vector<rt::Vector3f> denoisedFramebuffer;

    REQUIRE_NOTHROW(rtx::RenderGpu(setup, rawFramebuffer, false));
    REQUIRE_NOTHROW(rtx::RenderGpu(setup, denoisedFramebuffer, true));

    REQUIRE(rawFramebuffer.size() == denoisedFramebuffer.size());

    int startX = 20, endX = 44;
    int startY = 20, endY = 44;
    int patchPixelCount = (endX - startX) * (endY - startY);

    double rawSum = 0.0, rawSqSum = 0.0;
    double denoSsum = 0.0, denoSqSum = 0.0;

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            const auto& rawPix = rawFramebuffer[y * kWidth + x];
            const auto& denoPix = denoisedFramebuffer[y * kWidth + x];

            float rawLuminance = 0.2126f * rawPix.x + 0.7152f * rawPix.y + 0.0722f * rawPix.z;
            float denoLuminance = 0.2126f * denoPix.x + 0.7152f * denoPix.y + 0.0722f * denoPix.z;

            rawSum += rawLuminance;
            rawSqSum += rawLuminance * rawLuminance;

            denoSsum += denoLuminance;
            denoSqSum += denoLuminance * denoLuminance;
        }
    }

    double rawMean = rawSum / patchPixelCount;
    double rawVariance = (rawSqSum / patchPixelCount) - (rawMean * rawMean);

    double denoMean = denoSsum / patchPixelCount;
    double denoVariance = (denoSqSum / patchPixelCount) - (denoMean * denoMean);

    INFO("Raw Mean: " << rawMean << ", Raw Variance: " << rawVariance);
    INFO("Denoised Mean: " << denoMean << ", Denoised Variance: " << denoVariance);

    REQUIRE_THAT(denoMean, WithinAbs(rawMean, 0.2));

    REQUIRE(denoVariance <= rawVariance);

    std::string testDenoisedPng = "test_denoised.png";
    REQUIRE(rt::WritePNG(testDenoisedPng, kWidth, kHeight, denoisedFramebuffer));
}
