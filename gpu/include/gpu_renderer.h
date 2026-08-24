#pragma once

#include "rt/scene/showcase.h"
#include "rt/core/vector3.h"
#include <vector>

namespace rtx {

    void RenderGpu(const rt::ShowcaseSetup& setup, std::vector<rt::Vector3f>& framebuffer, bool denoise = false);

}
