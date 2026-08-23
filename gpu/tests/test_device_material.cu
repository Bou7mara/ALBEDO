#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "device_material.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/dielectric.h"
#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/disney_principled.h"
#include "rt/materials/energy_compensation.h"

#include <cuda_runtime.h>
#include <cmath>

using Catch::Matchers::WithinAbs;

namespace {

    struct MaterialTestInput {
        rtx::DeviceMaterial mat;
        rt::Vector3f wo;
        rt::Vector3f n;
        rt::Point2f u;
        rt::Vector3f wiEval;
        rt::Point2f uv;
        rtx::DeviceTextureList textures;
    };

    struct MaterialTestOutput {
        rt::Vector3f sampleF;
        float samplePdf;
        rt::Vector3f sampledWi;
        rt::Vector3f f;
        float evalPdf;
    };

    __global__ void RunMaterialDeviceTests(const MaterialTestInput* inputs, MaterialTestOutput* outputs, int count) {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= count) return;

        const MaterialTestInput& in = inputs[idx];
        MaterialTestOutput& out = outputs[idx];

        out.sampleF = rtx::SampleBsdf(in.mat, in.wo, in.n, in.u, &out.sampledWi, &out.samplePdf, in.uv, &in.textures);
        out.f = rtx::EvaluateBsdf(in.mat, in.wo, in.wiEval, in.n, in.uv, &in.textures);
        out.evalPdf = rtx::PdfBsdf(in.mat, in.wo, in.wiEval, in.n, in.uv, &in.textures);
    }

} // namespace

TEST_CASE("Lambertian host/device parity with CPU BSDF", "[gpu][material][lambertian]") {
    rt::Vector3f albedo(0.7f, 0.2f, 0.3f);
    rt::Lambertian cpuMat(albedo);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeLambertian(albedo);

    rt::Vector3f wo = Normalize(rt::Vector3f(0.1f, 0.2f, 0.9f));
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Point2f u(0.25f, 0.75f);
    rt::Vector3f wiEval = Normalize(rt::Vector3f(0.3f, 0.4f, 0.8f));

    rt::Vector3f cpuSampledWi;
    float cpuSamplePdf = 0.0f;
    rt::Vector3f cpuSampleF = cpuMat.Sample_f(wo, n, u, &cpuSampledWi, &cpuSamplePdf);

    rt::Vector3f hostSampledWi;
    float hostSamplePdf = 0.0f;
    rt::Vector3f hostSampleF = rtx::SampleBsdf(devMat, wo, n, u, &hostSampledWi, &hostSamplePdf);

    REQUIRE_THAT(hostSampleF.x, WithinAbs(cpuSampleF.x, 1e-5f));
    REQUIRE_THAT(hostSampleF.y, WithinAbs(cpuSampleF.y, 1e-5f));
    REQUIRE_THAT(hostSampleF.z, WithinAbs(cpuSampleF.z, 1e-5f));
    REQUIRE_THAT(hostSamplePdf, WithinAbs(cpuSamplePdf, 1e-5f));
    REQUIRE_THAT(hostSampledWi.x, WithinAbs(cpuSampledWi.x, 1e-5f));
    REQUIRE_THAT(hostSampledWi.y, WithinAbs(cpuSampledWi.y, 1e-5f));
    REQUIRE_THAT(hostSampledWi.z, WithinAbs(cpuSampledWi.z, 1e-5f));

    rt::Vector3f cpuF = cpuMat.f(wo, wiEval, n);
    float cpuEvalPdf = cpuMat.Pdf(wo, wiEval, n);

    rt::Vector3f hostF = rtx::EvaluateBsdf(devMat, wo, wiEval, n);
    float hostEvalPdf = rtx::PdfBsdf(devMat, wo, wiEval, n);

    REQUIRE_THAT(hostF.x, WithinAbs(cpuF.x, 1e-5f));
    REQUIRE_THAT(hostF.y, WithinAbs(cpuF.y, 1e-5f));
    REQUIRE_THAT(hostF.z, WithinAbs(cpuF.z, 1e-5f));
    REQUIRE_THAT(hostEvalPdf, WithinAbs(cpuEvalPdf, 1e-5f));

    MaterialTestInput input{ devMat, wo, n, u, wiEval, rt::Point2f(0.0f, 0.0f), rtx::DeviceTextureList{} };
    MaterialTestInput* d_in = nullptr;
    MaterialTestOutput* d_out = nullptr;
    cudaMalloc(&d_in, sizeof(MaterialTestInput));
    cudaMalloc(&d_out, sizeof(MaterialTestOutput));
    cudaMemcpy(d_in, &input, sizeof(MaterialTestInput), cudaMemcpyHostToDevice);

    RunMaterialDeviceTests<<<1, 1>>>(d_in, d_out, 1);
    cudaDeviceSynchronize();

    MaterialTestOutput devOut{};
    cudaMemcpy(&devOut, d_out, sizeof(MaterialTestOutput), cudaMemcpyDeviceToHost);

    REQUIRE_THAT(devOut.sampleF.x, WithinAbs(cpuSampleF.x, 1e-5f));
    REQUIRE_THAT(devOut.sampleF.y, WithinAbs(cpuSampleF.y, 1e-5f));
    REQUIRE_THAT(devOut.sampleF.z, WithinAbs(cpuSampleF.z, 1e-5f));
    REQUIRE_THAT(devOut.samplePdf, WithinAbs(cpuSamplePdf, 1e-5f));
    REQUIRE_THAT(devOut.sampledWi.x, WithinAbs(cpuSampledWi.x, 1e-5f));
    REQUIRE_THAT(devOut.sampledWi.y, WithinAbs(cpuSampledWi.y, 1e-5f));
    REQUIRE_THAT(devOut.sampledWi.z, WithinAbs(cpuSampledWi.z, 1e-5f));

    REQUIRE_THAT(devOut.f.x, WithinAbs(cpuF.x, 1e-5f));
    REQUIRE_THAT(devOut.f.y, WithinAbs(cpuF.y, 1e-5f));
    REQUIRE_THAT(devOut.f.z, WithinAbs(cpuF.z, 1e-5f));
    REQUIRE_THAT(devOut.evalPdf, WithinAbs(cpuEvalPdf, 1e-5f));

    cudaFree(d_in);
    cudaFree(d_out);
}

