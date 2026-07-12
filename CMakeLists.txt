cmake_minimum_required(VERSION 3.20)
project(raytracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only core, exposed as an interface library so both the main
# executable and the test suite can link against it uniformly.
add_library(rt_core INTERFACE)
target_include_directories(rt_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(rt_core INTERFACE cxx_std_23)

# Apply warnings-as-errors strictly to our own targets (propagated via rt_core INTERFACE)
# without enforcing it on external dependencies fetched by CMake.
if(MSVC)
    target_compile_options(rt_core INTERFACE /W4 /WX)
else()
    target_compile_options(rt_core INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(raytracer src/main.cpp)
target_link_libraries(raytracer PRIVATE rt_core)

enable_testing()
add_subdirectory(tests)
