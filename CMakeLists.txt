cmake_minimum_required(VERSION 3.20)
project(raytracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core math/geometry library, compiled as a static library to build non-templated operations.
add_library(rt_core STATIC
    src/core/transform.cpp
    src/shapes/sphere.cpp
    src/cameras/perspective_camera.cpp
    src/materials/lambertian.cpp
    src/materials/metal.cpp
)
target_include_directories(rt_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(rt_core PUBLIC cxx_std_23)

# Apply warnings-as-errors strictly to our own targets (propagated via rt_core PUBLIC)
# without enforcing it on external dependencies fetched by CMake.
if(MSVC)
    target_compile_options(rt_core PRIVATE /W4 /WX)
else()
    target_compile_options(rt_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(raytracer src/main.cpp)
target_link_libraries(raytracer PRIVATE rt_core)

enable_testing()
add_subdirectory(tests)
