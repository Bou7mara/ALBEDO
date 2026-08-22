#include <optix.h>
#include "launch_params.h"
#include "rt/core/vector3.h"

extern "C" {
    __constant__ rtx::LaunchParams params;
}

extern "C" __global__ void __raygen__smoke_test() {
    const uint3 idx = optixGetLaunchIndex();
    
    if (idx.x >= params.width || idx.y >= params.height) {
        return;
    }

    const float u = (float)idx.x / (float)(params.width > 1 ? params.width - 1 : 1);
    const float v = (float)idx.y / (float)(params.height > 1 ? params.height - 1 : 1);
    const unsigned int pixel = idx.y * params.width + idx.x;

    params.outputBuffer[pixel] = rt::Vector3f(u, v, 0.2f);
}
