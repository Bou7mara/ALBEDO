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
)

target_link_libraries(rt_tests PRIVATE rt_core Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(rt_tests)
