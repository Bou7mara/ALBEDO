#ifndef RT_MATERIALS_LAMBERTIAN_H
#define RT_MATERIALS_LAMBERTIAN_H

#include "rt/materials/bsdf.h"

namespace rt {

class Lambertian : public BSDF {
public:
    explicit Lambertian(const Vector3f& albedo) : albedo_(albedo) {}

    Vector3f f(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override;
    Vector3f Sample_f(const Vector3f& wo, const Vector3f& n,
                       const Point2f& u, Vector3f* wi,
                       float* pdf) const override;

private:
    Vector3f albedo_;
};

} // namespace rt

#endif // RT_MATERIALS_LAMBERTIAN_H