TEST_CASE("Lambertian Oren-Nayar roughness evaluation parity", "[gpu][material][lambertian]") {
    rt::Vector3f albedo(0.8f, 0.7f, 0.6f);
    float roughness = 0.5f;
    rt::Lambertian cpuMat(albedo, roughness);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeLambertian(albedo, roughness);

    rt::Vector3f wo = Normalize(rt::Vector3f(0.5f, 0.2f, 0.8f));
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Vector3f wiEval = Normalize(rt::Vector3f(-0.3f, 0.4f, 0.8f));

    rt::Vector3f cpuF = cpuMat.f(wo, wiEval, n);
    rt::Vector3f hostF = rtx::EvaluateBsdf(devMat, wo, wiEval, n);

    REQUIRE_THAT(hostF.x, WithinAbs(cpuF.x, 1e-5f));
    REQUIRE_THAT(hostF.y, WithinAbs(cpuF.y, 1e-5f));
    REQUIRE_THAT(hostF.z, WithinAbs(cpuF.z, 1e-5f));

    MaterialTestInput input{ devMat, wo, n, rt::Point2f(0.5f, 0.5f), wiEval, rt::Point2f(0.0f, 0.0f), rtx::DeviceTextureList{} };
    MaterialTestInput* d_in = nullptr;
    MaterialTestOutput* d_out = nullptr;
    cudaMalloc(&d_in, sizeof(MaterialTestInput));
    cudaMalloc(&d_out, sizeof(MaterialTestOutput));
    cudaMemcpy(d_in, &input, sizeof(MaterialTestInput), cudaMemcpyHostToDevice);

    RunMaterialDeviceTests<<<1, 1>>>(d_in, d_out, 1);
    cudaDeviceSynchronize();

    MaterialTestOutput devOut{};
    cudaMemcpy(&devOut, d_out, sizeof(MaterialTestOutput), cudaMemcpyDeviceToHost);

    REQUIRE_THAT(devOut.f.x, WithinAbs(cpuF.x, 1e-5f));
    REQUIRE_THAT(devOut.f.y, WithinAbs(cpuF.y, 1e-5f));
    REQUIRE_THAT(devOut.f.z, WithinAbs(cpuF.z, 1e-5f));

    cudaFree(d_in);
    cudaFree(d_out);
}

