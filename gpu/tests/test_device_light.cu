#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "device_light.h"
#include <cuda_runtime.h>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace {

    __global__ void TestSampleLightKernel(rtx::DeviceLightList list,
                                          const float* uVals,
                                          int* lightIndices,
                                          float* pmfs,
                                          int count) {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= count) return;

        rtx::SampleLightDevice(list, uVals[idx], &lightIndices[idx], &pmfs[idx]);
    }

}

TEST_CASE("Device light CDF sampling parity", "[gpu][light][cdf]") {
    std::vector<rtx::DeviceLight> hostLights(3);
    hostLights[0].instanceIndex = 0; hostLights[0].power = 10.0f;
    hostLights[1].instanceIndex = 1; hostLights[1].power = 30.0f;
    hostLights[2].instanceIndex = 2; hostLights[2].power = 60.0f;

    std::vector<float> hostCdf = { 0.1f, 0.4f, 1.0f };

    rtx::DeviceLightList list{};
    list.lights = hostLights.data();
    list.cdf = hostCdf.data();
    list.count = 3;
    list.totalPower = 100.0f;

    std::vector<float> testU = { 0.05f, 0.1f, 0.25f, 0.4f, 0.7f, 0.99f };
    std::vector<int> expectedIndices = { 0, 0, 1, 1, 2, 2 };
    std::vector<float> expectedPmfs = { 0.1f, 0.1f, 0.3f, 0.3f, 0.6f, 0.6f };

    for (size_t i = 0; i < testU.size(); ++i) {
        int lightIdx = -1;
        float pmf = 0.0f;
        const auto* l = rtx::SampleLightDevice(list, testU[i], &lightIdx, &pmf);
        REQUIRE(l != nullptr);
        REQUIRE(lightIdx == expectedIndices[i]);
        REQUIRE_THAT(pmf, WithinAbs(expectedPmfs[i], 1e-5f));
    }

    int count = static_cast<int>(testU.size());
    rtx::DeviceLight* d_lights = nullptr;
    float* d_cdf = nullptr;
    float* d_u = nullptr;
    int* d_indices = nullptr;
    float* d_pmfs = nullptr;

    cudaMalloc(&d_lights, hostLights.size() * sizeof(rtx::DeviceLight));
    cudaMalloc(&d_cdf, hostCdf.size() * sizeof(float));
    cudaMalloc(&d_u, count * sizeof(float));
    cudaMalloc(&d_indices, count * sizeof(int));
    cudaMalloc(&d_pmfs, count * sizeof(float));

    cudaMemcpy(d_lights, hostLights.data(), hostLights.size() * sizeof(rtx::DeviceLight), cudaMemcpyHostToDevice);
    cudaMemcpy(d_cdf, hostCdf.data(), hostCdf.size() * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_u, testU.data(), count * sizeof(float), cudaMemcpyHostToDevice);

    rtx::DeviceLightList devList{};
    devList.lights = d_lights;
    devList.cdf = d_cdf;
    devList.count = 3;
    devList.totalPower = 100.0f;

    TestSampleLightKernel<<<1, count>>>(devList, d_u, d_indices, d_pmfs, count);
    cudaDeviceSynchronize();

    std::vector<int> devIndices(count);
    std::vector<float> devPmfs(count);
    cudaMemcpy(devIndices.data(), d_indices, count * sizeof(int), cudaMemcpyDeviceToHost);
    cudaMemcpy(devPmfs.data(), d_pmfs, count * sizeof(float), cudaMemcpyDeviceToHost);

    for (int i = 0; i < count; ++i) {
        REQUIRE(devIndices[i] == expectedIndices[i]);
        REQUIRE_THAT(devPmfs[i], WithinAbs(expectedPmfs[i], 1e-5f));
    }

    cudaFree(d_lights);
    cudaFree(d_cdf);
    cudaFree(d_u);
    cudaFree(d_indices);
    cudaFree(d_pmfs);
}
