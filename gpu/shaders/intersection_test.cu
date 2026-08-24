#include <optix.h>
#include <cuda_runtime.h>
#include <stdint.h>

#include "rt/core/point3.h"
#include "rt/core/vector3.h"
#include "rt/core/normal3.h"
#include "device_scene.h"

struct TestRay {
    rt::Point3f origin;
    rt::Vector3f direction;
    float tMax;
};

struct TestHitResult {
    int hit;
    float t;
    rt::Point3f p;
    rt::Normal3f n;
};

struct IntersectionTestParams {
    OptixTraversableHandle iasHandle;
    const TestRay* testRays;
    TestHitResult* results;
    unsigned int numTestRays;
};

extern "C" {
    __constant__ IntersectionTestParams params;
}

struct HitPayload {
    int hit;
    float t;
    rt::Point3f p;
    rt::Normal3f n;
};

static __forceinline__ __device__ void PackPointer(void* ptr, unsigned int& p0, unsigned int& p1) {
    const uint64_t uptr = reinterpret_cast<uint64_t>(ptr);
    p0 = static_cast<unsigned int>(uptr >> 32);
    p1 = static_cast<unsigned int>(uptr & 0xFFFFFFFF);
}

static __forceinline__ __device__ void* UnpackPointer(unsigned int p0, unsigned int p1) {
    const uint64_t uptr = (static_cast<uint64_t>(p0) << 32) | static_cast<uint64_t>(p1);
    return reinterpret_cast<void*>(uptr);
}

extern "C" __global__ void __raygen__intersection_test() {
    const uint3 idx = optixGetLaunchIndex();
    if (idx.x >= params.numTestRays) return;

    TestRay ray = params.testRays[idx.x];

    HitPayload payload{};
    payload.hit = 0;
    payload.t = ray.tMax;

    unsigned int p0 = 0, p1 = 0;
    PackPointer(&payload, p0, p1);

    optixTrace(
        params.iasHandle,
        make_float3(ray.origin.x, ray.origin.y, ray.origin.z),
        make_float3(ray.direction.x, ray.direction.y, ray.direction.z),
        1e-4f,
        ray.tMax,
        0.0f,
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_NONE,
        0,
        1,
        0,
        p0, p1
    );

    params.results[idx.x] = TestHitResult{ payload.hit, payload.t, payload.p, payload.n };
}

extern "C" __global__ void __closesthit__intersection_test() {
    unsigned int p0 = optixGetPayload_0();
    unsigned int p1 = optixGetPayload_1();
    HitPayload* payload = reinterpret_cast<HitPayload*>(UnpackPointer(p0, p1));

    payload->hit = 1;
    payload->t = optixGetRayTmax();

    const float3 rayOrigin = optixGetWorldRayOrigin();
    const float3 rayDir = optixGetWorldRayDirection();
    payload->p = rt::Point3f(
        rayOrigin.x + payload->t * rayDir.x,
        rayOrigin.y + payload->t * rayDir.y,
        rayOrigin.z + payload->t * rayDir.z
    );

    const rtx::MeshSbtData* sbtData = reinterpret_cast<const rtx::MeshSbtData*>(optixGetSbtDataPointer());
    const int primIdx = optixGetPrimitiveIndex();
    const float2 bary = optixGetTriangleBarycentrics();
    const float u = bary.x;
    const float v = bary.y;
    const float w = 1.0f - u - v;

    int idx0 = sbtData->indices[primIdx * 3 + 0];
    int idx1 = sbtData->indices[primIdx * 3 + 1];
    int idx2 = sbtData->indices[primIdx * 3 + 2];

    rt::Vector3f localN(0, 0, 1);
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
    payload->n = Normalize(rt::Normal3f(rt::Vector3f(worldN.x, worldN.y, worldN.z)));
}

extern "C" __global__ void __miss__intersection_test() {
    unsigned int p0 = optixGetPayload_0();
    unsigned int p1 = optixGetPayload_1();
    HitPayload* payload = reinterpret_cast<HitPayload*>(UnpackPointer(p0, p1));
    payload->hit = 0;
}
