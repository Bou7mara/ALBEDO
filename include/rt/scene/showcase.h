#pragma once
#include "rt/scene/scene.h"
#include "rt/cam/perspective_camera.h"

namespace rt {
    // Container holding a complete rendered scene setup including scene geometry, camera, and render settings
    struct ShowcaseSetup {
        Scene scene;              // 3D scene containing shapes and lights
        PerspectiveCamera camera; // Perspective camera configured for the scene
        int imageWidth;           // Rendered image width in pixels
        int imageHeight;          // Rendered image height in pixels
        int samplesPerPixel;      // Number of anti-aliasing rays per pixel (SPP)
        int maxDepth;             // Maximum ray bounce depth for path tracing

        ShowcaseSetup(Scene s, PerspectiveCamera c, int w, int h, int spp, int depth = 50)
            : scene(std::move(s)), camera(std::move(c)), imageWidth(w), imageHeight(h), samplesPerPixel(spp), maxDepth(depth) {}
    };

    // Factory function creating a pre-configured showcase scene with spheres, materials, and lights
    ShowcaseSetup CreateShowcaseScene(int width = 1600, int height = 1000, int spp = 500);
}

