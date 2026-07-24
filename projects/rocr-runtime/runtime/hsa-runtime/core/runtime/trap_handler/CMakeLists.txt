################################################################################
##
## The University of Illinois/NCSA
## Open Source License (NCSA)
##
## Copyright (c) 2025, Advanced Micro Devices, Inc. All rights reserved.
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

# Import target 'clang' and 'llvm-objcopy'
find_package(Clang REQUIRED HINTS ${CMAKE_PREFIX_PATH}/llvm PATHS /opt/rocm/llvm )
find_package(LLVM REQUIRED HINTS ${CMAKE_PREFIX_PATH}/llvm PATHS /opt/rocm/llvm )

set (TARGET_DEVICES_DEFAULT "gfx900;gfx942;gfx950;gfx1010;gfx1030;gfx1100;gfx1200;gfx1250;gfx1260;gfx1310")

# Determine the target devices if not specified
if (NOT DEFINED TARGET_DEVICES OR TARGET_DEVICES STREQUAL "")
  if (DEFINED THEROCK_DIST_AMDGPU_TARGETS AND NOT THEROCK_DIST_AMDGPU_TARGETS STREQUAL "")
  message(STATUS "Using THEROCK_DIST_AMDGPU_TARGETS from build system: ${THEROCK_DIST_AMDGPU_TARGETS}")
  set(TARGET_DEVICES ${THEROCK_DIST_AMDGPU_TARGETS})
  elseif (DEFINED GPU_TARGETS AND NOT GPU_TARGETS STREQUAL "")
    message(STATUS "Using GPU_TARGETS from build system: ${GPU_TARGETS}")
    set(TARGET_DEVICES ${GPU_TARGETS})
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

if(${CMAKE_VERBOSE_MAKEFILE})
  get_property(clang_path TARGET clang PROPERTY LOCATION)
  get_property(objcopy_path TARGET llvm-objcopy PROPERTY LOCATION)
  message("Using clang from: ${clang_path}")
  message("Using llvm-objcopy from: ${objcopy_path}")
  message("Trap handlers assembled for: ${TARGET_DEVICES}")
endif()

##==========================================
##  Add custom command to generate a kernel code object file
##==========================================
function(gen_kernel_bc TARGET_ID INPUT_FILE OUTPUT_FILE)

  set(CODE_OBJECT "${OUTPUT_FILE}.hsaco")

  separate_arguments(CLANG_ARG_LIST UNIX_COMMAND
  "-x assembler -target amdgcn-amd-amdhsa -mcpu=${TARGET_ID} -o ${CODE_OBJECT} ${INPUT_FILE}")

  ## Add custom command to produce a code object file.
  add_custom_command(OUTPUT ${CODE_OBJECT} COMMAND clang ${CLANG_ARG_LIST}
    DEPENDS ${INPUT_FILE} clang
    COMMENT "BUILDING bitcode for ${OUTPUT_FILE}..."
    VERBATIM)

  separate_arguments(OBJCOPY_ARG_LIST UNIX_COMMAND "--dump-section=.text=${OUTPUT_FILE} ${CODE_OBJECT}")

  ## Extract .text segment
  add_custom_command(OUTPUT ${OUTPUT_FILE}
                     COMMAND llvm-objcopy ${OBJCOPY_ARG_LIST}
                     DEPENDS ${CODE_OBJECT} llvm-objcopy
                     COMMENT "Extracting binary for ${OUTPUT_FILE}..."
                     VERBATIM)

  if(${CMAKE_VERBOSE_MAKEFILE})
    message("     Trap Handler Source: " ${INPUT_FILE})
    message("     Trap Handler Binary: " ${OUTPUT_FILE})
  endif()

endfunction(gen_kernel_bc)

