#include <optix.h>
#include <cuda_runtime.h>
#include <stdint.h>

#include "launch_params.h"
#include "device_scene.h"
#include "device_material.h"
#include "device_light.h"
#include "rt/integrator_constants.h"
#include "rt/core/vector3.h"
#include "rt/core/point3.h"
#include "rt/core/normal3.h"
#include "rt/core/ray.h"
#include "rt/core/rng.h"
#include "rt/cam/perspective_camera.h"

struct PathTracerParams {
    OptixTraversableHandle iasHandle;
    rt::PerspectiveCamera camera;
    rtx::DeviceLightList lights;
    unsigned int width;
    unsigned int height;
    unsigned int samplesPerPixel;
    unsigned int maxDepth;
    unsigned int frameSeed;
    rt::Vector3f* outputBuffer;
    rt::Vector3f* albedoBuffer;
    rt::Vector3f* normalBuffer;
};

extern "C" {
    __constant__ PathTracerParams params;
}

struct PathHitPayload {
    int hit;
    float t;
    rt::Point3f p;
    rt::Normal3f n;
    rt::Normal3f ns;
    rtx::DeviceMaterial material;
    unsigned int instanceIndex;
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

static __forceinline__ __device__ rt::Vector3f GetAlbedoForGuide(const rtx::DeviceMaterial& mat) {
    switch (mat.kind) {
        case rtx::MaterialKind::Lambertian:
            return mat.lambertian.albedo;
        case rtx::MaterialKind::Metal:
            return mat.metal.albedo;
        case rtx::MaterialKind::Dielectric:
            return mat.dielectric.tint;
        case rtx::MaterialKind::MicrofacetDielectric:
            return rt::Vector3f(1.0f, 1.0f, 1.0f);
        case rtx::MaterialKind::MicrofacetConductor:
            return mat.microfacetConductor.tint;
        case rtx::MaterialKind::Emissive:
            return mat.emissive.radiance;
    }
    return rt::Vector3f(0.5f, 0.5f, 0.5f);
}

static __forceinline__ __device__ uint32_t HashSeed(uint32_t x, uint32_t y, uint32_t sampleIdx, uint32_t seed) {
    uint32_t h = seed ^ (x * 73856093u) ^ (y * 19349663u) ^ (sampleIdx * 83492791u);
    h = (h ^ 61u) ^ (h >> 16u);
    h += (h << 3u);
    h ^= (h >> 4u);
    h *= 0x27d4eb2du;
    h ^= (h >> 15u);
    return h;
}

static __forceinline__ __device__ PathHitPayload TraceRay(OptixTraversableHandle handle,
                                                          const rt::Point3f& origin,
                                                          const rt::Vector3f& direction,
                                                          float tMin = 1e-4f,
                                                          float tMax = 1e20f) {
    PathHitPayload payload{};
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
        0, // SBT offset
        1, // SBT stride
        0, // missSBTIndex
        p0, p1
    );

    return payload;
}

static __forceinline__ __device__ bool TraceShadowRay(OptixTraversableHandle handle,
                                                      const rt::Point3f& origin,
                                                      const rt::Vector3f& direction,
                                                      float tMin = 1e-4f,
                                                      float tMax = 1e20f) {
    unsigned int p0 = 0;
    optixTrace(
        handle,
        make_float3(origin.x, origin.y, origin.z),
        make_float3(direction.x, direction.y, direction.z),
        tMin,
        tMax,
        0.0f,
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT | OPTIX_RAY_FLAG_DISABLE_CLOSESTHIT,
        0,
        1,
        0,
        p0
    );
    return (p0 != 0);
}

