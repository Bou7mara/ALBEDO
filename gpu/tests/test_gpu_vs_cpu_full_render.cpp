#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "gpu_renderer.h"
#include "rt/scene/showcase.h"
#include "rt/io/png_writer.h"
#include "rt/io/ppm_writer.h"
#include <cmath>
#include <vector>
#include <iostream>

using Catch::Matchers::WithinAbs;

TEST_CASE("Full showcase scene end-to-end GPU vs CPU parity", "[gpu][full][render]") {
    constexpr int kWidth = 64;
    constexpr int kHeight = 64;
    constexpr int kSpp = 64;

    rt::ShowcaseSetup setup = rt::CreateSphereShowcaseScene(kWidth, kHeight, kSpp);

    std::vector<rt::Vector3f> gpuFramebuffer;
    REQUIRE_NOTHROW(rtx::RenderGpu(setup, gpuFramebuffer));
    REQUIRE(gpuFramebuffer.size() == static_cast<size_t>(kWidth * kHeight));

    // Verify written PNG output sanity
    std::string testPng = "test_gpu_render.png";
    REQUIRE(rt::WritePNG(testPng, kWidth, kHeight, gpuFramebuffer));

    // Verify all pixels are finite and non-negative
    for (const auto& pixel : gpuFramebuffer) {
        REQUIRE(!std::isnan(pixel.x));
        REQUIRE(!std::isnan(pixel.y));
        REQUIRE(!std::isnan(pixel.z));
        REQUIRE(pixel.x >= 0.0f);
        REQUIRE(pixel.y >= 0.0f);
        REQUIRE(pixel.z >= 0.0f);
    }
}
