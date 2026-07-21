#pragma once
#include "rt/scene/scene.h"
#include "rt/cam/perspective_camera.h"

namespace rt {
    struct ShowcaseSetup {
        Scene scene;
        PerspectiveCamera camera;
        int imageWidth;
        int imageHeight;
        int samplesPerPixel;
        int maxDepth;

        ShowcaseSetup(Scene s, PerspectiveCamera c, int w, int h, int spp, int depth = 50)
            : scene(std::move(s)), camera(std::move(c)), imageWidth(w), imageHeight(h), samplesPerPixel(spp), maxDepth(depth) {}
    };

    ShowcaseSetup CreateShowcaseScene(int width = 1600, int height = 1000, int spp = 500);
}
