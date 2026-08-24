#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "rt/textures/image2d.h"
#include "rt/core/vector3.h"

#include <cuda_runtime.h>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace {

    __global__ void SampleTextureKernel(rt::Image2DView<float> view,
                                        const float* uCoords,
                                        const float* vCoords,
                                        float* outResults,
                                        int numSamples) {
        int idx = blockDim.x * blockIdx.x + threadIdx.x;
        if (idx < numSamples) {
            outResults[idx] = view.Sample(uCoords[idx], vCoords[idx]);
        }
    }

    __global__ void SampleVectorTextureKernel(rt::Image2DView<rt::Vector3f> view,
                                              const float* uCoords,
                                              const float* vCoords,
                                              rt::Vector3f* outResults,
                                              int numSamples) {
        int idx = blockDim.x * blockIdx.x + threadIdx.x;
        if (idx < numSamples) {
            outResults[idx] = view.Sample(uCoords[idx], vCoords[idx]);
        }
    }

}

TEST_CASE("Image2DView host/device scalar sampling parity", "[gpu][texture][parity]") {

    int w = 4;
    int h = 4;
    std::vector<float> hostData(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            hostData[y * w + x] = static_cast<float>((x + y) % 2);
        }
    }

    float* d_data = nullptr;
    cudaMalloc(&d_data, hostData.size() * sizeof(float));
    cudaMemcpy(d_data, hostData.data(), hostData.size() * sizeof(float), cudaMemcpyHostToDevice);

    std::vector<float> hostU;
    std::vector<float> hostV;
    for (float u = -0.5f; u <= 1.5f; u += 0.1f) {
        for (float v = -0.5f; v <= 1.5f; v += 0.1f) {
            hostU.push_back(u);
            hostV.push_back(v);
        }
    }
    int numSamples = static_cast<int>(hostU.size());

    float* d_u = nullptr;
    float* d_v = nullptr;
    float* d_out = nullptr;
    cudaMalloc(&d_u, numSamples * sizeof(float));
    cudaMalloc(&d_v, numSamples * sizeof(float));
    cudaMalloc(&d_out, numSamples * sizeof(float));

    cudaMemcpy(d_u, hostU.data(), numSamples * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_v, hostV.data(), numSamples * sizeof(float), cudaMemcpyHostToDevice);

    for (rt::WrapMode mode : { rt::WrapMode::Clamp, rt::WrapMode::Repeat }) {
        rt::Image2DView<float> hostView(hostData.data(), w, h, mode);
        rt::Image2DView<float> devView(d_data, w, h, mode);

        int blockSize = 256;
        int numBlocks = (numSamples + blockSize - 1) / blockSize;
        SampleTextureKernel<<<numBlocks, blockSize>>>(devView, d_u, d_v, d_out, numSamples);
        cudaDeviceSynchronize();

        std::vector<float> devResults(numSamples);
        cudaMemcpy(devResults.data(), d_out, numSamples * sizeof(float), cudaMemcpyDeviceToHost);

        for (int i = 0; i < numSamples; ++i) {
            float hostVal = hostView.Sample(hostU[i], hostV[i]);
            float devVal = devResults[i];
            REQUIRE_THAT(devVal, WithinAbs(hostVal, 1e-5f));
        }
    }

    cudaFree(d_data);
    cudaFree(d_u);
    cudaFree(d_v);
    cudaFree(d_out);
}

TEST_CASE("Image2DView host/device Vector3f sampling parity", "[gpu][texture][vector][parity]") {
    int w = 3;
    int h = 3;
    std::vector<rt::Vector3f> hostData(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            hostData[y * w + x] = rt::Vector3f(static_cast<float>(x) * 0.5f, static_cast<float>(y) * 0.5f, 0.75f);
        }
    }

    rt::Vector3f* d_data = nullptr;
    cudaMalloc(&d_data, hostData.size() * sizeof(rt::Vector3f));
    cudaMemcpy(d_data, hostData.data(), hostData.size() * sizeof(rt::Vector3f), cudaMemcpyHostToDevice);

    std::vector<float> hostU = { 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, -0.2f, 1.3f };
    std::vector<float> hostV = { 0.1f, 0.35f, 0.5f, 0.65f, 0.9f, -0.4f, 1.4f };
    int numSamples = static_cast<int>(hostU.size());

    float* d_u = nullptr;
    float* d_v = nullptr;
    rt::Vector3f* d_out = nullptr;
    cudaMalloc(&d_u, numSamples * sizeof(float));
    cudaMalloc(&d_v, numSamples * sizeof(float));
    cudaMalloc(&d_out, numSamples * sizeof(rt::Vector3f));

    cudaMemcpy(d_u, hostU.data(), numSamples * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_v, hostV.data(), numSamples * sizeof(float), cudaMemcpyHostToDevice);

    for (rt::WrapMode mode : { rt::WrapMode::Clamp, rt::WrapMode::Repeat }) {
        rt::Image2DView<rt::Vector3f> hostView(hostData.data(), w, h, mode);
        rt::Image2DView<rt::Vector3f> devView(d_data, w, h, mode);

        SampleVectorTextureKernel<<<1, numSamples>>>(devView, d_u, d_v, d_out, numSamples);
        cudaDeviceSynchronize();

        std::vector<rt::Vector3f> devResults(numSamples);
        cudaMemcpy(devResults.data(), d_out, numSamples * sizeof(rt::Vector3f), cudaMemcpyDeviceToHost);

        for (int i = 0; i < numSamples; ++i) {
            rt::Vector3f hostVal = hostView.Sample(hostU[i], hostV[i]);
            rt::Vector3f devVal = devResults[i];
            REQUIRE_THAT(devVal.x, WithinAbs(hostVal.x, 1e-5f));
            REQUIRE_THAT(devVal.y, WithinAbs(hostVal.y, 1e-5f));
            REQUIRE_THAT(devVal.z, WithinAbs(hostVal.z, 1e-5f));
        }
    }

    cudaFree(d_data);
    cudaFree(d_u);
    cudaFree(d_v);
    cudaFree(d_out);
}
