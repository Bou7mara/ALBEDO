#pragma once
#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#ifndef __CUDA_ARCH__
#include <iostream>
#endif
#include <limits>

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    class Ray {
    public:
        Point3f o;
        Vector3f d;
        mutable float tMax; 
        float time;

        constexpr __host__ __device__ Ray()
            : o(), d(), tMax(std::numeric_limits<float>::infinity()), time(0.0f) {}

        constexpr __host__ __device__ Ray(const Point3f& origin, const Vector3f& direction,
                                          float tMax_ = std::numeric_limits<float>::infinity(),
                                          float time_ = 0.0f)
            : o(origin), d(direction), tMax(tMax_), time(time_) {}

        constexpr __host__ __device__ Point3f operator()(float t) const {
            return o + t * d;
        }
    };

#ifndef __CUDA_ARCH__
    inline std::ostream& operator<<(std::ostream& os, const Ray& r) {
        os << "Ray[o=" << r.o << ", d=" << r.d << ", tMax=" << r.tMax
           << ", time=" << r.time << "]";
        return os;
    }
#endif

} // namespace rt
