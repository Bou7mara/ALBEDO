#include <optix.h>
#include <cuda_runtime.h>
#include <stdint.h>

#include "launch_params.h"
#include "device_scene.h"
#include "device_material.h"
#include "rt/core/vector3.h"
#include "rt/core/point3.h"
#include "rt/core/normal3.h"
#include "rt/core/ray.h"
#include "rt/core/rng.h"
#include "rt/cam/perspective_camera.h"

struct SingleBounceParams {
    OptixTraversableHandle iasHandle;
    rt::PerspectiveCamera camera;
    unsigned int width;
    unsigned int height;
    unsigned int frameSeed;
    rt::Vector3f* outputBuffer;
};

extern "C" {
    __constant__ SingleBounceParams params;
}

struct HitPayload {
    int hit;
    float t;
    rt::Point3f p;
    rt::Normal3f n;
    rtx::DeviceMaterial material;
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

static __forceinline__ __device__ rt::Vector3f SkyGradient(const rt::Vector3f& dir) {
    rt::Vector3f unitDir = Normalize(dir);
    float t = 0.5f * (unitDir.y + 1.0f);
    return 0.15f * ((1.0f - t) * rt::Vector3f(0.8f, 0.8f, 0.9f) + t * rt::Vector3f(0.4f, 0.5f, 0.7f));
}

static __forceinline__ __device__ uint32_t HashSeed(uint32_t x, uint32_t y, uint32_t seed) {
    uint32_t h = seed ^ (x * 73856093u) ^ (y * 19349663u);
    h = (h ^ 61u) ^ (h >> 16u);
    h += (h << 3u);
    h ^= (h >> 4u);
    h *= 0x27d4eb2du;
    h ^= (h >> 15u);
    return h;
}

static __forceinline__ __device__ HitPayload TraceRay(OptixTraversableHandle handle,
                                                      const rt::Point3f& origin,
                                                      const rt::Vector3f& direction,
                                                      float tMin = 1e-4f,
                                                      float tMax = 1e20f) {
    HitPayload payload{};
    payload.hit = 0;
    payload.t = tMax;

    unsigned int p0 = 0, p1 = 0;
    PackPointer(&payload, p0, p1);

    optixTrace(
        handle,
        make_float3(origin.x, origin.y, origin.z),
        make_float3(direction.x, direction.y, direction.z),
        tMin,
        tMax,
        0.0f,
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_NONE,
        0,
        1,
        0,
        p0, p1
    );

    return payload;
}

extern "C" __global__ void __raygen__single_bounce() {
    const uint3 idx = optixGetLaunchIndex();
    if (idx.x >= params.width || idx.y >= params.height) return;

    const unsigned int pixel = idx.y * params.width + idx.x;

    rt::CameraSample sample{ rt::Point2f(static_cast<float>(idx.x) + 0.5f, static_cast<float>(idx.y) + 0.5f) };
    rt::Ray ray = params.camera.GenerateRay(sample);

    rt::RNG rng(HashSeed(idx.x, idx.y, params.frameSeed));

    HitPayload hit0 = TraceRay(params.iasHandle, ray.o, ray.d);
    if (!hit0.hit) {
        params.outputBuffer[pixel] = SkyGradient(ray.d);
        return;
    }

    rt::Vector3f wo = Normalize(-ray.d);
    rt::Vector3f L = rtx::EvaluateEmission(hit0.material, wo, static_cast<rt::Vector3f>(hit0.n));

    rt::Vector3f wi(0.0f, 0.0f, 0.0f);
    float pdf = 0.0f;
    rt::Vector3f f = rtx::SampleBsdf(hit0.material, wo, static_cast<rt::Vector3f>(hit0.n), rng.Uniform2D(), &wi, &pdf);

    if (pdf > 0.0f && (f.x > 0.0f || f.y > 0.0f || f.z > 0.0f)) {
        float cosTheta = AbsDot(wi, hit0.n);
        rt::Vector3f throughput = f * cosTheta / pdf;

        rt::Vector3f offsetN = (Dot(wi, hit0.n) > 0.0f) ? static_cast<rt::Vector3f>(hit0.n) : -static_cast<rt::Vector3f>(hit0.n);
        rt::Point3f bounceOrigin = hit0.p + 1e-3f * offsetN;

        HitPayload hit1 = TraceRay(params.iasHandle, bounceOrigin, wi);
        if (hit1.hit) {
            rt::Vector3f wo1 = Normalize(-wi);
            L += throughput * rtx::EvaluateEmission(hit1.material, wo1, static_cast<rt::Vector3f>(hit1.n));
        } else {
            L += throughput * SkyGradient(wi);
        }
    }

    params.outputBuffer[pixel] = L;
}

extern "C" __global__ void __closesthit__single_bounce() {
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
    payload->n = Normalize(rt::Normal3f(rt::Vector3f(worldN.x, worldN.y, worldN.z)));
    payload->material = sbtData->material;
}

extern "C" __global__ void __miss__single_bounce() {
    unsigned int p0 = optixGetPayload_0();
    unsigned int p1 = optixGetPayload_1();
    HitPayload* payload = reinterpret_cast<HitPayload*>(UnpackPointer(p0, p1));
    payload->hit = 0;
}