TEST_CASE("Dielectric reflection and refraction parity", "[gpu][material][dielectric]") {
    rt::Vector3f tint(0.9f, 0.95f, 1.0f);
    float ior = 1.5f;
    rt::Dielectric cpuMat(ior, tint);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeDielectric(ior, tint);

    rt::Vector3f wo = Normalize(rt::Vector3f(0.2f, 0.3f, 0.9f));
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Point2f u(0.1f, 0.5f); // choose refraction branch (u.x > Fr)

    rt::Vector3f cpuWi;
    float cpuPdf = 0.0f;
    rt::Vector3f cpuSampleF = cpuMat.Sample_f(wo, n, u, &cpuWi, &cpuPdf);

    rt::Vector3f hostWi;
    float hostPdf = 0.0f;
    rt::Vector3f hostSampleF = rtx::SampleBsdf(devMat, wo, n, u, &hostWi, &hostPdf);

    REQUIRE_THAT(hostSampleF.x, WithinAbs(cpuSampleF.x, 1e-4f));
    REQUIRE_THAT(hostSampleF.y, WithinAbs(cpuSampleF.y, 1e-4f));
    REQUIRE_THAT(hostSampleF.z, WithinAbs(cpuSampleF.z, 1e-4f));
    REQUIRE_THAT(hostPdf, WithinAbs(cpuPdf, 1e-4f));
    REQUIRE_THAT(hostWi.x, WithinAbs(cpuWi.x, 1e-4f));
    REQUIRE_THAT(hostWi.y, WithinAbs(cpuWi.y, 1e-4f));
    REQUIRE_THAT(hostWi.z, WithinAbs(cpuWi.z, 1e-4f));
}

TEST_CASE("Dielectric TIR and dispersion parity", "[gpu][material][dielectric]") {
    rt::Vector3f tint(0.9f, 0.95f, 1.0f);
    float ior = 1.5f;
    float dispersion = 0.02f;
    rt::Dielectric cpuMat(ior, tint, dispersion);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeDielectric(ior, tint, dispersion);

    rt::Vector3f woTir = Normalize(rt::Vector3f(0.95f, 0.0f, 0.1f)); // grazing inside -> TIR
    rt::Vector3f n(0.0f, 0.0f, -1.0f); // exiting interface
    rt::Point2f u(0.99f, 0.1f); // choose transmission branch -> triggers TIR fallback

    rt::Vector3f cpuWi;
    float cpuPdf = 0.0f;
    rt::Vector3f cpuSampleF = cpuMat.Sample_f(woTir, n, u, &cpuWi, &cpuPdf);

    rt::Vector3f hostWi;
    float hostPdf = 0.0f;
    rt::Vector3f hostSampleF = rtx::SampleBsdf(devMat, woTir, n, u, &hostWi, &hostPdf);

    REQUIRE_THAT(hostSampleF.x, WithinAbs(cpuSampleF.x, 1e-4f));
    REQUIRE_THAT(hostWi.x, WithinAbs(cpuWi.x, 1e-4f));
    REQUIRE_THAT(hostWi.y, WithinAbs(cpuWi.y, 1e-4f));
    REQUIRE_THAT(hostWi.z, WithinAbs(cpuWi.z, 1e-4f));
}

