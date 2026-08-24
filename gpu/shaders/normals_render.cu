#include <optix.h>
#include <cuda_runtime.h>
#include <stdint.h>

#include "launch_params.h"
#include "device_scene.h"
#include "rt/core/vector3.h"
#include "rt/core/point3.h"
#include "rt/core/normal3.h"
#include "rt/core/ray.h"

extern "C" {
    __constant__ rtx::NormalsLaunchParams params;
}

static __forceinline__ __device__ unsigned int FloatAsUint(float f) {
    union { float f; unsigned int u; } un;
    un.f = f;
    return un.u;
}

static __forceinline__ __device__ float UintAsFloat(unsigned int u) {
    union { float f; unsigned int u; } un;
    un.u = u;
    return un.f;
}

extern "C" __global__ void __raygen__normals() {
    const uint3 idx = optixGetLaunchIndex();
    if (idx.x >= params.width || idx.y >= params.height) return;

    const unsigned int pixel = idx.y * params.width + idx.x;

    rt::CameraSample sample{ rt::Point2f(static_cast<float>(idx.x) + 0.5f, static_cast<float>(idx.y) + 0.5f) };
    rt::Ray ray = params.camera.GenerateRay(sample);

    unsigned int p0 = 0, p1 = 0, p2 = 0;
    optixTrace(
        params.iasHandle,
        make_float3(ray.o.x, ray.o.y, ray.o.z),
        make_float3(ray.d.x, ray.d.y, ray.d.z),
        1e-4f,
        ray.tMax,
        0.0f,
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_NONE,
        0,
        1,
        0,
        p0, p1, p2
    );

    params.outputBuffer[pixel] = rt::Vector3f(UintAsFloat(p0), UintAsFloat(p1), UintAsFloat(p2));
}

extern "C" __global__ void __closesthit__normals() {
    const rtx::MeshSbtData* sbtData = reinterpret_cast<const rtx::MeshSbtData*>(optixGetSbtDataPointer());
    const int primIdx = optixGetPrimitiveIndex();
    const float2 bary = optixGetTriangleBarycentrics();
    const float u = bary.x;
    const float v = bary.y;
    const float w = 1.0f - u - v;

    int idx0 = sbtData->indices[primIdx * 3 + 0];
    int idx1 = sbtData->indices[primIdx * 3 + 1];
    int idx2 = sbtData->indices[primIdx * 3 + 2];

    rt::Vector3f localN(0.0f, 0.0f, 1.0f);
    if (sbtData->normals != nullptr) {
        rt::Normal3f n0 = sbtData->normals[idx0];
        rt::Normal3f n1 = sbtData->normals[idx1];
        rt::Normal3f n2 = sbtData->normals[idx2];
        localN = w * static_cast<rt::Vector3f>(n0) + u * static_cast<rt::Vector3f>(n1) + v * static_cast<rt::Vector3f>(n2);
    } else {
        rt::Point3f v0 = sbtData->positions[idx0];
        rt::Point3f v1 = sbtData->positions[idx1];
        rt::Point3f v2 = sbtData->positions[idx2];
        localN = Cross(v1 - v0, v2 - v0);
    }

    float3 worldN = optixTransformNormalFromObjectToWorldSpace(make_float3(localN.x, localN.y, localN.z));
    rt::Vector3f nWorld = Normalize(rt::Vector3f(worldN.x, worldN.y, worldN.z));

    rt::Vector3f color = 0.5f * (nWorld + rt::Vector3f(1.0f, 1.0f, 1.0f));

    optixSetPayload_0(FloatAsUint(color.x));
    optixSetPayload_1(FloatAsUint(color.y));
    optixSetPayload_2(FloatAsUint(color.z));
}

extern "C" __global__ void __miss__normals() {
    const float3 rayDir = optixGetWorldRayDirection();
    rt::Vector3f unitDir = Normalize(rt::Vector3f(rayDir.x, rayDir.y, rayDir.z));
    float t = 0.5f * (unitDir.y + 1.0f);
    rt::Vector3f bg = 0.15f * ((1.0f - t) * rt::Vector3f(0.8f, 0.8f, 0.9f) + t * rt::Vector3f(0.4f, 0.5f, 0.7f));

    optixSetPayload_0(FloatAsUint(bg.x));
    optixSetPayload_1(FloatAsUint(bg.y));
    optixSetPayload_2(FloatAsUint(bg.z));
}
