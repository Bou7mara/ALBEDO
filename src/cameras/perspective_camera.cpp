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

Ray PerspectiveCamera::GenerateRay(const CameraSample& sample) const {
    // Continuous raster coords -> normalized [0,1] screen coords.
    float ndcX = sample.pFilm.x / static_cast<float>(imageWidth_);
    float ndcY = sample.pFilm.y / static_cast<float>(imageHeight_);

    // [0,1] -> [-halfWidth, halfWidth] / [-halfHeight, halfHeight].
    // Y is flipped: raster space grows downward (row 0 = top of image),
    // screen space grows upward, matching conventional image coordinates.
    float screenX = (2.0f * ndcX - 1.0f) * halfWidth_;
    float screenY = (1.0f - 2.0f * ndcY) * halfHeight_;

    // Image plane sits at z=1 in camera space (arbitrary distance --
    // only the ray's direction matters for a pinhole camera, and it
    // gets normalized below, so the plane's exact distance is free).
    Vector3f dirCamera = Normalize(Vector3f(screenX, screenY, 1.0f));
    Ray rayCamera(Point3f(0.0f, 0.0f, 0.0f), dirCamera);

    // Transform::operator()(const Ray&) already handles converting the
    // origin as a point and the direction as a vector correctly -- no
    // new logic needed here, just reusing what Transform already does.
    return cameraToWorld_(rayCamera);
}

} // namespace rt
