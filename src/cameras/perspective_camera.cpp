#include "rt/cam/perspective_camera.h"
#include "rt/core/math_utils.h"
#include <cmath>

namespace rt {
    // Constructor: Calculates camera view matrix and screen half-dimensions from field of view
    PerspectiveCamera::PerspectiveCamera(const Point3f& eye, const Point3f& lookAt,
                                          const Vector3f& up, float fovYDegrees,
                                          int imageWidth, int imageHeight)
        : cameraToWorld_(Transform::LookAt(eye, lookAt, up).Inverse()),
          imageWidth_(imageWidth),
          imageHeight_(imageHeight)
    {
        // Calculate aspect ratio (width divided by height)
        float aspect = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
        
        // Calculate physical height of screen at distance 1.0 using trigonometry
        halfHeight_ = std::tan(Radians(fovYDegrees) / 2.0f);
        
        // Calculate physical width using aspect ratio
        halfWidth_ = aspect * halfHeight_;
    }

    // Generates a ray passing through a specific pixel location on the film
    Ray PerspectiveCamera::GenerateRay(const CameraSample& sample) const {
        // Convert pixel coordinates (0 to width, 0 to height) to normalized device coordinates (0 to 1)
        float ndcX = sample.pFilm.x / static_cast<float>(imageWidth_);
        float ndcY = sample.pFilm.y / static_cast<float>(imageHeight_);

        // Map normalized device coordinates to physical camera screen coordinates (-halfWidth to +halfWidth, etc.)
        float screenX = (2.0f * ndcX - 1.0f) * halfWidth_;
        float screenY = (1.0f - 2.0f * ndcY) * halfHeight_;

        // Create ray direction in camera space pointing towards positive Z (+Z into the screen)
        Vector3f dirCamera = Normalize(Vector3f(screenX, screenY, 1.0f));
        Ray rayCamera(Point3f(0.0f, 0.0f, 0.0f), dirCamera);

        // Transform the ray from camera space to world space coordinates
        return cameraToWorld_(rayCamera);
    }
}