##==========================================
## Find device code object name and forward to custom command
##==========================================
function(build_kernel TRAP_HANDLER_NAME TARGET_ID POSTFIX SOURCE_SUFFIX)

  ## generate trap handler object code files
  set (CODE_OBJECT_FILE "${TRAP_HANDLER_NAME}_${POSTFIX}")
  set (TRAP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/trap_handler${SOURCE_SUFFIX}.s")
  gen_kernel_bc(${TARGET_ID} ${TRAP_FILE} ${CODE_OBJECT_FILE})

  ## Build a list of code object file names
  ## These will be target dependencies.
  set (HSACO_TARG_LIST ${HSACO_TARG_LIST} "${CODE_OBJECT_FILE}" PARENT_SCOPE)

endfunction(build_kernel)

##==========================================
## Build the kernel for a list of devices
##==========================================
function(build_kernel_for_devices TRAP_HANDLER_NAME)

  set(HSACO_TARG_LIST "")
  set(BUILT_OUTPUTS "")

  foreach(dev ${TARGET_DEVICES})
    # Determine source file suffix based on gfx architecture
    if(${dev} MATCHES "^gfx13")
      set(SOURCE_SUFFIX "_gfx13")
    elseif(${dev} MATCHES "^gfx1260")
      # gfx1260 brings many gfx13 features, and they share the same 1st level
      # trap handler ABI.
      set(SOURCE_SUFFIX "_gfx13")
    elseif(${dev} MATCHES "^gfx12")
      set(SOURCE_SUFFIX "_gfx12")
    else()
      set(SOURCE_SUFFIX "")
    endif()

    # Determine postfix based on gfx architecture
    if(${dev} MATCHES "^gfx9[45]")
      set(POSTFIX "942")
    elseif("${dev}" MATCHES "^gfx9")
      set(POSTFIX "9")
    elseif(${dev} STREQUAL "gfx1010")
      set(POSTFIX "1010")
    elseif(${dev} MATCHES "^gfx10")
      set(POSTFIX "10")
    elseif(${dev} MATCHES "^gfx11")
      set(POSTFIX "11")
    elseif(${dev} MATCHES "^gfx125")
      set(POSTFIX "1250")
    elseif(${dev} STREQUAL "gfx1260")
      set(POSTFIX "1260")
    elseif(${dev} MATCHES "^gfx12")
      set(POSTFIX "12")
    elseif(${dev} MATCHES "^gfx13")
      set(POSTFIX "13")
    else()
      message(WARNING "Unknown target device: ${dev}. Skipping trap handler generation.")
      continue()
    endif()

    # Temporary check to prevent regenerating trap handlers for devices that share 
    # them (e.g., gfx942 and gfx950) when building ROCR for all default targets.
    # TODO: Remove when architecture family grouping is implemented.
    list(FIND BUILT_OUTPUTS "${POSTFIX}" idx)
    if(NOT idx EQUAL -1)
      continue()
    endif()
    list(APPEND BUILT_OUTPUTS "${POSTFIX}")

    if (${CMAKE_VERBOSE_MAKEFILE})      
      message("\n  Generating: ${dev} ...")
    endif()

    build_kernel(${TRAP_HANDLER_NAME} ${dev} "${POSTFIX}" "${SOURCE_SUFFIX}")
  endforeach()
  
  set(HSACO_TARG_LIST ${HSACO_TARG_LIST} PARENT_SCOPE)

endfunction(build_kernel_for_devices)

##==========================================
## Create Trap Handler Object Code blobs file
##==========================================
function(generate_bytecodeStrm HeaderFILE)

  separate_arguments(ARG_LIST UNIX_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/${HeaderFILE}.h")
  set(ARG_LIST ${ARG_LIST} ${HSACO_TARG_LIST})

  ## Add a custom command that generates amd_trap_handler_v2.h
  ## This depends on all the generated code object files and the Python generator script.

  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_custom_command(
    OUTPUT ${HeaderFILE}.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/create_trap_handler_header.py ${ARG_LIST}
    COMMENT "Collating trap handlers..."
    DEPENDS ${HSACO_TARG_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/create_trap_handler_header.py)


  ## Export a target that builds (and depends on) amd_trap_handler_v2.h
  add_custom_target( ${HeaderFILE} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${HeaderFILE}.h )

endfunction(generate_bytecodeStrm)

##==========================================
## Main function calls
##==========================================

build_kernel_for_devices("kCodeTrapHandlerV2")
generate_bytecodeStrm("amd_trap_handler_v2")
