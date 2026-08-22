#include <catch2/catch_test_macros.hpp>
#include "optix_context.h"

TEST_CASE("OptixContext creation and initialization", "[gpu][optix]") {
    std::unique_ptr<rtx::OptixContext> ctx;
    REQUIRE_NOTHROW(ctx = rtx::OptixContext::Create());
    REQUIRE(ctx != nullptr);
    REQUIRE(ctx->GetOptixDeviceContext() != nullptr);
    REQUIRE(ctx->GetCudaContext() != nullptr);
    REQUIRE(ctx->GetCudaStream() != nullptr);
    REQUIRE(!ctx->GetDeviceName().empty());
}
