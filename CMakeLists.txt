cmake_minimum_required(VERSION 3.15)
project(ALBEDO LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific warning flags and treat warnings as errors
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -Wpedantic)
endif()

# Include directory for the public headers
set(RT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Executable target
add_executable(albedo src/main.cpp)
target_include_directories(albedo PRIVATE ${RT_INCLUDE_DIR})

# Unit testing setup
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.4.11
    )
    FetchContent_MakeAvailable(doctest)

    # test_main.cpp contains DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN to speed up builds
    add_executable(tests
        tests/test_main.cpp
        tests/test_tuple3.cpp
        tests/test_geometry.cpp
    )
    target_include_directories(tests PRIVATE ${RT_INCLUDE_DIR})
    target_link_libraries(tests PRIVATE doctest::doctest)

    enable_testing()
    add_test(NAME CoreTests COMMAND tests)
endif()
