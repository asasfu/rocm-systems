################################################################################
##
## The University of Illinois/NCSA
## Open Source License (NCSA)
##
## Copyright (c) 2014-2020, Advanced Micro Devices, Inc. All rights reserved.
##
## Developed by:
##
##                 AMD Research and AMD HSA Software Development
##
##                 Advanced Micro Devices, Inc.
##
##                 www.amd.com
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to
## deal with the Software without restriction, including without limitation
## the rights to use, copy, modify, merge, publish, distribute, sublicense,
## and/or sell copies of the Software, and to permit persons to whom the
## Software is furnished to do so, subject to the following conditions:
##
##  - Redistributions of source code must retain the above copyright notice,
##    this list of conditions and the following disclaimers.
##  - Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimers in
##    the documentation and/or other materials provided with the distribution.
##  - Neither the names of Advanced Micro Devices, Inc,
##    nor the names of its contributors may be used to endorse or promote
##    products derived from this Software without specific prior written
##    permission.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
## THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
## OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
## ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
## DEALINGS WITH THE SOFTWARE.
##
################################################################################

cmake_minimum_required ( VERSION 3.7 )

# Import target 'clang'
find_package(Clang REQUIRED HINTS ${CMAKE_PREFIX_PATH}/llvm PATHS /opt/rocm/llvm )

# Define all supported variants for every GFX family
set(GFX7_VARIANTS "gfx700;gfx701;gfx702")
set(GFX8_VARIANTS "gfx801;gfx802;gfx803;gfx805;gfx810")
set(GFX9_VARIANTS "gfx900;gfx902;gfx904;gfx906;gfx908;gfx909;gfx90a;gfx90c;gfx942;gfx950")
set(GFX10_VARIANTS "gfx1010;gfx1011;gfx1012;gfx1013;gfx1030;gfx1031;gfx1032;gfx1033;gfx1034;gfx1035;gfx1036")
set(GFX11_VARIANTS "gfx1100;gfx1101;gfx1102;gfx1103;gfx1150;gfx1151;gfx1152;gfx115E;gfx1170;gfx1171")
set(GFX12_VARIANTS "gfx1200;gfx1201")
set(GFX13_VARIANTS "gfx1310;gfx131F;gfx1370")

set(TARGET_DEVICES_DEFAULT ${GFX7_VARIANTS} ${GFX8_VARIANTS} 
                           ${GFX9_VARIANTS} ${GFX10_VARIANTS}
                           ${GFX11_VARIANTS} ${GFX12_VARIANTS}
                           ${GFX13_VARIANTS})

# Determine the target devices if not specified
if (NOT DEFINED TARGET_DEVICES OR TARGET_DEVICES STREQUAL "")
  if (DEFINED THEROCK_DIST_AMDGPU_TARGETS AND NOT THEROCK_DIST_AMDGPU_TARGETS STREQUAL "")
    set(TARGET_DEVICES ${THEROCK_DIST_AMDGPU_TARGETS})
    message(STATUS "Using THEROCK_DIST_AMDGPU_TARGETS from build system: ${THEROCK_DIST_AMDGPU_TARGETS}")
  elseif (DEFINED GPU_TARGETS AND NOT GPU_TARGETS STREQUAL "")
    set(TARGET_DEVICES ${GPU_TARGETS})
    message(STATUS "Using GPU_TARGETS from build system: ${GPU_TARGETS}")
  elseif(DEFINED AMDGPU_TARGETS AND NOT AMDGPU_TARGETS STREQUAL "") 
    message(STATUS "Using AMDGPU_TARGETS from build system: ${AMDGPU_TARGETS}")
    set(TARGET_DEVICES ${AMDGPU_TARGETS})
  else()
  	message(STATUS "No target devices specified, using defaults: ${TARGET_DEVICES_DEFAULT}")
    message(STATUS "To specify targets: cmake -DTARGET_DEVICES=\"gfx900;gfx1010...\"")
    message(STATUS "Valid ASICs: ${TARGET_DEVICES_DEFAULT}")
    set(TARGET_DEVICES ${TARGET_DEVICES_DEFAULT})
  endif()    
endif()
set( TARGET_DEVICES ${TARGET_DEVICES} CACHE STRING "Build targets" FORCE )

if(${CMAKE_VERBOSE_MAKEFILE})
  get_property(clang_path TARGET clang PROPERTY LOCATION)
  message("Using clang from: ${clang_path}")
  message("Build Setting:")
  message("  Target Devices*: ${TARGET_DEVICES}")
  message("  (Specify \";\" separated list of target IDs.)")
  message("       Clang path: ${clang_path}")
endif()

