cmake_minimum_required(VERSION 3.20)
project(raytracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

add_library(rt_core STATIC
    src/core/transform.cpp
    src/shapes/sphere.cpp
    src/cameras/perspective_camera.cpp
    src/materials/lambertian.cpp
    src/materials/metal.cpp
    src/accel/bvh.cpp
    src/materials/microfacet_brdf.cpp
    src/materials/dielectric.cpp
    src/materials/emissive.cpp
    src/lights/diffuse_area_light.cpp
    src/scene/showcase.cpp
    src/scene/mesh_generators.cpp
    src/shapes/Triangle.cpp
    src/io/obj_loader.cpp
    src/shapes/mesh_instance.cpp
    src/shapes/instance.cpp
    src/scene/scene_node.cpp
    src/shapes/quad.cpp
    src/materials/disney_principled.cpp
    src/materials/energy_compensation.cpp
    src/accel/tlas.cpp
    src/accel/bvh4.cpp
    src/accel/wide_bvh.cpp
)

target_include_directories(rt_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(rt_core PUBLIC $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX /wd4723 /wd4324 /EHsc /arch:AVX2>)
else()
    target_compile_options(rt_core PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Werror -march=native>)
endif()

add_executable(raytracer src/main.cpp)
target_link_libraries(raytracer PRIVATE rt_core)

add_executable(benchmark_bvh benchmarks/bench_main.cpp)
target_link_libraries(benchmark_bvh PRIVATE rt_core)
target_include_directories(benchmark_bvh PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_subdirectory(tests)

option(ALBEDO_ENABLE_GPU "Build the OptiX/CUDA renderer" ON)

if(ALBEDO_ENABLE_GPU)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_ARCHITECTURES 89)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    include(FindOptiX)
    add_subdirectory(gpu)
    target_link_libraries(raytracer PRIVATE rt_gpu)
    target_compile_definitions(raytracer PRIVATE ALBEDO_ENABLE_GPU)
endif()
