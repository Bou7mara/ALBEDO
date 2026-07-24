#pragma once
#include "rt/cam/camera.h"
#include "rt/core/transform.h"

namespace rt {

// Perspective camera model.
// Simulates a pinhole perspective camera where rays originate from the eye position
// and pass through points on an image screen situated in front of the camera.
class PerspectiveCamera : public Camera {
public:
    // Constructs the camera by calculating the view transformation and image screen dimensions
    // eye is camera position, lookAt is target point, up is direction pointing up
    // fovYDegrees is vertical field of view angle in degrees
    // imageWidth and imageHeight define resolution in pixels
    PerspectiveCamera(const Point3f& eye, const Point3f& lookAt, const Vector3f& up,
                       float fovYDegrees, int imageWidth, int imageHeight);

    // Generates a 3D perspective ray in world space for a given pixel sample on the film
    Ray GenerateRay(const CameraSample& sample) const override;

private:
    Transform cameraToWorld_; // Matrix that transforms rays from camera local space to world space
    int imageWidth_;          // Image width in pixels
    int imageHeight_;         // Image height in pixels
    float halfHeight_;        // Physical half-height of the virtual screen plane at distance z = -1
    float halfWidth_;         // Physical half-width of the virtual screen plane at distance z = -1
};

}

