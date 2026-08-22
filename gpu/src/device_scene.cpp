#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "device_scene.h"
#include <cuda_runtime.h>

#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/dielectric.h"
#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/emissive.h"

#include <optix_stubs.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <cstring>
#include <cmath>

namespace rtx {

    DeviceMaterial ConvertBsdfToDeviceMaterial(const rt::BSDF* bsdf) {
        if (!bsdf) {
            return DeviceMaterial::MakeLambertian(rt::Vector3f(0.5f, 0.5f, 0.5f));
        }
        if (auto l = dynamic_cast<const rt::Lambertian*>(bsdf)) {
            return DeviceMaterial::MakeLambertian(l->Albedo(), l->Roughness());
        }
        if (auto m = dynamic_cast<const rt::Metal*>(bsdf)) {
            return DeviceMaterial::MakeMetal(m->Albedo());
        }
        if (auto d = dynamic_cast<const rt::Dielectric*>(bsdf)) {
            return DeviceMaterial::MakeDielectric(d->Ior(), d->Tint(), d->Dispersion());
        }
        if (auto e = dynamic_cast<const rt::Emissive*>(bsdf)) {
            return DeviceMaterial::MakeEmissive(e->Radiance());
        }
        if (auto mf = dynamic_cast<const rt::Microfacet*>(bsdf)) {
            if (mf->IsDielectric()) {
                return DeviceMaterial::MakeMicrofacetDielectric(std::sqrt(mf->Alpha()), mf->Ior());
            } else {
                return DeviceMaterial::MakeMicrofacetConductor(std::sqrt(mf->Alpha()), mf->Eta(), mf->K(), mf->Tint());
            }
        }
        return DeviceMaterial::MakeLambertian(rt::Vector3f(0.5f, 0.5f, 0.5f));
    }

    namespace {

        #define CUDA_CHECK(call)                                                                     \
            do {                                                                                     \
                cudaError_t error = (call);                                                          \
                if (error != cudaSuccess) {                                                          \
                    std::ostringstream ss;                                                           \
                    ss << "CUDA Error (" << cudaGetErrorName(error) << "): "                         \
                       << cudaGetErrorString(error) << " at " << __FILE__ << ":" << __LINE__;       \
                    throw std::runtime_error(ss.str());                                              \
                }                                                                                    \
            } while (0)

        #define OPTIX_CHECK(call)                                                                    \
            do {                                                                                     \
                OptixResult res = (call);                                                            \
                if (res != OPTIX_SUCCESS) {                                                          \
                    std::ostringstream ss;                                                           \
                    ss << "OptiX Error (" << optixGetErrorName(res) << "): "                         \
                       << optixGetErrorString(res) << " at " << __FILE__ << ":" << __LINE__;        \
                    throw std::runtime_error(ss.str());                                              \
                }                                                                                    \
            } while (0)

