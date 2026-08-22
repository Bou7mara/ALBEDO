# gpu/CMakeLists.txt

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPTIXIR_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/optixir")
file(MAKE_DIRECTORY "${OPTIXIR_OUTPUT_DIR}")

# 1. Custom Command: smoke_test.cu -> OptiX-IR (.optixir)
set(SMOKE_TEST_CU "${CMAKE_CURRENT_SOURCE_DIR}/shaders/smoke_test.cu")
set(SMOKE_TEST_OPTIXIR "${OPTIXIR_OUTPUT_DIR}/smoke_test.optixir")

add_custom_command(
    OUTPUT "${SMOKE_TEST_OPTIXIR}"
    COMMAND "${CMAKE_CUDA_COMPILER}"
            --optix-ir
            -arch=compute_89
            -code=sm_89
            --std=c++17
            --expt-relaxed-constexpr
            --generate-line-info
            -I "${OptiX_INCLUDE_DIR}"
            -I "${CMAKE_CURRENT_SOURCE_DIR}/include"
            -I "${CMAKE_SOURCE_DIR}/include"
            "${SMOKE_TEST_CU}"
            -o "${SMOKE_TEST_OPTIXIR}"
    DEPENDS "${SMOKE_TEST_CU}"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/launch_params.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/vector3.h"
    COMMENT "Compiling smoke_test.cu -> OptiX-IR (${SMOKE_TEST_OPTIXIR})"
    VERBATIM
)

# 2. Custom Command: intersection_test.cu -> OptiX-IR (.optixir)
set(INTERSECTION_TEST_CU "${CMAKE_CURRENT_SOURCE_DIR}/shaders/intersection_test.cu")
set(INTERSECTION_TEST_OPTIXIR "${OPTIXIR_OUTPUT_DIR}/intersection_test.optixir")

add_custom_command(
    OUTPUT "${INTERSECTION_TEST_OPTIXIR}"
    COMMAND "${CMAKE_CUDA_COMPILER}"
            --optix-ir
            -arch=compute_89
            -code=sm_89
            --std=c++17
            --expt-relaxed-constexpr
            --generate-line-info
            -I "${OptiX_INCLUDE_DIR}"
            -I "${CMAKE_CURRENT_SOURCE_DIR}/include"
            -I "${CMAKE_SOURCE_DIR}/include"
            "${INTERSECTION_TEST_CU}"
            -o "${INTERSECTION_TEST_OPTIXIR}"
    DEPENDS "${INTERSECTION_TEST_CU}"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/device_scene.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/point3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/vector3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/normal3.h"
    COMMENT "Compiling intersection_test.cu -> OptiX-IR (${INTERSECTION_TEST_OPTIXIR})"
    VERBATIM
)

# 3. Custom Command: normals_render.cu -> OptiX-IR (.optixir)
set(NORMALS_RENDER_CU "${CMAKE_CURRENT_SOURCE_DIR}/shaders/normals_render.cu")
set(NORMALS_RENDER_OPTIXIR "${OPTIXIR_OUTPUT_DIR}/normals_render.optixir")

add_custom_command(
    OUTPUT "${NORMALS_RENDER_OPTIXIR}"
    COMMAND "${CMAKE_CUDA_COMPILER}"
            --optix-ir
            -arch=compute_89
            -code=sm_89
            --std=c++17
            --expt-relaxed-constexpr
            --generate-line-info
            -I "${OptiX_INCLUDE_DIR}"
            -I "${CMAKE_CURRENT_SOURCE_DIR}/include"
            -I "${CMAKE_SOURCE_DIR}/include"
            "${NORMALS_RENDER_CU}"
            -o "${NORMALS_RENDER_OPTIXIR}"
    DEPENDS "${NORMALS_RENDER_CU}"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/device_scene.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/launch_params.h"
            "${CMAKE_SOURCE_DIR}/include/rt/cam/perspective_camera.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/point3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/vector3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/normal3.h"
    COMMENT "Compiling normals_render.cu -> OptiX-IR (${NORMALS_RENDER_OPTIXIR})"
    VERBATIM
)

# 4. Custom Command: single_bounce.cu -> OptiX-IR (.optixir)
set(SINGLE_BOUNCE_CU "${CMAKE_CURRENT_SOURCE_DIR}/shaders/single_bounce.cu")
set(SINGLE_BOUNCE_OPTIXIR "${OPTIXIR_OUTPUT_DIR}/single_bounce.optixir")

add_custom_command(
    OUTPUT "${SINGLE_BOUNCE_OPTIXIR}"
    COMMAND "${CMAKE_CUDA_COMPILER}"
            --optix-ir
            -arch=compute_89
            -code=sm_89
            --std=c++17
            --expt-relaxed-constexpr
            --generate-line-info
            -I "${OptiX_INCLUDE_DIR}"
            -I "${CMAKE_CURRENT_SOURCE_DIR}/include"
            -I "${CMAKE_SOURCE_DIR}/include"
            "${SINGLE_BOUNCE_CU}"
            -o "${SINGLE_BOUNCE_OPTIXIR}"
    DEPENDS "${SINGLE_BOUNCE_CU}"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/device_scene.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/device_material.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/launch_params.h"
            "${CMAKE_SOURCE_DIR}/include/rt/cam/perspective_camera.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/point3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/vector3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/normal3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/rng.h"
    COMMENT "Compiling single_bounce.cu -> OptiX-IR (${SINGLE_BOUNCE_OPTIXIR})"
    VERBATIM
)

