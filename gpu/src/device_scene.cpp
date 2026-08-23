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
#include "rt/materials/disney_principled.h"

#include <optix_stubs.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <cstring>
#include <cmath>

namespace rtx {

    struct TextureRegistry {
        std::vector<const rt::Image2D<rt::Vector3f>*> textures3f;
        std::vector<const rt::Image2D<float>*> textures1f;

        uint32_t GetOrAdd3f(const rt::Image2D<rt::Vector3f>* tex) {
            if (!tex) return kNoTexture;
            for (uint32_t i = 0; i < textures3f.size(); ++i) {
                if (textures3f[i] == tex) return i;
            }
            textures3f.push_back(tex);
            return static_cast<uint32_t>(textures3f.size() - 1);
        }

        uint32_t GetOrAdd1f(const rt::Image2D<float>* tex) {
            if (!tex) return kNoTexture;
            for (uint32_t i = 0; i < textures1f.size(); ++i) {
                if (textures1f[i] == tex) return i;
            }
            textures1f.push_back(tex);
            return static_cast<uint32_t>(textures1f.size() - 1);
        }
    };

    DeviceMaterial ConvertBsdfToDeviceMaterial(const rt::BSDF* bsdf, TextureRegistry* registry) {
        if (!bsdf) {
            return DeviceMaterial::MakeLambertian(rt::Vector3f(0.5f, 0.5f, 0.5f));
        }
        if (auto l = dynamic_cast<const rt::Lambertian*>(bsdf)) {
            uint32_t albedoTex = registry ? registry->GetOrAdd3f(l->AlbedoTexture().get()) : kNoTexture;
            return DeviceMaterial::MakeLambertian(l->Albedo(), l->Roughness(), albedoTex);
        }
        if (auto m = dynamic_cast<const rt::Metal*>(bsdf)) {
            uint32_t albedoTex = registry ? registry->GetOrAdd3f(m->AlbedoTexture().get()) : kNoTexture;
            return DeviceMaterial::MakeMetal(m->Albedo(), albedoTex);
        }
        if (auto d = dynamic_cast<const rt::Dielectric*>(bsdf)) {
            return DeviceMaterial::MakeDielectric(d->Sellmeier(), d->Tint());
        }
        if (auto e = dynamic_cast<const rt::Emissive*>(bsdf)) {
            return DeviceMaterial::MakeEmissive(e->Radiance());
        }
        if (auto mf = dynamic_cast<const rt::Microfacet*>(bsdf)) {
            if (mf->IsDielectric()) {
                return DeviceMaterial::MakeMicrofacetDielectric(std::sqrt(mf->Alpha()), mf->Ior());
            } else {
                uint32_t roughTex = registry ? registry->GetOrAdd1f(mf->RoughnessTexture().get()) : kNoTexture;
                uint32_t tintTex = registry ? registry->GetOrAdd3f(mf->TintTexture().get()) : kNoTexture;
                return DeviceMaterial::MakeMicrofacetConductor(std::sqrt(mf->Alpha()), mf->Eta(), mf->K(), mf->Tint(), roughTex, tintTex);
            }
        }
        if (auto dp = dynamic_cast<const rt::DisneyPrincipled*>(bsdf)) {
            const auto& p = dp->Params();
            uint32_t baseTex = registry ? registry->GetOrAdd3f(p.baseColorTexture.get()) : kNoTexture;
            uint32_t roughTex = registry ? registry->GetOrAdd1f(p.roughnessTexture.get()) : kNoTexture;
            uint32_t metalTex = registry ? registry->GetOrAdd1f(p.metallicTexture.get()) : kNoTexture;
            return DeviceMaterial::MakeDisney(
                p.baseColor, p.metallic, p.subsurface, p.specular, p.roughness,
                p.specularTint, p.anisotropic, p.sheen, p.sheenTint, p.clearcoat, p.clearcoatGloss,
                baseTex, roughTex, metalTex
            );
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
          d_lightCdf(other.d_lightCdf),
          textureList(other.textureList),
          d_textures3fViews(other.d_textures3fViews),
          d_textures1fViews(other.d_textures1fViews) {
        other.iasHandle = 0;
        other.iasBuffer = 0;
        other.d_instances = 0;
        other.d_lights = 0;
        other.d_lightCdf = 0;
        other.lightList = DeviceLightList{};
        other.d_textures3fViews = 0;
        other.d_textures1fViews = 0;
        other.textureList = DeviceTextureList{};
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
            textureList = other.textureList;
            d_textures3fViews = other.d_textures3fViews;
            d_textures1fViews = other.d_textures1fViews;

            other.iasHandle = 0;
            other.iasBuffer = 0;
            other.d_instances = 0;
            other.d_lights = 0;
            other.d_lightCdf = 0;
            other.lightList = DeviceLightList{};
            other.d_textures3fViews = 0;
            other.d_textures1fViews = 0;
            other.textureList = DeviceTextureList{};
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
        if (d_textures3fViews) {
            cudaFree(reinterpret_cast<void*>(d_textures3fViews));
            d_textures3fViews = 0;
        }
        if (d_textures1fViews) {
            cudaFree(reinterpret_cast<void*>(d_textures1fViews));
            d_textures1fViews = 0;
        }
        iasHandle = 0;
        lightList = DeviceLightList{};
        textureList = DeviceTextureList{};
    }

    DeviceScene DeviceScene::Build(const std::shared_ptr<rt::SceneNode>& root,
                                   OptixDeviceContext ctx,
                                   CUstream stream) {
        DeviceScene scene;
        if (!root) return scene;

        std::unordered_map<const rt::TriangleMesh*, size_t> meshCache;
        std::vector<OptixInstance> optixInstances;
        std::vector<DeviceLight> hostLights;
        TextureRegistry textureRegistry;

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

                    DeviceMaterial mat = ConvertBsdfToDeviceMaterial(node->bsdf.get(), &textureRegistry);
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

        // Upload 3D vector textures (RGB)
        if (!textureRegistry.textures3f.empty()) {
            std::vector<rt::Image2DView<rt::Vector3f>> hostViews3f;
            for (const auto* tex : textureRegistry.textures3f) {
                size_t byteSize = tex->texels.size() * sizeof(rt::Vector3f);
                CUdeviceptr d_data = 0;
                CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_data), byteSize));
                CUDA_CHECK(cudaMemcpyAsync(reinterpret_cast<void*>(d_data), tex->texels.data(), byteSize, cudaMemcpyHostToDevice, stream));
                scene.allocatedBuffers.push_back(d_data);
                hostViews3f.emplace_back(reinterpret_cast<const rt::Vector3f*>(d_data), tex->width, tex->height, tex->wrap);
            }
            size_t viewsByteSize = hostViews3f.size() * sizeof(rt::Image2DView<rt::Vector3f>);
            CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&scene.d_textures3fViews), viewsByteSize));
            CUDA_CHECK(cudaMemcpyAsync(reinterpret_cast<void*>(scene.d_textures3fViews), hostViews3f.data(), viewsByteSize, cudaMemcpyHostToDevice, stream));
            scene.textureList.textures3f = reinterpret_cast<const rt::Image2DView<rt::Vector3f>*>(scene.d_textures3fViews);
            scene.textureList.count3f = static_cast<unsigned int>(hostViews3f.size());
        }

        // Upload 1D scalar textures (float)
        if (!textureRegistry.textures1f.empty()) {
            std::vector<rt::Image2DView<float>> hostViews1f;
            for (const auto* tex : textureRegistry.textures1f) {
                size_t byteSize = tex->texels.size() * sizeof(float);
                CUdeviceptr d_data = 0;
                CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_data), byteSize));
                CUDA_CHECK(cudaMemcpyAsync(reinterpret_cast<void*>(d_data), tex->texels.data(), byteSize, cudaMemcpyHostToDevice, stream));
                scene.allocatedBuffers.push_back(d_data);
                hostViews1f.emplace_back(reinterpret_cast<const float*>(d_data), tex->width, tex->height, tex->wrap);
            }
            size_t viewsByteSize = hostViews1f.size() * sizeof(rt::Image2DView<float>);
            CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&scene.d_textures1fViews), viewsByteSize));
            CUDA_CHECK(cudaMemcpyAsync(reinterpret_cast<void*>(scene.d_textures1fViews), hostViews1f.data(), viewsByteSize, cudaMemcpyHostToDevice, stream));
            scene.textureList.textures1f = reinterpret_cast<const rt::Image2DView<float>*>(scene.d_textures1fViews);
            scene.textureList.count1f = static_cast<unsigned int>(hostViews1f.size());
        }

        // Upload Directional Albedo Energy Compensation LUTs
        const auto& lut = rt::GetDirectionalAlbedoLUT();
        size_t lutBytes = lut.eTable.texels.size() * sizeof(float);
        CUdeviceptr d_energyLut = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_energyLut), lutBytes));
        CUDA_CHECK(cudaMemcpyAsync(reinterpret_cast<void*>(d_energyLut), lut.eTable.texels.data(), lutBytes, cudaMemcpyHostToDevice, stream));
        scene.allocatedBuffers.push_back(d_energyLut);
        scene.textureList.energyLut = rt::Image2DView<float>(reinterpret_cast<const float*>(d_energyLut), lut.eTable.width, lut.eTable.height, lut.eTable.wrap);

        size_t avgLutBytes = lut.eAvgTable.texels.size() * sizeof(float);
        CUdeviceptr d_energyAvgLut = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_energyAvgLut), avgLutBytes));
        CUDA_CHECK(cudaMemcpyAsync(reinterpret_cast<void*>(d_energyAvgLut), lut.eAvgTable.texels.data(), avgLutBytes, cudaMemcpyHostToDevice, stream));
        scene.allocatedBuffers.push_back(d_energyAvgLut);
        scene.textureList.energyAvgLut = rt::Image2DView<float>(reinterpret_cast<const float*>(d_energyAvgLut), lut.eAvgTable.width, lut.eAvgTable.height, lut.eAvgTable.wrap);

        CUDA_CHECK(cudaStreamSynchronize(stream));
        return scene;
    }

} // namespace rtx