TEST_CASE("Metal host/device parity with CPU BSDF", "[gpu][material][metal]") {
    rt::Vector3f albedo(0.9f, 0.85f, 0.75f);
    rt::Metal cpuMat(albedo);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeMetal(albedo);

    rt::Vector3f wo = Normalize(rt::Vector3f(0.3f, 0.4f, 0.8f));
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Point2f u(0.35f, 0.65f);
    rt::Vector3f wiEval = Normalize(rt::Vector3f(0.2f, -0.3f, 0.9f));

    rt::Vector3f cpuSampledWi;
    float cpuSamplePdf = 0.0f;
    rt::Vector3f cpuSampleF = cpuMat.Sample_f(wo, n, u, &cpuSampledWi, &cpuSamplePdf);

    rt::Vector3f hostSampledWi;
    float hostSamplePdf = 0.0f;
    rt::Vector3f hostSampleF = rtx::SampleBsdf(devMat, wo, n, u, &hostSampledWi, &hostSamplePdf);

    REQUIRE_THAT(hostSampleF.x, WithinAbs(cpuSampleF.x, 1e-5f));
    REQUIRE_THAT(hostSampleF.y, WithinAbs(cpuSampleF.y, 1e-5f));
    REQUIRE_THAT(hostSampleF.z, WithinAbs(cpuSampleF.z, 1e-5f));
    REQUIRE_THAT(hostSamplePdf, WithinAbs(cpuSamplePdf, 1e-5f));
    REQUIRE_THAT(hostSampledWi.x, WithinAbs(cpuSampledWi.x, 1e-5f));
    REQUIRE_THAT(hostSampledWi.y, WithinAbs(cpuSampledWi.y, 1e-5f));
    REQUIRE_THAT(hostSampledWi.z, WithinAbs(cpuSampledWi.z, 1e-5f));

    MaterialTestInput input{ devMat, wo, n, u, wiEval, rt::Point2f(0.0f, 0.0f), rtx::DeviceTextureList{} };
    MaterialTestInput* d_in = nullptr;
    MaterialTestOutput* d_out = nullptr;
    cudaMalloc(&d_in, sizeof(MaterialTestInput));
    cudaMalloc(&d_out, sizeof(MaterialTestOutput));
    cudaMemcpy(d_in, &input, sizeof(MaterialTestInput), cudaMemcpyHostToDevice);

    RunMaterialDeviceTests<<<1, 1>>>(d_in, d_out, 1);
    cudaDeviceSynchronize();

    MaterialTestOutput devOut{};
    cudaMemcpy(&devOut, d_out, sizeof(MaterialTestOutput), cudaMemcpyDeviceToHost);

    REQUIRE_THAT(devOut.sampleF.x, WithinAbs(cpuSampleF.x, 1e-5f));
    REQUIRE_THAT(devOut.sampleF.y, WithinAbs(cpuSampleF.y, 1e-5f));
    REQUIRE_THAT(devOut.sampleF.z, WithinAbs(cpuSampleF.z, 1e-5f));
    REQUIRE_THAT(devOut.samplePdf, WithinAbs(cpuSamplePdf, 1e-5f));
    REQUIRE_THAT(devOut.sampledWi.x, WithinAbs(cpuSampledWi.x, 1e-5f));
    REQUIRE_THAT(devOut.sampledWi.y, WithinAbs(cpuSampledWi.y, 1e-5f));
    REQUIRE_THAT(devOut.sampledWi.z, WithinAbs(cpuSampledWi.z, 1e-5f));

    cudaFree(d_in);
    cudaFree(d_out);
}


TEST_CASE("Disney Principled host/device parity with CPU BSDF", "[gpu][material][disney]") {
    rt::DisneyParams params{};
    params.baseColor = rt::Vector3f(0.8f, 0.5f, 0.2f);
    params.metallic = 0.3f;
    params.subsurface = 0.2f;
    params.specular = 0.6f;
    params.roughness = 0.4f;
    params.sheen = 0.5f;
    params.clearcoat = 0.8f;
    params.clearcoatGloss = 0.9f;

    rt::DisneyPrincipled cpuMat(params);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeDisney(
        params.baseColor, params.metallic, params.subsurface, params.specular, params.roughness,
        params.specularTint, params.anisotropic, params.sheen, params.sheenTint, params.clearcoat, params.clearcoatGloss
    );

    rt::Vector3f wo = Normalize(rt::Vector3f(0.3f, 0.4f, 0.8f));
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Vector3f wiEval = Normalize(rt::Vector3f(0.2f, -0.3f, 0.9f));

    rt::Vector3f cpuF = cpuMat.f(wo, wiEval, n);
    float cpuEvalPdf = cpuMat.Pdf(wo, wiEval, n);

    rt::Vector3f hostF = rtx::EvaluateBsdf(devMat, wo, wiEval, n);
    float hostEvalPdf = rtx::PdfBsdf(devMat, wo, wiEval, n);

    REQUIRE_THAT(hostF.x, WithinAbs(cpuF.x, 1e-4f));
    REQUIRE_THAT(hostF.y, WithinAbs(cpuF.y, 1e-4f));
    REQUIRE_THAT(hostF.z, WithinAbs(cpuF.z, 1e-4f));
    REQUIRE_THAT(hostEvalPdf, WithinAbs(cpuEvalPdf, 1e-4f));

    rt::Point2f u(0.35f, 0.65f);
    MaterialTestInput input{ devMat, wo, n, u, wiEval, rt::Point2f(0.0f, 0.0f), rtx::DeviceTextureList{} };
    MaterialTestInput* d_in = nullptr;
    MaterialTestOutput* d_out = nullptr;
    cudaMalloc(&d_in, sizeof(MaterialTestInput));
    cudaMalloc(&d_out, sizeof(MaterialTestOutput));
    cudaMemcpy(d_in, &input, sizeof(MaterialTestInput), cudaMemcpyHostToDevice);

    RunMaterialDeviceTests<<<1, 1>>>(d_in, d_out, 1);
    cudaDeviceSynchronize();

    MaterialTestOutput devOut{};
    cudaMemcpy(&devOut, d_out, sizeof(MaterialTestOutput), cudaMemcpyDeviceToHost);

    REQUIRE_THAT(devOut.f.x, WithinAbs(cpuF.x, 1e-4f));
    REQUIRE_THAT(devOut.f.y, WithinAbs(cpuF.y, 1e-4f));
    REQUIRE_THAT(devOut.f.z, WithinAbs(cpuF.z, 1e-4f));
    REQUIRE_THAT(devOut.evalPdf, WithinAbs(cpuEvalPdf, 1e-4f));

    cudaFree(d_in);
    cudaFree(d_out);
}