##==========================================
##  Add custom command to generate a kernel code object file
##==========================================
function(gen_kernel_bc TARGET_ID INPUT_FILE OUTPUT_FILE)
  if(WIN32)
    set (WIN_CL_COMPILE_OPTION "-fshort-wchar")
  endif()
  separate_arguments(CLANG_ARG_LIST UNIX_COMMAND
    "-O2 -x cl -Xclang -finclude-default-header -cl-denorms-are-zero -cl-std=CL2.0
    -target amdgcn-amd-amdhsa -mcpu=${TARGET_ID} -mcode-object-version=4 ${WIN_CL_COMPILE_OPTION}
    -o ${OUTPUT_FILE} ${INPUT_FILE}")

  ## Add custom command to produce a code object file.
  ## This depends on the kernel source file & compiler.
  add_custom_command(OUTPUT ${OUTPUT_FILE} COMMAND clang ${CLANG_ARG_LIST}
    DEPENDS ${INPUT_FILE} clang
    COMMENT "BUILDING bitcode for ${OUTPUT_FILE}..."
    VERBATIM)

if(${CMAKE_VERBOSE_MAKEFILE})
  message("      Kernel Source: " ${INPUT_FILE})
  message("     Kernel Bitcode: " ${OUTPUT_FILE})
endif()

endfunction(gen_kernel_bc)

##==========================================
## Find device code object name and forward to custom command
##==========================================
function(build_kernel BLIT_NAME TARGET_ID)

  ## generate kernel bitcodes
  set (CODE_OBJECT_FILE "${BLIT_NAME}_${TARGET_ID}")
  set (CL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/imageblit_kernels.cl)
  gen_kernel_bc(${TARGET_ID} ${CL_FILE} ${CODE_OBJECT_FILE})

  ## Build a list of code object file names
  ## These will be target dependencies.
  set (HSACO_TARG_LIST ${HSACO_TARG_LIST} "${CODE_OBJECT_FILE}" PARENT_SCOPE)

endfunction(build_kernel)

##==========================================
## Build the kernel for a list of devices
##==========================================
function(build_kernel_for_devices BLIT_NAME)

  set(HSACO_TARG_LIST "")

  # Expand target devices to include all variants of each GFX family
  set(EXPANDED_TARGET_DEVS "")
  foreach(dev ${TARGET_DEVICES})
    if (${dev} MATCHES "^gfx7") 
      list(APPEND EXPANDED_TARGETS ${GFX7_VARIANTS})
    elseif(${dev} MATCHES "^gfx8")
      list(APPEND EXPANDED_TARGETS ${GFX8_VARIANTS})
    elseif(${dev} MATCHES "^gfx9")
      list(APPEND EXPANDED_TARGETS ${GFX9_VARIANTS})
    elseif(${dev} MATCHES "^gfx10")
      list(APPEND EXPANDED_TARGETS ${GFX10_VARIANTS})
    elseif(${dev} MATCHES "^gfx11")
      list(APPEND EXPANDED_TARGETS ${GFX11_VARIANTS})
    elseif(${dev} MATCHES "^gfx12")
      list(APPEND EXPANDED_TARGETS ${GFX12_VARIANTS})
    elseif(${dev} MATCHES "^gfx13")
      list(APPEND EXPANDED_TARGETS ${GFX13_VARIANTS})
    endif()
  endforeach(dev)

  # Remove duplicates
  list(REMOVE_DUPLICATES EXPANDED_TARGETS)

  foreach(dev ${EXPANDED_TARGETS})
    # For gfx1250/1260, llvm compiler does not support blit kernels for images
    # Skip generation for any unsupported device that is not listed in TARGET_DEVICES_DEFAULT
    list(FIND TARGET_DEVICES_DEFAULT ${dev} dev_index)
    if(dev_index EQUAL -1)
      message(STATUS "Skipping unsupported device: ${dev}")
      continue()
    endif()

    if(${CMAKE_VERBOSE_MAKEFILE})
      message("\n  Generating: ${dev} ...")
    endif()
    build_kernel(${BLIT_NAME} ${dev})
  endforeach(dev)

  set(HSACO_TARG_LIST ${HSACO_TARG_LIST} PARENT_SCOPE)

endfunction(build_kernel_for_devices)

##==========================================
## Create BLIT Code Object blobs file
##==========================================
function(generate_blit_file BFILE)

  ## Add a custom command that generates opencl_blit_objects.cpp
  ## This depends on all the generated code object files and the C++ generator script.
  if (UNIX)
    add_custom_command(OUTPUT ${BFILE}.cpp
                     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/create_hsaco_ascii_file.sh ${CMAKE_CURRENT_BINARY_DIR}/${BFILE}.cpp
                     DEPENDS ${HSACO_TARG_LIST} create_hsaco_ascii_file.sh )
  else()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_custom_command(
      OUTPUT ${BFILE}.cpp
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/create_hsaco_ascii_file.py ${CMAKE_CURRENT_BINARY_DIR}/${BFILE}.cpp ${HSACO_TARG_LIST}
      COMMENT "Collating blit shaders..."
      DEPENDS ${HSACO_TARG_LIST} create_hsaco_ascii_file.py)
  endif()

  ## Export a target that builds (and depends on) opencl_blit_objects.cpp
  add_custom_target( ${BFILE} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${BFILE}.cpp )

endfunction(generate_blit_file)

build_kernel_for_devices("ocl_blit_object")

# Only generate the blit file if we have any kernels built
list(LENGTH HSACO_TARG_LIST kernel_count)
if(kernel_count GREATER 0)
  generate_blit_file("opencl_blit_objects")
else()
  # No kernels built - create stub file (opencl_blit_objects.cpp)
  message(WARNING "No image blit kernels built for current targets. Creating stub opencl_blit_objects.cpp")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/opencl_blit_objects.cpp 
    "// No image blit kernels available for current target devices\n"
    "// This occurs when building for targets not yet supported by LLVM OpenCL compiler\n\n"
    "namespace rocr {\n"
    "namespace image {\n"
    "// Empty - no image blit support for current targets\n"
    "} // namespace image\n"
    "} // namespace rocr\n")
  add_custom_target(opencl_blit_objects DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/opencl_blit_objects.cpp)
endif()