extern "C" __global__ void __raygen__albedo() {
    const uint3 idx = optixGetLaunchIndex();
    if (idx.x >= params.width || idx.y >= params.height) return;

    const unsigned int pixel = idx.y * params.width + idx.x;

    rt::Vector3f colorSum(0.0f, 0.0f, 0.0f);

    for (unsigned int s = 0; s < params.samplesPerPixel; ++s) {
        rt::RNG rng(HashSeed(idx.x, idx.y, s, params.frameSeed));

        rt::Point2f jitter = rng.Uniform2D();
        rt::CameraSample sample{ rt::Point2f(static_cast<float>(idx.x) + jitter.x, static_cast<float>(idx.y) + jitter.y) };
        rt::Ray r = params.camera.GenerateRay(sample);

        rt::Vector3f L(0.0f, 0.0f, 0.0f);
        rt::Vector3f throughput(1.0f, 1.0f, 1.0f);
        bool specularBounce = true;
        float prevBsdfPdf = 0.0f;

        for (unsigned int depth = 0; depth < params.maxDepth; ++depth) {
            PathHitPayload isect = TraceRay(params.iasHandle, r.o, r.d);
            if (!isect.hit) {
                if (depth == 0 && params.normalBuffer && params.albedoBuffer) {
                    params.normalBuffer[pixel] = rt::Vector3f(0.0f, 0.0f, 0.0f);
                    params.albedoBuffer[pixel] = rt::Vector3f(0.0f, 0.0f, 0.0f);
                }
                L += throughput * SkyGradient(r.d);
                break;
            }

            rt::Vector3f wo = Normalize(-r.d);
            rt::Vector3f n = static_cast<rt::Vector3f>(isect.n);

            if (depth == 0 && params.normalBuffer && params.albedoBuffer) {
                params.normalBuffer[pixel] = n;
                params.albedoBuffer[pixel] = GetAlbedoForGuide(isect.material);
            }

            rt::Vector3f emitted = rtx::EvaluateEmission(isect.material, wo, n);

            if (emitted.x > 0.0f || emitted.y > 0.0f || emitted.z > 0.0f) {
                if (specularBounce || params.lights.count == 0) {
                    L += throughput * emitted;
                } else {
                    float pmf = rtx::LightPmfForInstance(params.lights, isect.instanceIndex);
                    float cosThetaLight = AbsDot(wo, n);
                    float lightArea = 1.0f;
                    for (unsigned int li = 0; li < params.lights.count; ++li) {
                        if (params.lights.lights[li].instanceIndex == isect.instanceIndex) {
                            lightArea = params.lights.lights[li].totalArea;
                            break;
                        }
                    }
                    float lightPdf = (isect.t * isect.t) / (lightArea * cosThetaLight) * pmf;
                    float weight = rt::PowerHeuristic(1, prevBsdfPdf, 1, lightPdf);
                    L += throughput * emitted * weight;
                }
            }

            // Next Event Estimation (Direct Light Sampling)
            if (params.lights.count > 0) {
                int lightIdx = -1;
                float pmf = 0.0f;
                const auto* light = rtx::SampleLightDevice(params.lights, rng.Uniform1D(), &lightIdx, &pmf);

                if (light && pmf > 0.0f) {
                    rtx::DeviceLiSample lightSample = rtx::SampleLightLi(*light, isect.p, rng.Uniform2D());
                    if (lightSample.pdf > 0.0f) {
                        constexpr float kEpsilon = 1e-4f;
                        rt::Vector3f offsetNormal = (Dot(lightSample.wi, isect.n) > 0.0f) ? n : -n;
                        rt::Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;

                        bool occluded = TraceShadowRay(params.iasHandle, offsetOrigin, lightSample.wi, kEpsilon, lightSample.dist - 2.0f * kEpsilon);
                        if (!occluded) {
                            rt::Vector3f f = rtx::EvaluateBsdf(isect.material, wo, lightSample.wi, n);
                            if (f.x > 0.0f || f.y > 0.0f || f.z > 0.0f) {
                                float bsdfPdf = rtx::PdfBsdf(isect.material, wo, lightSample.wi, n);
                                if (bsdfPdf > 0.0f) {
                                    float lPdf = lightSample.pdf * pmf;
                                    float weight = rt::PowerHeuristic(1, lPdf, 1, bsdfPdf);
                                    float cosTheta = AbsDot(lightSample.wi, isect.n);
                                    L += throughput * f * lightSample.Li * cosTheta * weight / lPdf;
                                }
                            }
                        }
                    }
                }
            }

            // Sample BSDF for next bounce
            rt::Vector3f wi(0.0f, 0.0f, 0.0f);
            float pdf = 0.0f;
            rt::Vector3f f = rtx::SampleBsdf(isect.material, wo, n, rng.Uniform2D(), &wi, &pdf);

            if (pdf <= 0.0f || (f.x == 0.0f && f.y == 0.0f && f.z == 0.0f)) break;

            float cosTheta = AbsDot(wi, isect.n);
            throughput = throughput * f * cosTheta / pdf;

            prevBsdfPdf = rtx::PdfBsdf(isect.material, wo, wi, n);
            specularBounce = (prevBsdfPdf == 0.0f);

            // Russian Roulette
            if (depth >= rt::kRRStartDepth) {
                const float q = std::clamp(rt::MaxChannel(throughput), rt::kRRProbabilityMinimumThreshold, rt::kRRProbabilityMaximumThreshold);
                if (rng.Uniform1D() > q) break;
                throughput = throughput / q;
            }

            constexpr float kEpsilon = 1e-4f;
            rt::Vector3f offsetNormal = (Dot(wi, isect.n) > 0.0f) ? n : -n;
            rt::Point3f offsetOrigin = isect.p + kEpsilon * offsetNormal;

            r = rt::Ray(offsetOrigin, wi);
        }

        colorSum += L;
    }

    params.outputBuffer[pixel] = colorSum / static_cast<float>(params.samplesPerPixel);
}

