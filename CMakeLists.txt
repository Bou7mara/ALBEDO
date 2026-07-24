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
)

target_include_directories(rt_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(rt_core PUBLIC /W4 /WX /arch:AVX2)
else()
    target_compile_options(rt_core PUBLIC -Wall -Wextra -Wpedantic -Werror -march=native)
endif()

add_executable(raytracer src/main.cpp)
target_link_libraries(raytracer PRIVATE rt_core)

add_executable(benchmark_bvh benchmarks/benchmark_bvh.cpp)
target_link_libraries(benchmark_bvh PRIVATE rt_core)

enable_testing()
add_subdirectory(tests)
