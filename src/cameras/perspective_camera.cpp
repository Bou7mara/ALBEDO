#include "rt/cam/perspective_camera.h"
#include "rt/core/math_utils.h"
#include <cmath>

namespace rt {
    PerspectiveCamera::PerspectiveCamera(const Point3f& eye, const Point3f& lookAt,
                                          const Vector3f& up, float fovYDegrees,
                                          int imageWidth, int imageHeight)
        : cameraToWorld_(Transform::LookAt(eye, lookAt, up).Inverse()),
          imageWidth_(imageWidth),
          imageHeight_(imageHeight)
    {
        float aspect = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
        halfHeight_ = std::tan(Radians(fovYDegrees) / 2.0f);
        halfWidth_ = aspect * halfHeight_;
    }
}