# 5. Custom Command: path_tracer.cu -> OptiX-IR (.optixir)
set(PATH_TRACER_CU "${CMAKE_CURRENT_SOURCE_DIR}/shaders/path_tracer.cu")
set(PATH_TRACER_OPTIXIR "${OPTIXIR_OUTPUT_DIR}/path_tracer.optixir")

add_custom_command(
    OUTPUT "${PATH_TRACER_OPTIXIR}"
    COMMAND "${CMAKE_CUDA_COMPILER}"
            --optix-ir
            -arch=compute_89
            -code=sm_89
            --std=c++17
            --expt-relaxed-constexpr
            --generate-line-info
            -I "${OptiX_INCLUDE_DIR}"
            -I "${CMAKE_CURRENT_SOURCE_DIR}/include"
            -I "${CMAKE_SOURCE_DIR}/include"
            "${PATH_TRACER_CU}"
            -o "${PATH_TRACER_OPTIXIR}"
    DEPENDS "${PATH_TRACER_CU}"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/device_scene.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/device_material.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/device_light.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/launch_params.h"
            "${CMAKE_SOURCE_DIR}/include/rt/cam/perspective_camera.h"
            "${CMAKE_SOURCE_DIR}/include/rt/integrator_constants.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/point3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/vector3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/normal3.h"
            "${CMAKE_SOURCE_DIR}/include/rt/core/rng.h"
    COMMENT "Compiling path_tracer.cu -> OptiX-IR (${PATH_TRACER_OPTIXIR})"
    VERBATIM
)

add_custom_target(optixir_shaders ALL DEPENDS
    "${SMOKE_TEST_OPTIXIR}"
    "${INTERSECTION_TEST_OPTIXIR}"
    "${NORMALS_RENDER_OPTIXIR}"
    "${SINGLE_BOUNCE_OPTIXIR}"
    "${PATH_TRACER_OPTIXIR}"
)

# 6. rt_gpu Library
add_library(rt_gpu STATIC
    src/optix_context.cpp
    src/optix_context.h
    src/device_scene.cpp
    src/gpu_renderer.cpp
    src/denoiser.cpp
    src/denoiser.h
    include/device_scene.h
    include/device_material.h
    include/device_light.h
    include/gpu_renderer.h
    include/launch_params.h
)

target_include_directories(rt_gpu PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(rt_gpu PUBLIC
    OPTIXIR_DIR="${OPTIXIR_OUTPUT_DIR}"
)

set_source_files_properties(src/gpu_renderer.cpp PROPERTIES LANGUAGE CUDA)
target_compile_options(rt_gpu PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>)

target_link_libraries(rt_gpu PUBLIC
    rt_core
    OptiX::OptiX
    CUDA::cudart
    CUDA::cuda_driver
)

add_dependencies(rt_gpu optixir_shaders)

# Main gpu_smoke_test executable
add_executable(gpu_smoke_test
    src/main.cpp
)

target_link_libraries(gpu_smoke_test PRIVATE
    rt_gpu
)

# 7. GPU unit tests
add_executable(gpu_tests
    tests/test_optix_context.cpp
    tests/test_device_scene.cpp
    tests/test_normals_render.cpp
    tests/test_single_bounce_render.cpp
    tests/test_cornell_box_render.cpp
    tests/test_gpu_vs_cpu_full_render.cpp
    tests/test_denoiser.cpp
)

target_include_directories(gpu_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_definitions(gpu_tests PRIVATE
    OPTIXIR_DIR="${OPTIXIR_OUTPUT_DIR}"
)

target_compile_options(gpu_tests PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
)

target_link_libraries(gpu_tests PRIVATE
    rt_gpu
    rt_core
    OptiX::OptiX
    CUDA::cudart
    CUDA::cuda_driver
    Catch2::Catch2WithMain
)

add_dependencies(gpu_tests optixir_shaders)

# 8. Dual Host/Device Math, Material, and Light Parity Tests
add_executable(gpu_math_tests
    tests/test_math_parity.cu
    tests/test_device_material.cu
    tests/test_device_light.cu
)

target_include_directories(gpu_math_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include"
)

target_compile_options(gpu_math_tests PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--extended-lambda --expt-relaxed-constexpr>
)

target_link_libraries(gpu_math_tests PRIVATE
    rt_core
    CUDA::cudart
    Catch2::Catch2WithMain
)

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(gpu_tests PROPERTIES LABELS "gpu")
catch_discover_tests(gpu_math_tests PROPERTIES LABELS "gpu;math;materials;lights")

