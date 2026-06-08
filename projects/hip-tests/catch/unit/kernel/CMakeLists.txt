# Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
#
# SPDX-License-Identifier: MIT

# Common Tests - Test independent of all platforms
set(TEST_SRC
    hipMemFaultStackAllocation.cc
    hipLaunchBounds.cc
    hipDynamicShared.cc
    hipDynamicShared2.cc
    hipEmptyKernel.cc
    hipGridLaunch.cc
    hipLanguageExtensions.cc
    hipLaunchParm.cc
    hipLaunchParmFunctor.cc
    hipTestConstant.cc
    hipTestGlobalVariable.cc
    hipTestMemKernel.cc
    hipLaunchBoundsBasic.cc
    hipAutoThreadIdx.cc
    hipLaunchKernelEx.cc
    hipTestUserSgprIncrease.cc
)

# Programmatic launch only on gfx1260
set(PROGRAMMATIC_LAUNCH_SUPPORTING_GPUS "gfx1260")
set(PROGRAMMATIC_LAUNCH_SUPPORTED OFF)
foreach(PROGRAMMATIC_GPU IN LISTS PROGRAMMATIC_LAUNCH_SUPPORTING_GPUS)
  string(FIND "${OFFLOAD_ARCH_STR}" "${PROGRAMMATIC_GPU}" _idx)
  if(_idx GREATER -1)
    set(PROGRAMMATIC_LAUNCH_SUPPORTED ON)
    break()
  endif()
endforeach()
if(PROGRAMMATIC_LAUNCH_SUPPORTED)
  message(STATUS "Programmatic launch tests enabled: ${OFFLOAD_ARCH_STR}")
  set(TEST_SRC ${TEST_SRC} hipProgrammaticLaunch.cc)
  # Only compile for GPUs that support programmatic launch (uses device builtins
  # guarded by arch macros, e.g. __gfx1260__).
  set(_programmatic_arch_flags "")
  foreach(_gpu IN LISTS PROGRAMMATIC_LAUNCH_SUPPORTING_GPUS)
    list(APPEND _programmatic_arch_flags "--offload-arch=${_gpu}")
  endforeach()
  add_custom_target(programmaticLaunch.code
                    COMMAND ${CMAKE_HIP_COMPILER} --cuda-device-only ${_programmatic_arch_flags}
                    -x hip ${CMAKE_CURRENT_SOURCE_DIR}/programmaticLaunch.hip
                    -o ${CMAKE_CURRENT_BINARY_DIR}/programmaticLaunch.code
                    -I${HIP_INCLUDE_DIR} ${HIP_PATH_OPT}
                    -I${CMAKE_CURRENT_SOURCE_DIR}/../../include)
  set_property(GLOBAL APPEND PROPERTY G_INSTALL_CUSTOM_TARGETS
               "${CMAKE_CURRENT_BINARY_DIR}/programmaticLaunch.code")
endif()

# KernelStackSize only on gfx1250 and gfx1251
set(KERNEL_STACK_SIZE_SUPPORTING_GPUS "gfx1250;gfx1251")
set(KERNEL_STACK_SIZE_SUPPORTED OFF)
foreach(STACK_SIZE_GPU IN LISTS KERNEL_STACK_SIZE_SUPPORTING_GPUS)
  string(FIND "${OFFLOAD_ARCH_STR}" "${STACK_SIZE_GPU}" _idx)
  if(_idx GREATER -1)
    set(KERNEL_STACK_SIZE_SUPPORTED ON)
    break()
  endif()
endforeach()

if(UNIX)
  set(TEST_SRC ${TEST_SRC}
      hipPrintfKernel.cc)
  if(KERNEL_STACK_SIZE_SUPPORTED)
    message(STATUS "KernelStackSize test enabled: ${OFFLOAD_ARCH_STR}")
    set(TEST_SRC ${TEST_SRC} hipKernelStackSize.cc)
  endif()
endif()

# only for AMD
if(HIP_PLATFORM MATCHES "amd")
  set(AMD_SRC
      hipExtLaunchKernelGGLBasic.cc
      hipSetupArgument.cc
      hipConfigureCall.cc
  )
  set(TEST_SRC ${TEST_SRC} ${AMD_SRC})
endif()

hip_add_exe_to_target(NAME KernelTest
                      TEST_SRC ${TEST_SRC}
                      TEST_TARGET_NAME build_tests)
if(PROGRAMMATIC_LAUNCH_SUPPORTED)
  add_dependencies(KernelTest programmaticLaunch.code)
endif()
if(HIP_PLATFORM MATCHES "amd")
if(UNIX)
set(TEST_SRC hipLaunchParmFunctor.cc)
hip_add_exe_to_target(NAME KernelArgPrefetchTest
                      TEST_SRC ${TEST_SRC}
                      TEST_TARGET_NAME build_tests)
set_target_properties(KernelArgPrefetchTest PROPERTIES COMPILE_FLAGS "-DKERNEL_ARG_PREFETCH -mllvm -amdgpu-kernarg-preload-count=16")
endif()
endif()
