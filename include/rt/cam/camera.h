#pragma once
#include "rt/core/point2.h"
#include "rt/core/ray.h"

namespace rt {

// Data required to generate a ray for a single film sample
// pFilm stores the 2D pixel coordinates on the image plane
struct CameraSample {
    Point2f pFilm;
};

// Abstract base class for all camera models (such as perspective or orthographic cameras)
// The camera is responsible for mapping 2D pixel samples on the image film to 3D rays in world space
class Camera {
public:
    virtual ~Camera() = default;

    // Generates a 3D ray starting from the camera for the given 2D film sample position
    virtual Ray GenerateRay(const CameraSample& sample) const = 0;
};

}