TEST_CASE("Textured material host/device parity with CPU BSDF", "[gpu][material][texture][parity]") {
    // 2x2 color texture
    auto colorTex = std::make_shared<rt::Image2D<rt::Vector3f>>(2, 2, rt::WrapMode::Clamp);
    colorTex->Set(0, 0, rt::Vector3f(0.9f, 0.1f, 0.1f));
    colorTex->Set(1, 0, rt::Vector3f(0.1f, 0.9f, 0.1f));
    colorTex->Set(0, 1, rt::Vector3f(0.1f, 0.1f, 0.9f));
    colorTex->Set(1, 1, rt::Vector3f(0.8f, 0.8f, 0.2f));

    // Upload texture to GPU
    rt::Vector3f* d_texData = nullptr;
    cudaMalloc(&d_texData, colorTex->texels.size() * sizeof(rt::Vector3f));
    cudaMemcpy(d_texData, colorTex->texels.data(), colorTex->texels.size() * sizeof(rt::Vector3f), cudaMemcpyHostToDevice);

    rt::Image2DView<rt::Vector3f> hostView(colorTex->texels.data(), colorTex->width, colorTex->height, colorTex->wrap);
    rt::Image2DView<rt::Vector3f> devView(d_texData, colorTex->width, colorTex->height, colorTex->wrap);

    rt::Image2DView<rt::Vector3f>* d_views = nullptr;
    cudaMalloc(&d_views, sizeof(rt::Image2DView<rt::Vector3f>));
    cudaMemcpy(d_views, &devView, sizeof(rt::Image2DView<rt::Vector3f>), cudaMemcpyHostToDevice);

    rtx::DeviceTextureList hostTexList{ &hostView, 1, nullptr, 0 };
    rtx::DeviceTextureList devTexList{ d_views, 1, nullptr, 0 };

    rt::Lambertian cpuMat(colorTex);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeLambertian(rt::Vector3f(1.0f, 1.0f, 1.0f), 0.0f, 0);

    rt::Vector3f wo(0.0f, 0.0f, 1.0f);
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Vector3f wiEval(0.0f, 0.0f, 1.0f);
    rt::Point2f uv(0.25f, 0.75f); // blue tile center

    rt::Vector3f cpuF = cpuMat.f(wo, wiEval, n, uv);
    rt::Vector3f hostF = rtx::EvaluateBsdf(devMat, wo, wiEval, n, uv, &hostTexList);

    REQUIRE_THAT(hostF.x, WithinAbs(cpuF.x, 1e-5f));
    REQUIRE_THAT(hostF.y, WithinAbs(cpuF.y, 1e-5f));
    REQUIRE_THAT(hostF.z, WithinAbs(cpuF.z, 1e-5f));

    MaterialTestInput input{ devMat, wo, n, rt::Point2f(0.5f, 0.5f), wiEval, uv, devTexList };
    MaterialTestInput* d_in = nullptr;
    MaterialTestOutput* d_out = nullptr;
    cudaMalloc(&d_in, sizeof(MaterialTestInput));
    cudaMalloc(&d_out, sizeof(MaterialTestOutput));
    cudaMemcpy(d_in, &input, sizeof(MaterialTestInput), cudaMemcpyHostToDevice);

    RunMaterialDeviceTests<<<1, 1>>>(d_in, d_out, 1);
    cudaDeviceSynchronize();

    MaterialTestOutput devOut{};
    cudaMemcpy(&devOut, d_out, sizeof(MaterialTestOutput), cudaMemcpyDeviceToHost);

    REQUIRE_THAT(devOut.f.x, WithinAbs(cpuF.x, 1e-5f));
    REQUIRE_THAT(devOut.f.y, WithinAbs(cpuF.y, 1e-5f));
    REQUIRE_THAT(devOut.f.z, WithinAbs(cpuF.z, 1e-5f));

    cudaFree(d_texData);
    cudaFree(d_views);
    cudaFree(d_in);
    cudaFree(d_out);
}