        DeviceTriangleMesh UploadAndBuildGAS(const std::shared_ptr<rt::TriangleMesh>& mesh,
                                             OptixDeviceContext ctx,
                                             CUstream stream) {
            DeviceTriangleMesh devMesh{};
            devMesh.triangleCount = static_cast<unsigned int>(mesh->TriangleCount());
            devMesh.vertexCount = static_cast<unsigned int>(mesh->positions.size());

            // 1. Upload Vertex Positions
            size_t posBytes = mesh->positions.size() * sizeof(rt::Point3f);
            CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&devMesh.d_positions), posBytes));
            CUDA_CHECK(cudaMemcpyAsync(
                reinterpret_cast<void*>(devMesh.d_positions),
                mesh->positions.data(),
                posBytes,
                cudaMemcpyHostToDevice,
                stream
            ));

            // 2. Upload Shading Normals (if any)
            if (!mesh->normals.empty()) {
                size_t normBytes = mesh->normals.size() * sizeof(rt::Normal3f);
                CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&devMesh.d_normals), normBytes));
                CUDA_CHECK(cudaMemcpyAsync(
                    reinterpret_cast<void*>(devMesh.d_normals),
                    mesh->normals.data(),
                    normBytes,
                    cudaMemcpyHostToDevice,
                    stream
                ));
            }

            // 3. Upload UVs (if any)
            if (!mesh->uvs.empty()) {
                size_t uvBytes = mesh->uvs.size() * sizeof(rt::Point2f);
                CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&devMesh.d_uvs), uvBytes));
                CUDA_CHECK(cudaMemcpyAsync(
                    reinterpret_cast<void*>(devMesh.d_uvs),
                    mesh->uvs.data(),
                    uvBytes,
                    cudaMemcpyHostToDevice,
                    stream
                ));
            }

            // 4. Upload Indices
            size_t idxBytes = mesh->indices.size() * sizeof(int);
            CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&devMesh.d_indices), idxBytes));
            CUDA_CHECK(cudaMemcpyAsync(
                reinterpret_cast<void*>(devMesh.d_indices),
                mesh->indices.data(),
                idxBytes,
                cudaMemcpyHostToDevice,
                stream
            ));

            // 5. Setup OptiX GAS Build Input
            OptixBuildInput buildInput{};
            buildInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
            
            CUdeviceptr vertexBuffers[1] = { devMesh.d_positions };
            buildInput.triangleArray.vertexBuffers = vertexBuffers;
            buildInput.triangleArray.numVertices = devMesh.vertexCount;
            buildInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
            buildInput.triangleArray.vertexStrideInBytes = sizeof(rt::Point3f);

            buildInput.triangleArray.indexBuffer = devMesh.d_indices;
            buildInput.triangleArray.numIndexTriplets = devMesh.triangleCount;
            buildInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
            buildInput.triangleArray.indexStrideInBytes = sizeof(int) * 3;

            uint32_t triangleInputFlags[1] = { OPTIX_GEOMETRY_FLAG_NONE };
            buildInput.triangleArray.flags = triangleInputFlags;
            buildInput.triangleArray.numSbtRecords = 1;

            OptixAccelBuildOptions accelOptions{};
            accelOptions.buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE;
            accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

            OptixAccelBufferSizes gasBufferSizes{};
            OPTIX_CHECK(optixAccelComputeMemoryUsage(ctx, &accelOptions, &buildInput, 1, &gasBufferSizes));

            CUdeviceptr d_tempGas = 0;
            CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_tempGas), gasBufferSizes.tempSizeInBytes));
            CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&devMesh.gasBuffer), gasBufferSizes.outputSizeInBytes));

            OPTIX_CHECK(optixAccelBuild(
                ctx,
                stream,
                &accelOptions,
                &buildInput,
                1,
                d_tempGas,
                gasBufferSizes.tempSizeInBytes,
                devMesh.gasBuffer,
                gasBufferSizes.outputSizeInBytes,
                &devMesh.gasHandle,
                nullptr,
                0
            ));

            CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_tempGas)));
            return devMesh;
        }

    } // namespace

    DeviceScene::~DeviceScene() {
        Destroy();
    }

    DeviceScene::DeviceScene(DeviceScene&& other) noexcept
        : meshes(std::move(other.meshes)),
          instances(std::move(other.instances)),
          allocatedBuffers(std::move(other.allocatedBuffers)),
          iasHandle(other.iasHandle),
          iasBuffer(other.iasBuffer),
          d_instances(other.d_instances),
          lightList(other.lightList),
          d_lights(other.d_lights),
          d_lightCdf(other.d_lightCdf) {
        other.iasHandle = 0;
        other.iasBuffer = 0;
        other.d_instances = 0;
        other.d_lights = 0;
        other.d_lightCdf = 0;
        other.lightList = DeviceLightList{};
    }

    DeviceScene& DeviceScene::operator=(DeviceScene&& other) noexcept {
        if (this != &other) {
            Destroy();
            meshes = std::move(other.meshes);
            instances = std::move(other.instances);
            allocatedBuffers = std::move(other.allocatedBuffers);
            iasHandle = other.iasHandle;
            iasBuffer = other.iasBuffer;
            d_instances = other.d_instances;
            lightList = other.lightList;
            d_lights = other.d_lights;
            d_lightCdf = other.d_lightCdf;

            other.iasHandle = 0;
            other.iasBuffer = 0;
            other.d_instances = 0;
            other.d_lights = 0;
            other.d_lightCdf = 0;
            other.lightList = DeviceLightList{};
        }
        return *this;
    }

    void DeviceScene::Destroy() {
        for (auto& mesh : meshes) {
            if (mesh.d_positions) { cudaFree(reinterpret_cast<void*>(mesh.d_positions)); mesh.d_positions = 0; }
            if (mesh.d_normals)   { cudaFree(reinterpret_cast<void*>(mesh.d_normals));   mesh.d_normals = 0; }
            if (mesh.d_uvs)       { cudaFree(reinterpret_cast<void*>(mesh.d_uvs));       mesh.d_uvs = 0; }
            if (mesh.d_indices)   { cudaFree(reinterpret_cast<void*>(mesh.d_indices));   mesh.d_indices = 0; }
            if (mesh.gasBuffer)   { cudaFree(reinterpret_cast<void*>(mesh.gasBuffer));   mesh.gasBuffer = 0; }
        }
        meshes.clear();
        instances.clear();

        for (auto buf : allocatedBuffers) {
            if (buf) cudaFree(reinterpret_cast<void*>(buf));
        }
        allocatedBuffers.clear();

        if (d_instances) {
            cudaFree(reinterpret_cast<void*>(d_instances));
            d_instances = 0;
        }
        if (iasBuffer) {
            cudaFree(reinterpret_cast<void*>(iasBuffer));
            iasBuffer = 0;
        }
        if (d_lights) {
            cudaFree(reinterpret_cast<void*>(d_lights));
            d_lights = 0;
        }
        if (d_lightCdf) {
            cudaFree(reinterpret_cast<void*>(d_lightCdf));
            d_lightCdf = 0;
        }
        iasHandle = 0;
        lightList = DeviceLightList{};
    }

    DeviceScene DeviceScene::Build(const std::shared_ptr<rt::SceneNode>& root,
                                   OptixDeviceContext ctx,
                                   CUstream stream) {
        DeviceScene scene;
        if (!root) return scene;

        std::unordered_map<const rt::TriangleMesh*, size_t> meshCache;
        std::vector<OptixInstance> optixInstances;
        std::vector<DeviceLight> hostLights;

        auto flatten = [&](auto& self, const std::shared_ptr<rt::SceneNode>& node, const rt::Transform& parentToWorld) -> void {
                if (!node) return;
                rt::Transform nodeToWorld = parentToWorld * node->localTransform;

                if (node->mesh && node->mesh->TriangleCount() > 0) {
                    size_t meshIndex = 0;
                    auto it = meshCache.find(node->mesh.get());
                    if (it != meshCache.end()) {
                        meshIndex = it->second;
                    } else {
                        DeviceTriangleMesh devMesh = UploadAndBuildGAS(node->mesh, ctx, stream);
                        meshIndex = scene.meshes.size();
                        scene.meshes.push_back(devMesh);
                        meshCache[node->mesh.get()] = meshIndex;
                    }

                    DeviceMaterial mat = ConvertBsdfToDeviceMaterial(node->bsdf.get());
                    size_t instanceIdx = scene.instances.size();
                    scene.instances.push_back(DeviceInstanceRecord{ meshIndex, mat });

                    OptixInstance instance{};
                    std::array<float, 12> xform = nodeToWorld.ToOptixRowMajor3x4();
                    std::memcpy(instance.transform, xform.data(), sizeof(float) * 12);
                    instance.instanceId = static_cast<unsigned int>(instanceIdx);
                    instance.visibilityMask = 255;
                    instance.sbtOffset = static_cast<unsigned int>(instanceIdx);
                    instance.flags = OPTIX_INSTANCE_FLAG_NONE;
                    instance.traversableHandle = scene.meshes[meshIndex].gasHandle;
                    optixInstances.push_back(instance);

                    // Track emissive lights
                    if (mat.kind == MaterialKind::Emissive) {
                        float totalArea = 0.0f;
                        for (int t = 0; t < node->mesh->TriangleCount(); ++t) {
                            rt::Point3f p0 = nodeToWorld(node->mesh->positions[node->mesh->indices[3 * t + 0]]);
                            rt::Point3f p1 = nodeToWorld(node->mesh->positions[node->mesh->indices[3 * t + 1]]);
                            rt::Point3f p2 = nodeToWorld(node->mesh->positions[node->mesh->indices[3 * t + 2]]);
                            totalArea += 0.5f * Length(Cross(p1 - p0, p2 - p0));
                        }
                        float maxRadiance = std::max(mat.emissive.radiance.x, std::max(mat.emissive.radiance.y, mat.emissive.radiance.z));
                        float power = totalArea * 3.14159265358979323846f * maxRadiance;

                        DeviceLight dl{};
                        dl.instanceIndex = static_cast<unsigned int>(instanceIdx);
                        dl.power = power;
                        dl.radiance = mat.emissive.radiance;
                        dl.triangleCount = scene.meshes[meshIndex].triangleCount;
                        dl.totalArea = totalArea;
                        dl.positions = reinterpret_cast<const rt::Point3f*>(scene.meshes[meshIndex].d_positions);
                        dl.indices = reinterpret_cast<const int*>(scene.meshes[meshIndex].d_indices);
                        hostLights.push_back(dl);
                    }
                }

                for (const auto& child : node->children) {
                    self(self, child, nodeToWorld);
                }
            };

        flatten(flatten, root, rt::Transform::Identity());

        if (optixInstances.empty()) {
            return scene;
        }

        // Build IAS over all instances
        size_t instancesBytes = optixInstances.size() * sizeof(OptixInstance);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&scene.d_instances), instancesBytes));
        CUDA_CHECK(cudaMemcpyAsync(
            reinterpret_cast<void*>(scene.d_instances),
            optixInstances.data(),
            instancesBytes,
            cudaMemcpyHostToDevice,
            stream
        ));

        OptixBuildInput iasBuildInput{};
        iasBuildInput.type = OPTIX_BUILD_INPUT_TYPE_INSTANCES;
        iasBuildInput.instanceArray.instances = scene.d_instances;
        iasBuildInput.instanceArray.numInstances = static_cast<unsigned int>(optixInstances.size());

        OptixAccelBuildOptions iasAccelOptions{};
        iasAccelOptions.buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE;
        iasAccelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

        OptixAccelBufferSizes iasBufferSizes{};
        OPTIX_CHECK(optixAccelComputeMemoryUsage(ctx, &iasAccelOptions, &iasBuildInput, 1, &iasBufferSizes));

        CUdeviceptr d_tempIas = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_tempIas), iasBufferSizes.tempSizeInBytes));
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&scene.iasBuffer), iasBufferSizes.outputSizeInBytes));

        OPTIX_CHECK(optixAccelBuild(
            ctx,
            stream,
            &iasAccelOptions,
            &iasBuildInput,
            1,
            d_tempIas,
            iasBufferSizes.tempSizeInBytes,
            scene.iasBuffer,
            iasBufferSizes.outputSizeInBytes,
            &scene.iasHandle,
            nullptr,
            0
        ));

        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_tempIas)));

        // Setup Light CDF & DeviceLightList
        if (!hostLights.empty()) {
            std::vector<float> hostCdf(hostLights.size());
            float totalPower = 0.0f;
            for (const auto& l : hostLights) totalPower += l.power;

            float running = 0.0f;
            for (size_t i = 0; i < hostLights.size(); ++i) {
                running += hostLights[i].power;
                hostCdf[i] = (totalPower > 0.0f) ? (running / totalPower) : (static_cast<float>(i + 1) / hostLights.size());
            }
            if (!hostCdf.empty()) hostCdf.back() = 1.0f;

            size_t lightsBytes = hostLights.size() * sizeof(DeviceLight);
            size_t cdfBytes = hostCdf.size() * sizeof(float);

            CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&scene.d_lights), lightsBytes));
            CUDA_CHECK(cudaMemcpyAsync(reinterpret_cast<void*>(scene.d_lights), hostLights.data(), lightsBytes, cudaMemcpyHostToDevice, stream));

            CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&scene.d_lightCdf), cdfBytes));
            CUDA_CHECK(cudaMemcpyAsync(reinterpret_cast<void*>(scene.d_lightCdf), hostCdf.data(), cdfBytes, cudaMemcpyHostToDevice, stream));

            scene.lightList.lights = reinterpret_cast<DeviceLight*>(scene.d_lights);
            scene.lightList.cdf = reinterpret_cast<float*>(scene.d_lightCdf);
            scene.lightList.count = static_cast<unsigned int>(hostLights.size());
            scene.lightList.totalPower = totalPower;
        }

        CUDA_CHECK(cudaStreamSynchronize(stream));
        return scene;
    }

} // namespace rtx
