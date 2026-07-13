#ifndef RT_MATERIALS_METAL_H
#define RT_MATERIALS_METAL_H

#include "rt/materials/bsdf.h"

namespace rt {

class Metal : public BSDF {
public:
    explicit Metal(const Vector3f& albedo) : albedo_(albedo) {}

    Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;
    Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                       const Point2f& u, Vector3f* wi, float* pdf) const override;

private:
    Vector3f albedo_;
};

} // namespace rt

#endif // RT_MATERIALS_METAL_H
