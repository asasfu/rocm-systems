/*
 * Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @addtogroup hipOccupancyDeviceVGPRValidation
 * @{
 * @ingroup OccupancyTest
 *
 * Validates that the device reports correct VGPR and LDS sizes for the
 * target architecture. These values come from COMGR ISA metadata and
 * the CLR ISA table. Incorrect values cause occupancy miscalculations
 * and kernel launch failures.
 */

#include <hip_test_common.hh>
#include <string>

/**
 * Test Description
 * ------------------------
 *  - Query hipDeviceAttributeMaxAvailableVgprsPerThread and validate it
 *    matches expected values per architecture.
 * Test source
 * ------------------------
 *  - catch/unit/occupancy/hipOccupancyDeviceVGPRValidation.cc
 * Test requirements
 * ------------------------
 */
HIP_TEST_CASE(Unit_hipOccupancy_DeviceVGPRCount_ArchValidation) {
  hipDeviceProp_t props;
  int device = 0;
  HIP_CHECK(hipGetDeviceProperties(&props, device));

  int maxVGPRs = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxVGPRs,
      hipDeviceAttributeMaxAvailableVgprsPerThread, device));

  std::string arch = props.gcnArchName;
  // Strip any target features (e.g., ":sramecc+:xnack-")
  auto colon = arch.find(':');
  if (colon != std::string::npos) {
    arch = arch.substr(0, colon);
  }

  INFO("Architecture: " << arch);
  INFO("MaxAvailableVgprsPerThread: " << maxVGPRs);

  // hipDeviceAttributeMaxAvailableVgprsPerThread reports the VGPRs a single
  // wave can address (AddressableNumVGPRs), not the physical per-SIMD file.
  // These coincide on archs with extended VGPR addressing (gfx1250/51, gfx1260)
  // but not on gfx1310/gfx1370, which cap addressing at the native 256 while the
  // SIMD file (1024) is shared across concurrent waves.
  if (arch == "gfx1260") {
    // Extended addressing, 1536 addressable VGPRs per thread
    REQUIRE(maxVGPRs == 1536);
  } else if (arch == "gfx1250" || arch == "gfx1251") {
    // Extended addressing, 1024 addressable VGPRs per thread
    REQUIRE(maxVGPRs == 1024);
  } else if (arch == "gfx1310" || arch == "gfx1370") {
    // Native addressing (no extended range): 256 addressable VGPRs per thread
    REQUIRE(maxVGPRs == 256);
  } else {
    INFO("No arch-specific VGPR check for " << arch << ", verifying > 0");
    REQUIRE(maxVGPRs > 0);
    REQUIRE(maxVGPRs <= 2048);
  }
}

/**
 * Test Description
 * ------------------------
 *  - Validate that the device reports the correct LDS (shared memory) size
 *    per CU for the target architecture.
 * Test source
 * ------------------------
 *  - catch/unit/occupancy/hipOccupancyDeviceVGPRValidation.cc
 * Test requirements
 * ------------------------
 */
HIP_TEST_CASE(Unit_hipOccupancy_DeviceLDSSize_ArchValidation) {
  hipDeviceProp_t props;
  int device = 0;
  HIP_CHECK(hipGetDeviceProperties(&props, device));

  std::string arch = props.gcnArchName;
  auto colon = arch.find(':');
  if (colon != std::string::npos) {
    arch = arch.substr(0, colon);
  }

  size_t ldsSize = props.maxSharedMemoryPerMultiProcessor;

  INFO("Architecture: " << arch);
  INFO("maxSharedMemoryPerMultiProcessor: " << ldsSize << " bytes (" << ldsSize / 1024
                                             << " KB)");

  if (arch == "gfx1260") {
    // 660 KB LDS per CU
    REQUIRE(ldsSize == 660u * 1024u);
  } else if (arch == "gfx1250" || arch == "gfx1251") {
    // 320 KB LDS per CU
    REQUIRE(ldsSize == 320u * 1024u);
  } else if (arch == "gfx1310" || arch == "gfx1370") {
    // GFX13: 192 KB LDS
    REQUIRE(ldsSize == 192u * 1024u);
  } else {
    INFO("No arch-specific LDS check for " << arch << ", verifying > 0");
    REQUIRE(ldsSize > 0);
  }
}
