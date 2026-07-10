# Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
#
# SPDX-License-Identifier: MIT

# Wavegroup tests - only for GPUs supporting wavegroups

set(TEST_SRC
    hipWavegroupLaunch.cc
)

set(WAVEGROUP_SUPPORTING_GPUS "gfx1260" "gfx1310" "gfx1370")
set(WAVEGROUP_SUPPORTED OFF)

foreach(WAVEGROUP_SUPPORTING_GPU IN LISTS WAVEGROUP_SUPPORTING_GPUS)
    string(FIND "${OFFLOAD_ARCH_STR}" "${WAVEGROUP_SUPPORTING_GPU}" _index)
    if(_index GREATER -1)
        set(WAVEGROUP_SUPPORTED ON)
        break()
    endif()
endforeach()

if(WAVEGROUP_SUPPORTED)
    message(STATUS "Wavegroups supported: ${OFFLOAD_ARCH_STR}")
    hip_add_exe_to_target(NAME WavegroupTest
                          TEST_SRC ${TEST_SRC}
                          TEST_TARGET_NAME build_tests)
endif()
