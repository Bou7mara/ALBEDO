#include <catch2/catch_test_macros.hpp>
#include "rt/scene/showcase.h"

using namespace rt;

TEST_CASE("CreateSphereShowcaseScene builds without throwing", "[showcase]") {
    REQUIRE_NOTHROW([] {
        ShowcaseSetup setup = CreateSphereShowcaseScene(160, 100, 4);
        REQUIRE(!setup.scene.Lights().empty());
    }());
}

TEST_CASE("CreateGemRoomShowcaseScene builds without throwing and has lights", "[showcase]") {
    REQUIRE_NOTHROW([] {
        ShowcaseSetup setup = CreateGemRoomShowcaseScene(160, 100, 4);
        REQUIRE(!setup.scene.Lights().empty());
        REQUIRE(setup.scene.Lights().size() >= 4);
    }());
}

TEST_CASE("CreateShowcaseScene default camera and BVH agree the scene is non-empty along view axis", "[showcase]") {
    ShowcaseSetup setup = CreateShowcaseScene(160, 100, 4);
    Ray centerRay(Point3f(0.0f, 1.6f, 4.6f), Normalize(Vector3f(0.0f, -0.2f, -1.0f)));
    SurfaceInteraction isect;
    REQUIRE(setup.scene.Intersect(centerRay, &isect));
}
