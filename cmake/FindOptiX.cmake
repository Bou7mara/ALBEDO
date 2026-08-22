# cmake/FindOptiX.cmake
# Locates OptiX 9.1 SDK headers or fetches them from NVIDIA/optix-dev (v9.1.0)

include(FetchContent)

set(OPTIX_TARGET_VERSION "9.1.0")

if(DEFINED ENV{OPTIX_ROOT} AND NOT DEFINED OPTIX_ROOT)
    set(OPTIX_ROOT "$ENV{OPTIX_ROOT}")
endif()

if(OPTIX_ROOT)
    find_path(OptiX_INCLUDE_DIR
        NAMES optix.h optix_stubs.h
        PATHS "${OPTIX_ROOT}/include" "${OPTIX_ROOT}"
        NO_DEFAULT_PATH
    )
endif()

if(NOT OptiX_INCLUDE_DIR)
    message(STATUS "OptiX SDK not found locally. Fetching OptiX headers (v${OPTIX_TARGET_VERSION}) from NVIDIA/optix-dev...")
    FetchContent_Declare(
        optix_dev
        GIT_REPOSITORY https://github.com/NVIDIA/optix-dev.git
        GIT_TAG        v9.1.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_GetProperties(optix_dev)
    if(NOT optix_dev_POPULATED)
        FetchContent_Populate(optix_dev)
    endif()
    set(OptiX_INCLUDE_DIR "${optix_dev_SOURCE_DIR}/include")
endif()

if(OptiX_INCLUDE_DIR AND EXISTS "${OptiX_INCLUDE_DIR}/optix.h")
    # Verify version macro in optix.h
    file(STRINGS "${OptiX_INCLUDE_DIR}/optix.h" _optix_version_line REGEX "#define OPTIX_VERSION ")
    if(_optix_version_line MATCHES "#define OPTIX_VERSION ([0-9]+)")
        set(OPTIX_VERSION_NUM "${CMAKE_MATCH_1}")
        message(STATUS "Found OptiX headers: ${OptiX_INCLUDE_DIR} (OPTIX_VERSION ${OPTIX_VERSION_NUM})")
    else()
        message(STATUS "Found OptiX headers: ${OptiX_INCLUDE_DIR}")
    endif()

    if(NOT TARGET OptiX::OptiX)
        add_library(OptiX::OptiX INTERFACE IMPORTED)
        target_include_directories(OptiX::OptiX INTERFACE "${OptiX_INCLUDE_DIR}")
    endif()

    set(OptiX_FOUND TRUE)
else()
    message(FATAL_ERROR "Could not find or fetch OptiX ${OPTIX_TARGET_VERSION} headers!")
endif()