TEST_CASE("Energy-compensated Microfacet Conductor host/device parity with CPU BSDF", "[gpu][material][energy_compensation][parity]") {
    float roughness = 0.8f;
    rt::Vector3f eta(0.2f, 0.9f, 1.1f);
    rt::Vector3f k(3.5f, 2.5f, 1.8f);
    rt::Vector3f tint(0.95f, 0.65f, 0.4f);

    rt::Microfacet cpuMat = rt::Microfacet::MakeConductorMicrofacet(roughness, eta, k, tint);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeMicrofacetConductor(roughness, eta, k, tint);

    const auto& lut = rt::GetDirectionalAlbedoLUT();

    // Upload LUTs to device
    float* d_lutTexels = nullptr;
    size_t lutBytes = lut.eTable.texels.size() * sizeof(float);
    cudaMalloc(&d_lutTexels, lutBytes);
    cudaMemcpy(d_lutTexels, lut.eTable.texels.data(), lutBytes, cudaMemcpyHostToDevice);

    float* d_avgLutTexels = nullptr;
    size_t avgLutBytes = lut.eAvgTable.texels.size() * sizeof(float);
    cudaMalloc(&d_avgLutTexels, avgLutBytes);
    cudaMemcpy(d_avgLutTexels, lut.eAvgTable.texels.data(), avgLutBytes, cudaMemcpyHostToDevice);

    rt::Image2DView<float> hostLutView(lut.eTable.texels.data(), lut.eTable.width, lut.eTable.height, lut.eTable.wrap);
    rt::Image2DView<float> hostAvgLutView(lut.eAvgTable.texels.data(), lut.eAvgTable.width, lut.eAvgTable.height, lut.eAvgTable.wrap);

    rt::Image2DView<float> devLutView(d_lutTexels, lut.eTable.width, lut.eTable.height, lut.eTable.wrap);
    rt::Image2DView<float> devAvgLutView(d_avgLutTexels, lut.eAvgTable.width, lut.eAvgTable.height, lut.eAvgTable.wrap);

    rtx::DeviceTextureList hostTexList{ nullptr, 0, nullptr, 0, hostLutView, hostAvgLutView };
    rtx::DeviceTextureList devTexList{ nullptr, 0, nullptr, 0, devLutView, devAvgLutView };

    rt::Vector3f wo = Normalize(rt::Vector3f(0.5f, 0.3f, 0.8f));
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Vector3f wiEval = Normalize(rt::Vector3f(0.2f, -0.4f, 0.9f));
    rt::Point2f uv(0.0f, 0.0f);

    rt::Vector3f cpuF = cpuMat.f(wo, wiEval, n);
    rt::Vector3f hostF = rtx::EvaluateBsdf(devMat, wo, wiEval, n, uv, &hostTexList);

    REQUIRE_THAT(hostF.x, WithinAbs(cpuF.x, 1e-4f));
    REQUIRE_THAT(hostF.y, WithinAbs(cpuF.y, 1e-4f));
    REQUIRE_THAT(hostF.z, WithinAbs(cpuF.z, 1e-4f));

    MaterialTestInput input{ devMat, wo, n, rt::Point2f(0.5f, 0.5f), wiEval, uv, devTexList };
    MaterialTestInput* d_in = nullptr;
    MaterialTestOutput* d_out = nullptr;
    cudaMalloc(&d_in, sizeof(MaterialTestInput));
    cudaMalloc(&d_out, sizeof(MaterialTestOutput));
    cudaMemcpy(d_in, &input, sizeof(MaterialTestInput), cudaMemcpyHostToDevice);

    RunMaterialDeviceTests<<<1, 1>>>(d_in, d_out, 1);
    cudaDeviceSynchronize();

    MaterialTestOutput devOut{};
    cudaMemcpy(&devOut, d_out, sizeof(MaterialTestOutput), cudaMemcpyDeviceToHost);

    REQUIRE_THAT(devOut.f.x, WithinAbs(cpuF.x, 1e-4f));
    REQUIRE_THAT(devOut.f.y, WithinAbs(cpuF.y, 1e-4f));
    REQUIRE_THAT(devOut.f.z, WithinAbs(cpuF.z, 1e-4f));

    cudaFree(d_lutTexels);
    cudaFree(d_avgLutTexels);
    cudaFree(d_in);
    cudaFree(d_out);
}
