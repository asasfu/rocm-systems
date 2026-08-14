/* Copyright (c) 2021-2026 Advanced Micro Devices, Inc. All rights reserved. */

#pragma once

//# WA: This file defines macros that the CMake defines but AtiMake doesn't always
//# AtiMake is used inconsistently, so our Makefiles def are not always included when building with our headers.
//# This results in missing defines, so we replicate them here as they break.
//# This file can be deleted once we deprecate AtiMake, and switch completely to CMake in the drivers.

//# See gpuopen/core/CMakeListst.txt for a list of defines, if something breaks.

#ifndef DD_PLATFORM_WINDOWS_UM
    #if _WIN32 && !_KERNEL_MODE
        #define DD_PLATFORM_WINDOWS_UM 1
        #define DD_PLATFORM_IS_UM      1
    #endif
#endif

#ifndef DD_PLATFORM_WINDOWS_KM
    #if _WIN32 && _KERNEL_MODE
        #define DD_PLATFORM_WINDOWS_KM 1
        #define DD_PLATFORM_IS_KM      1
    #endif
#endif

#ifndef DD_PLATFORM_LINUX_UM
    #ifdef __linux__
        #define DD_PLATFORM_LINUX_UM 1
        #define DD_PLATFORM_IS_UM    1
        #define DD_PLATFORM_IS_GNU   1
    #endif
#endif
