#pragma once
#include "rt/core/point2.h"
#include "rt/core/ray.h"

namespace rt {

struct CameraSample {
    Point2f pFilm;
};

class Camera {
public:
    virtual ~Camera() = default;

    virtual Ray GenerateRay(const CameraSample& sample) const = 0;
};

}
