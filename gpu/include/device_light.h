#pragma once

#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/normal3.h"
#include "rt/core/point2.h"
#include "rt/core/sampling.h"
#include "device_material.h"

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rtx {

    struct DeviceLight {
        unsigned int instanceIndex;
        float power;
        rt::Vector3f radiance;
        unsigned int triangleCount;
        float totalArea;
        const rt::Point3f* positions;
        const int* indices;
    };

    struct DeviceLightList {
        DeviceLight* lights = nullptr;
        float* cdf = nullptr;
        unsigned int count = 0;
        float totalPower = 0.0f;
    };

    struct DeviceLiSample {
        rt::Vector3f Li;
        rt::Vector3f wi;
        float pdf;
        float dist;
        rt::Point3f p;
        rt::Normal3f n;
    };

    __host__ __device__ inline const DeviceLight* SampleLightDevice(const DeviceLightList& list,
                                                                    float u,
                                                                    int* lightIdx,
                                                                    float* pmf) {
        if (list.count == 0 || list.cdf == nullptr) {
            *lightIdx = -1;
            *pmf = 0.0f;
            return nullptr;
        }

        // Linear scan CDF inversion, identical to Scene::SampleLight
        for (unsigned int i = 0; i < list.count; ++i) {
            if (u <= list.cdf[i]) {
                *lightIdx = static_cast<int>(i);
                float prevCdf = (i > 0) ? list.cdf[i - 1] : 0.0f;
                *pmf = list.cdf[i] - prevCdf;
                return &list.lights[i];
            }
        }

        *lightIdx = static_cast<int>(list.count - 1);
        float prevCdf = (list.count > 1) ? list.cdf[list.count - 2] : 0.0f;
        *pmf = list.cdf[list.count - 1] - prevCdf;
        return &list.lights[list.count - 1];
    }

    __host__ __device__ inline float LightPmfForInstance(const DeviceLightList& list, unsigned int instanceIdx) {
        for (unsigned int i = 0; i < list.count; ++i) {
            if (list.lights[i].instanceIndex == instanceIdx) {
                float prevCdf = (i > 0) ? list.cdf[i - 1] : 0.0f;
                return list.cdf[i] - prevCdf;
            }
        }
        return 0.0f;
    }

    __host__ __device__ inline DeviceLiSample SampleLightLi(const DeviceLight& light,
                                                            const rt::Point3f& pRef,
                                                            const rt::Point2f& u) {
        DeviceLiSample sample{};
        if (light.triangleCount == 0 || light.totalArea <= 0.0f) {
            sample.pdf = 0.0f;
            return sample;
        }

        // Pick triangle
        float uScaled = u.x * static_cast<float>(light.triangleCount);
        unsigned int triIdx = static_cast<unsigned int>(uScaled);
        if (triIdx >= light.triangleCount) triIdx = light.triangleCount - 1;
        float uTri = uScaled - static_cast<float>(triIdx);

        int i0 = light.indices[triIdx * 3 + 0];
        int i1 = light.indices[triIdx * 3 + 1];
        int i2 = light.indices[triIdx * 3 + 2];

        rt::Point3f v0 = light.positions[i0];
        rt::Point3f v1 = light.positions[i1];
        rt::Point3f v2 = light.positions[i2];

        // Sample point on triangle
        float sqrtU = std::sqrt(uTri);
        float b0 = 1.0f - sqrtU;
        float b1 = u.y * sqrtU;
        float b2 = 1.0f - b0 - b1;

        sample.p = b0 * v0 + b1 * v1 + b2 * v2;
        rt::Vector3f geomN = Normalize(Cross(v1 - v0, v2 - v0));
        sample.n = rt::Normal3f(geomN);

        rt::Vector3f toLight = sample.p - pRef;
        sample.dist = Length(toLight);
        if (sample.dist < 1e-6f) {
            sample.pdf = 0.0f;
            return sample;
        }
        sample.wi = toLight / sample.dist;

        float cosThetaLight = Dot(-sample.wi, sample.n);
        if (cosThetaLight <= 0.0f) {
            sample.pdf = 0.0f;
            sample.Li = rt::Vector3f(0.0f, 0.0f, 0.0f);
            return sample;
        }

        sample.Li = light.radiance;
        // Reproduce triangle area-to-solid-angle measure
        sample.pdf = (sample.dist * sample.dist) / (light.totalArea * cosThetaLight);
        return sample;
    }

} // namespace rtx
