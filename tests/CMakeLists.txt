include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

# Enable Catch2's CMake helper scripts (e.g. catch_discover_tests)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_executable(rt_tests
    test_tuple3.cpp
    test_vector3.cpp
    test_point3.cpp
    test_normal3.cpp
    test_ray.cpp
    test_transform.cpp
    test_sphere.cpp
    test_vector2.cpp
    test_point2.cpp
    test_camera.cpp
    test_scene.cpp
    test_onb.cpp
    test_sampling.cpp
    test_lambertian.cpp
    test_shape.cpp
    test_rng.cpp
    test_metal.cpp
    test_bounds3.cpp
    test_bvh.cpp
    test_dielectric.cpp
    test_fresnel.cpp
    test_microfacet.cpp
    test_microfacet_brdf.cpp
    test_emissive.cpp
    test_bsdf_pdf.cpp
    test_sphere_sampling.cpp
    test_mis_weights.cpp
    test_power_sampling.cpp
    test_progress.cpp
    test_multithreading.cpp
)

target_link_libraries(rt_tests PRIVATE rt_core Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(rt_tests)
