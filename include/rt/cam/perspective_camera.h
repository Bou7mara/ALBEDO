#pragma once
#include "rt/cam/camera.h"
#include "rt/core/transform.h"

namespace rt {

class PerspectiveCamera : public Camera {
public:
    PerspectiveCamera(const Point3f& eye, const Point3f& lookAt, const Vector3f& up,
                       float fovYDegrees, int imageWidth, int imageHeight);

    Ray GenerateRay(const CameraSample& sample) const override;

private:
    Transform cameraToWorld_;
    int imageWidth_;
    int imageHeight_;
    float halfHeight_;
    float halfWidth_;
};

}
