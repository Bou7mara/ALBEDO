#pragma once

#include <optix.h>
#include "rt/core/vector3.h"
#include "rt/cam/perspective_camera.h"

namespace rtx {

    struct LaunchParams {
        unsigned int width;
        unsigned int height;
        rt::Vector3f* outputBuffer;
    };

    struct NormalsLaunchParams {
        OptixTraversableHandle iasHandle;
        rt::PerspectiveCamera camera;
        unsigned int width;
        unsigned int height;
        rt::Vector3f* outputBuffer;
    };

}
