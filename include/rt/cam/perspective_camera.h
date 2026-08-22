#pragma once
#include "rt/cam/camera.h"
#include "rt/core/transform.h"
#include "rt/core/math_utils.h"
#include <cmath>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

class PerspectiveCamera : public Camera {
public:
    constexpr __host__ __device__ PerspectiveCamera() = default;

    PerspectiveCamera(const Point3f& eye, const Point3f& lookAt, const Vector3f& up,
                      float fovYDegrees, int imageWidth, int imageHeight);

    __host__ __device__ Ray GenerateRay(const CameraSample& sample) const override {
        float ndcX = sample.pFilm.x / static_cast<float>(imageWidth_);
        float ndcY = sample.pFilm.y / static_cast<float>(imageHeight_);

        float screenX = (2.0f * ndcX - 1.0f) * halfWidth_;
        float screenY = (1.0f - 2.0f * ndcY) * halfHeight_;

        Vector3f dirCamera = Normalize(Vector3f(screenX, screenY, 1.0f));
        Ray rayCamera(Point3f(0.0f, 0.0f, 0.0f), dirCamera);

        return cameraToWorld_(rayCamera);
    }

    __host__ __device__ const Transform& CameraToWorld() const { return cameraToWorld_; }
    __host__ __device__ int ImageWidth() const { return imageWidth_; }
    __host__ __device__ int ImageHeight() const { return imageHeight_; }
    __host__ __device__ float HalfWidth() const { return halfWidth_; }
    __host__ __device__ float HalfHeight() const { return halfHeight_; }

private:
    Transform cameraToWorld_;
    int imageWidth_ = 0;
    int imageHeight_ = 0;
    float halfHeight_ = 0.0f;
    float halfWidth_ = 0.0f;
};

} // namespace rt
