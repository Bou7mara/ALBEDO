#pragma once

#include <optix.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <vector>
#include <memory>
#include <unordered_map>

#include "device_material.h"
#include "device_light.h"
#include "rt/scene/scene_node.h"
#include "rt/shapes/triangle.h"
#include "rt/core/point3.h"
#include "rt/core/normal3.h"
#include "rt/core/point2.h"
#include "rt/core/vector3.h"

namespace rtx {

    struct MeshSbtData {
        const rt::Point3f* positions;
        const rt::Normal3f* normals;
        const rt::Point2f* uvs;
        const int* indices;
        int triangleCount;
        DeviceMaterial material;
    };

    struct DeviceTriangleMesh {
        CUdeviceptr d_positions = 0;
        CUdeviceptr d_normals = 0;
        CUdeviceptr d_uvs = 0;
        CUdeviceptr d_indices = 0;
        unsigned int triangleCount = 0;
        unsigned int vertexCount = 0;
        OptixTraversableHandle gasHandle = 0;
        CUdeviceptr gasBuffer = 0;
    };

    struct DeviceInstanceRecord {
        size_t meshIndex = 0;
        DeviceMaterial material{};
    };

    struct DeviceScene {
        std::vector<DeviceTriangleMesh> meshes;
        std::vector<DeviceInstanceRecord> instances;
        std::vector<CUdeviceptr> allocatedBuffers;
        OptixTraversableHandle iasHandle = 0;
        CUdeviceptr iasBuffer = 0;
        CUdeviceptr d_instances = 0;

        DeviceLightList lightList{};
        CUdeviceptr d_lights = 0;
        CUdeviceptr d_lightCdf = 0;

        DeviceScene() = default;
        ~DeviceScene();

        DeviceScene(const DeviceScene&) = delete;
        DeviceScene& operator=(const DeviceScene&) = delete;
        DeviceScene(DeviceScene&& other) noexcept;
        DeviceScene& operator=(DeviceScene&& other) noexcept;

        void Destroy();

        static DeviceScene Build(const std::shared_ptr<rt::SceneNode>& root,
                                 OptixDeviceContext ctx,
                                 CUstream stream = nullptr);
    };

    DeviceMaterial ConvertBsdfToDeviceMaterial(const rt::BSDF* bsdf);

} // namespace rtx
