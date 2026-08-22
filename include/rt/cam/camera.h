#pragma once
#include "rt/core/point2.h"
#include "rt/core/ray.h"

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

struct CameraSample {
    Point2f pFilm;
};

class Camera {
public:
    __host__ __device__ virtual ~Camera() = default;

    __host__ __device__ virtual Ray GenerateRay(const CameraSample& sample) const = 0;
};

} // namespace rt
