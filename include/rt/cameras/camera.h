#ifndef RT_CAMERAS_CAMERA_H
#define RT_CAMERAS_CAMERA_H

#include "rt/core/point2.h"
#include "rt/core/ray.h"

namespace rt {

struct CameraSample {
    Point2f pFilm;
};

class Camera {
public:
    virtual ~Camera() = default;

    // Given a sample position in continuous raster space, returns the
    // corresponding world-space ray. Direction is always normalized.
    virtual Ray GenerateRay(const CameraSample& sample) const = 0;
};

} // namespace rt

#endif // RT_CAMERAS_CAMERA_H
