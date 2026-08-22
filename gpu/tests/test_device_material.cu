#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "device_material.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/dielectric.h"
#include "rt/materials/microfacet_brdf.h"
#include "rt/materials/emissive.h"

#include <cuda_runtime.h>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace {

    struct MaterialTestInput {
        rtx::DeviceMaterial mat;
        rt::Vector3f wo;
        rt::Vector3f n;
        rt::Point2f u;
        rt::Vector3f wiEval;
    };

    struct MaterialTestOutput {
        rt::Vector3f f;
        rt::Vector3f sampleF;
        rt::Vector3f sampledWi;
        float samplePdf;
        float evalPdf;
        rt::Vector3f emission;
    };

    __global__ void RunMaterialDeviceTests(const MaterialTestInput* inputs,
                                           MaterialTestOutput* outputs,
                                           int count) {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= count) return;

        const auto& in = inputs[idx];
        auto& out = outputs[idx];

        out.f = rtx::EvaluateBsdf(in.mat, in.wo, in.wiEval, in.n);
        out.sampleF = rtx::SampleBsdf(in.mat, in.wo, in.n, in.u, &out.sampledWi, &out.samplePdf);
        out.evalPdf = rtx::PdfBsdf(in.mat, in.wo, in.wiEval, in.n);
        out.emission = rtx::EvaluateEmission(in.mat, in.wo, in.n);
    }

} // namespace

TEST_CASE("Lambertian host/device parity with CPU BSDF", "[gpu][material][lambertian]") {
    rt::Vector3f albedo(0.7f, 0.3f, 0.2f);
    rt::Lambertian cpuMat(albedo);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeLambertian(albedo);

    rt::Vector3f wo = Normalize(rt::Vector3f(0.3f, 0.4f, 0.8f));
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Point2f u(0.35f, 0.65f);
    rt::Vector3f wiEval = Normalize(rt::Vector3f(0.2f, -0.3f, 0.9f));

    // CPU Evaluation
    rt::Vector3f cpuF = cpuMat.f(wo, wiEval, n);
    float cpuEvalPdf = cpuMat.Pdf(wo, wiEval, n);
    rt::Vector3f cpuSampledWi;
    float cpuSamplePdf = 0.0f;
    rt::Vector3f cpuSampleF = cpuMat.Sample_f(wo, n, u, &cpuSampledWi, &cpuSamplePdf);

    // Host DeviceMaterial Evaluation
    rt::Vector3f hostF = rtx::EvaluateBsdf(devMat, wo, wiEval, n);
    float hostEvalPdf = rtx::PdfBsdf(devMat, wo, wiEval, n);
    rt::Vector3f hostSampledWi;
    float hostSamplePdf = 0.0f;
    rt::Vector3f hostSampleF = rtx::SampleBsdf(devMat, wo, n, u, &hostSampledWi, &hostSamplePdf);

    REQUIRE_THAT(hostF.x, WithinAbs(cpuF.x, 1e-5f));
    REQUIRE_THAT(hostF.y, WithinAbs(cpuF.y, 1e-5f));
    REQUIRE_THAT(hostF.z, WithinAbs(cpuF.z, 1e-5f));
    REQUIRE_THAT(hostEvalPdf, WithinAbs(cpuEvalPdf, 1e-5f));
    REQUIRE_THAT(hostSamplePdf, WithinAbs(cpuSamplePdf, 1e-5f));
    REQUIRE_THAT(hostSampledWi.x, WithinAbs(cpuSampledWi.x, 1e-5f));
    REQUIRE_THAT(hostSampledWi.y, WithinAbs(cpuSampledWi.y, 1e-5f));
    REQUIRE_THAT(hostSampledWi.z, WithinAbs(cpuSampledWi.z, 1e-5f));

    // Device Kernel Evaluation
    MaterialTestInput input{ devMat, wo, n, u, wiEval };
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
    REQUIRE_THAT(devOut.evalPdf, WithinAbs(cpuEvalPdf, 1e-5f));
    REQUIRE_THAT(devOut.samplePdf, WithinAbs(cpuSamplePdf, 1e-5f));
    REQUIRE_THAT(devOut.sampledWi.x, WithinAbs(cpuSampledWi.x, 1e-5f));
    REQUIRE_THAT(devOut.sampledWi.y, WithinAbs(cpuSampledWi.y, 1e-5f));
    REQUIRE_THAT(devOut.sampledWi.z, WithinAbs(cpuSampledWi.z, 1e-5f));

    cudaFree(d_in);
    cudaFree(d_out);
}

TEST_CASE("Oren-Nayar host/device parity with CPU BSDF", "[gpu][material][orennayar]") {
    rt::Vector3f albedo(0.7f, 0.3f, 0.2f);
    float roughness = 0.4f;
    rt::Lambertian cpuMat(albedo, roughness);
    rtx::DeviceMaterial devMat = rtx::DeviceMaterial::MakeLambertian(albedo, roughness);

    rt::Vector3f wo = Normalize(rt::Vector3f(0.8f, 0.2f, 0.5f));
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Vector3f wiEval = Normalize(rt::Vector3f(0.7f, 0.3f, 0.6f));

    rt::Vector3f cpuF = cpuMat.f(wo, wiEval, n);
    rt::Vector3f hostF = rtx::EvaluateBsdf(devMat, wo, wiEval, n);

    REQUIRE_THAT(hostF.x, WithinAbs(cpuF.x, 1e-5f));
    REQUIRE_THAT(hostF.y, WithinAbs(cpuF.y, 1e-5f));
    REQUIRE_THAT(hostF.z, WithinAbs(cpuF.z, 1e-5f));

    rt::Point2f u(0.35f, 0.65f);
    MaterialTestInput input{ devMat, wo, n, u, wiEval };
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

TEST_CASE("Microfacet conductor and dielectric parity", "[gpu][material][microfacet]") {
    float roughness = 0.25f;
    rt::Vector3f eta(0.2f, 0.9f, 1.1f);
    rt::Vector3f k(3.5f, 2.4f, 1.8f);
    rt::Vector3f tint(1.0f, 0.9f, 0.8f);

    auto cpuMat = rt::Microfacet::MakeConductorMicrofacet(roughness, eta, k, tint);
    auto devMat = rtx::DeviceMaterial::MakeMicrofacetConductor(roughness, eta, k, tint);

    rt::Vector3f wo = Normalize(rt::Vector3f(0.2f, 0.3f, 0.9f));
    rt::Vector3f n(0.0f, 0.0f, 1.0f);
    rt::Point2f u(0.4f, 0.6f);

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