extern "C" __global__ void __closesthit__albedo() {
    unsigned int p0 = optixGetPayload_0();
    unsigned int p1 = optixGetPayload_1();
    PathHitPayload* payload = reinterpret_cast<PathHitPayload*>(UnpackPointer(p0, p1));

    payload->hit = 1;
    payload->t = optixGetRayTmax();
    payload->instanceIndex = optixGetInstanceId();

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

    rt::Point3f v0 = sbtData->positions[idx0];
    rt::Point3f v1 = sbtData->positions[idx1];
    rt::Point3f v2 = sbtData->positions[idx2];
    rt::Vector3f localGeomN = Cross(v1 - v0, v2 - v0);

    float3 worldGeomN = optixTransformNormalFromObjectToWorldSpace(make_float3(localGeomN.x, localGeomN.y, localGeomN.z));
    payload->n = Normalize(rt::Normal3f(rt::Vector3f(worldGeomN.x, worldGeomN.y, worldGeomN.z)));

    if (sbtData->normals != nullptr) {
        rt::Normal3f n0 = sbtData->normals[idx0];
        rt::Normal3f n1 = sbtData->normals[idx1];
        rt::Normal3f n2 = sbtData->normals[idx2];
        rt::Vector3f localShadN = w * static_cast<rt::Vector3f>(n0) + u * static_cast<rt::Vector3f>(n1) + v * static_cast<rt::Vector3f>(n2);
        float3 worldShadN = optixTransformNormalFromObjectToWorldSpace(make_float3(localShadN.x, localShadN.y, localShadN.z));
        payload->ns = Normalize(rt::Normal3f(rt::Vector3f(worldShadN.x, worldShadN.y, worldShadN.z)));
    } else {
        payload->ns = payload->n;
    }

    payload->material = sbtData->material;
}

extern "C" __global__ void __miss__albedo() {
    unsigned int p0 = optixGetPayload_0();
    unsigned int p1 = optixGetPayload_1();
    PathHitPayload* payload = reinterpret_cast<PathHitPayload*>(UnpackPointer(p0, p1));
    payload->hit = 0;
}
