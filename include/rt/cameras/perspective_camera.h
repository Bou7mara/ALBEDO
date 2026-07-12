#ifndef RT_CAMERAS_PERSPECTIVE_CAMERA_H
#define RT_CAMERAS_PERSPECTIVE_CAMERA_H

#include "rt/cameras/camera.h"
#include "rt/core/transform.h"

namespace rt {

class PerspectiveCamera : public Camera {
public:
    // eye/lookAt/up define camera placement in world space, exactly as
    // consumed by Transform::LookAt. fovYDegrees is the full vertical
    // field of view. imageWidth/imageHeight determine aspect ratio and
    // convert continuous raster coordinates to normalized screen
    // coordinates.
    PerspectiveCamera(const Point3f& eye, const Point3f& lookAt, const Vector3f& up,
                       float fovYDegrees, int imageWidth, int imageHeight);

    Ray GenerateRay(const CameraSample& sample) const override;

private:
    Transform cameraToWorld_;
    int imageWidth_;
    int imageHeight_;
    float halfHeight_;   // half-extent of the image plane at z=1, in camera space
    float halfWidth_;    // derived once at construction
};

} // namespace rt

#endif // RT_CAMERAS_PERSPECTIVE_CAMERA_H
