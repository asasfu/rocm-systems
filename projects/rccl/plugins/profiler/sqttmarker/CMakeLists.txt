# RCCL SQTT marker profiler plugin (librccl-profiler-sqttmarker.so)
#
# Can be built two ways:
#   1. Standalone:   cmake -S . -B build && cmake --build build
#   2. As part of the main RCCL build via add_subdirectory(plugins/profiler/sqttmarker)
#
# The plugin adds SQTT (SQ Thread Trace) instrumentation markers to RCCL
# operations for performance analysis and profiling.

# Detect standalone vs. in-tree configuration.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(rccl-profiler-sqttmarker LANGUAGES CXX)
    set(SQTTMARKER_STANDALONE ON)
else()
    set(SQTTMARKER_STANDALONE OFF)
endif()

# RCCL source root (this file lives at plugins/profiler/sqttmarker).
get_filename_component(RCCL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

# Check ROCm version for SQTT support (requires >= 7.13)
if(SQTTMARKER_STANDALONE)
    # Determine ROCm version for standalone builds
    set(EXPLICIT_ROCM_VERSION "" CACHE STRING "Explicit ROCM version")
    if(EXPLICIT_ROCM_VERSION)
        set(rocm_version_string "${EXPLICIT_ROCM_VERSION}")
    else()
        find_file(rocm_version_file "version" PATH_SUFFIXES ".info" "core/.info"
            HINTS /opt/rocm $ENV{ROCM_PATH}
            REQUIRED)
        file(READ "${rocm_version_file}" rocm_version_string)
    endif()
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" rocm_version_matches ${rocm_version_string})
    if(rocm_version_matches)
        set(ROCM_MAJOR_VERSION ${CMAKE_MATCH_1})
        set(ROCM_MINOR_VERSION ${CMAKE_MATCH_2})
    endif()
endif()

# Validate ROCm version
if(DEFINED ROCM_MAJOR_VERSION)
    if(${ROCM_MAJOR_VERSION} LESS 7)
        message(WARNING "SQTT marker plugin requires ROCm >= 7.13. Current: ${ROCM_MAJOR_VERSION}.${ROCM_MINOR_VERSION}. Skipping build.")
        return()
    elseif(${ROCM_MAJOR_VERSION} EQUAL 7 AND ${ROCM_MINOR_VERSION} LESS 13)
        message(WARNING "SQTT marker plugin requires ROCm >= 7.13. Current: ${ROCM_MAJOR_VERSION}.${ROCM_MINOR_VERSION}. Skipping build.")
        return()
    endif()
    message(STATUS "Building SQTT marker profiler plugin (ROCm ${ROCM_MAJOR_VERSION}.${ROCM_MINOR_VERSION})")
endif()

add_library(rccl-profiler-sqttmarker SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/sqttmarker_plugin.cc
)

target_include_directories(rccl-profiler-sqttmarker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RCCL_ROOT}/src/include/plugin
    ${RCCL_ROOT}/src/include
)

target_compile_definitions(rccl-profiler-sqttmarker PRIVATE
    RCCL_SQTTMARKER_PLUGIN_BUILD
    SQTT_ENABLED=1
)

target_compile_features(rccl-profiler-sqttmarker PRIVATE cxx_std_17)

target_compile_options(rccl-profiler-sqttmarker PRIVATE
    -Wall
    -Wextra
    "-fpass-plugin=/opt/rocm/lib/libsqttinstrumentpass.so"
)

# Match the library naming convention
set_target_properties(rccl-profiler-sqttmarker PROPERTIES
    OUTPUT_NAME "rccl-profiler-sqttmarker"
    PREFIX "lib"
    POSITION_INDEPENDENT_CODE ON
)

if(NOT SQTTMARKER_STANDALONE)
    set_target_properties(rccl-profiler-sqttmarker PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )
endif()
