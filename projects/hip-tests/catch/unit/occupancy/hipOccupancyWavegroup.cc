/*
 * Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @addtogroup hipOccupancyWavegroup
 * @{
 * @ingroup OccupancyTest
 *
 * Tests for occupancy API behavior with wavegroup-enabled kernels.
 * Wavegroup kernels have different resource accounting:
 * - VGPRs are allocated per-wavegroup (shared), not per-wave
 * - Thread count must be a multiple of 128 (4 wavegroups x wave32)
 * - Scratch is allocated per-wavegroup, not per-wave
 */

#include <hip_test_common.hh>
#include <vector>

#define SKIP_IF_NOT_WAVEGROUP_DEVICE() \
  do { \
    int _wg_dev = -1, _wg_sup = 0; \
    HIP_CHECK(hipGetDevice(&_wg_dev)); \
    HIP_CHECK(hipDeviceGetAttribute(&_wg_sup, hipDeviceAttributeWavegroupLaunch, _wg_dev)); \
    if (!_wg_sup) { \
      HIP_SKIP_TEST("Not a wavegroup-capable device"); \
      return; \
    } \
  } while (0)

// WAVEGROUP_SUPPORT is set by CMake when building for a wavegroup-capable arch.
// The wavegroup attribute is additionally guarded per-architecture during device
// compilation because amdgpu_wavegroup_kernel crashes the backend on older archs.
#ifdef WAVEGROUP_SUPPORT

#if !defined(__HIP_DEVICE_COMPILE__) || \
    defined(__gfx1260__) || defined(__gfx1310__) || defined(__gfx1370__)
#define WAVEGROUP_KERNEL_ATTR \
    __attribute__((amdgpu_wavegroup_kernel(4, 32, 128, 1, 1)))
#define WAVEGROUP_KERNEL_ATTR_512 \
    __attribute__((amdgpu_wavegroup_kernel(4, 32, 512, 1, 1)))
#else
#define WAVEGROUP_KERNEL_ATTR
#define WAVEGROUP_KERNEL_ATTR_512
#endif

// Minimal wavegroup kernel
static __global__ WAVEGROUP_KERNEL_ATTR
void wavegroup_kern_128(float* out) {
  out[threadIdx.x] = 1.0f;
}

// Larger wavegroup kernel (4 waves per wavegroup)
static __global__ WAVEGROUP_KERNEL_ATTR_512
void wavegroup_kern_512(float* out) {
  out[threadIdx.x] = 1.0f;
}

// Wavegroup kernel with high VGPR usage via inline ASM clobbers.
// Forces the compiler to allocate many VGPRs per wave, making VGPRs
// the occupancy bottleneck (not wave slots). This exposes the occupancy
// miscalculation where usedVGPRs (per-wavegroup) is treated as per-wave.
static __global__ WAVEGROUP_KERNEL_ATTR_512
void wavegroup_kern_vgpr_heavy(float* out) {
#if __has_builtin(__builtin_amdgcn_wave_id_in_wavegroup)
  // Clobber v0-v127 to force 128 VGPRs per wave.
  // With 4 waves per wavegroup, total = 128*4 = 512 VGPRs per wavegroup.
  // This makes VGPRs the constraining resource for occupancy:
  //   1536 / 512 = 3 wavegroups per SIMD (VGPR limited)
  //   vs 16 / 4 = 4 wavegroups per SIMD (wave slot limited)
  //   Expected: 3 blocks per CU
  float val = static_cast<float>(threadIdx.x);
  asm volatile("" : "+v"(val) ::
    "v0","v1","v2","v3","v4","v5","v6","v7","v8","v9",
    "v10","v11","v12","v13","v14","v15","v16","v17","v18","v19",
    "v20","v21","v22","v23","v24","v25","v26","v27","v28","v29",
    "v30","v31","v32","v33","v34","v35","v36","v37","v38","v39",
    "v40","v41","v42","v43","v44","v45","v46","v47","v48","v49",
    "v50","v51","v52","v53","v54","v55","v56","v57","v58","v59",
    "v60","v61","v62","v63","v64","v65","v66","v67","v68","v69",
    "v70","v71","v72","v73","v74","v75","v76","v77","v78","v79",
    "v80","v81","v82","v83","v84","v85","v86","v87","v88","v89",
    "v90","v91","v92","v93","v94","v95","v96","v97","v98","v99",
    "v100","v101","v102","v103","v104","v105","v106","v107","v108","v109",
    "v110","v111","v112","v113","v114","v115","v116","v117","v118","v119",
    "v120","v121","v122","v123","v124","v125","v126","v127"
  );
  out[threadIdx.x] = val;
#else
  out[threadIdx.x] = 1.0f;
#endif
}


/**
 * Test Description
 * ------------------------
 *  - Verify occupancy APIs return valid results for wavegroup kernels.
 *    MaxActiveBlocksPerMultiprocessor must return > 0 for both 128 and
 *    512-thread wavegroup kernels. MaxPotentialBlockSize must return
 *    exactly the kernel's declared block size (fixed at compile time).
 * Test source
 * ------------------------
 *  - catch/unit/occupancy/hipOccupancyWavegroup.cc
 * Test requirements
 * ------------------------
 *  - Device supports wavegroups
 */
HIP_TEST_CASE(Unit_hipOccupancyWavegroup_BasicOccupancyQueries) {
  SKIP_IF_NOT_WAVEGROUP_DEVICE();

  // MaxActiveBlocks for 128- and 512-thread wavegroup kernels
  int numBlocks128 = 0, numBlocks512 = 0;
  HIP_CHECK(hipOccupancyMaxActiveBlocksPerMultiprocessor(
      &numBlocks128, wavegroup_kern_128, 128, 0));
  HIP_CHECK(hipOccupancyMaxActiveBlocksPerMultiprocessor(
      &numBlocks512, wavegroup_kern_512, 512, 0));

  INFO("numBlocks (128 threads): " << numBlocks128);
  INFO("numBlocks (512 threads): " << numBlocks512);
  REQUIRE(numBlocks128 > 0);
  REQUIRE(numBlocks512 > 0);

  // MaxPotentialBlockSize must return the kernel's fixed block size
  int gridSize = 0, blockSize = 0;
  HIP_CHECK(hipOccupancyMaxPotentialBlockSize(
      &gridSize, &blockSize, wavegroup_kern_128, 0, 0));

  INFO("Potential block size: " << blockSize);
  INFO("Potential grid size: " << gridSize);
  REQUIRE(blockSize == 128);
  REQUIRE(gridSize > 0);
}

/**
 * Test Description
 * ------------------------
 *  - Verify occupancy with shared memory for wavegroup kernels.
 *    Wavegroup workgroups can use up to the full LDS allocation.
 * Test source
 * ------------------------
 *  - catch/unit/occupancy/hipOccupancyWavegroup.cc
 * Test requirements
 * ------------------------
 *  - Device supports wavegroups
 */
HIP_TEST_CASE(Unit_hipOccupancyWavegroup_WithSharedMemory) {
  SKIP_IF_NOT_WAVEGROUP_DEVICE();
  int numBlocks_noshmem = 0;
  int numBlocks_shmem = 0;

  HIP_CHECK(hipOccupancyMaxActiveBlocksPerMultiprocessor(
      &numBlocks_noshmem, wavegroup_kern_128, 128, 0));
  HIP_CHECK(hipOccupancyMaxActiveBlocksPerMultiprocessor(
      &numBlocks_shmem, wavegroup_kern_128, 128, 32 * 1024));

  INFO("Wavegroup blocks (no shmem): " << numBlocks_noshmem);
  INFO("Wavegroup blocks (32KB shmem): " << numBlocks_shmem);

  REQUIRE(numBlocks_noshmem > 0);
  REQUIRE(numBlocks_shmem > 0);
  // Shared memory should constrain occupancy
  REQUIRE(numBlocks_shmem <= numBlocks_noshmem);
}

/**
 * Test Description
 * ------------------------
 *  - Verify that hipOccupancyAvailableDynamicSMemPerBlock returns a valid
 *    value for wavegroup kernels.
 * Test source
 * ------------------------
 *  - catch/unit/occupancy/hipOccupancyWavegroup.cc
 * Test requirements
 * ------------------------
 *  - Device supports wavegroups
 */
HIP_TEST_CASE(Unit_hipOccupancyWavegroup_AvailableDynamicSMem) {
  SKIP_IF_NOT_WAVEGROUP_DEVICE();
  size_t dynamicSmemSize = 0;

  HIP_CHECK(hipOccupancyAvailableDynamicSMemPerBlock(
      &dynamicSmemSize, wavegroup_kern_128, 1, 128));

  INFO("Available dynamic shared memory for wavegroup kernel: " << dynamicSmemSize);
  REQUIRE(dynamicSmemSize > 0);
}

/**
 * Test Description
 * ------------------------
 *  - Verify that hipOccupancyMaxActiveBlocksPerMultiprocessor returns the correct
 *    number of blocks for a high-VGPR wavegroup kernel. This test exposes the
 *    occupancy miscalculation where usedVGPRs (per-wavegroup) is treated as
 *    per-wave, causing underestimated occupancy.
 *
 *    The kernel clobbers 128 VGPRs per wave. With 4 waves per wavegroup,
 *    usedVGPRs = 128*4 = 512 per wavegroup. The correct occupancy calculation:
 *      wavegroups_per_simd = vgprsPerSimd / alignUp(usedVGPRs, granularity)
 *      blocks_per_cu = min(wavegroups_per_simd, MaxWavesPerSimd / wavesPerWavegroup)
 *
 *    The buggy calculation treats usedVGPRs as per-wave, getting
 *    alu_limited_threads < blockSize, and returning 0 blocks.
 * Test source
 * ------------------------
 *  - catch/unit/occupancy/hipOccupancyWavegroup.cc
 * Test requirements
 * ------------------------
 *  - Device supports wavegroups
 */
HIP_TEST_CASE(Unit_hipOccupancyWavegroup_OccupancyCorrectness_HighVGPR) {
  SKIP_IF_NOT_WAVEGROUP_DEVICE();
  // Query kernel attributes
  hipFuncAttributes attr;
  HIP_CHECK(hipFuncGetAttributes(&attr,
      reinterpret_cast<const void*>(wavegroup_kern_vgpr_heavy)));

  int usedVGPRs = attr.numRegs;
  int blockSize = attr.maxThreadsPerBlock;

  INFO("=== Occupancy correctness for high-VGPR wavegroup kernel ===");
  INFO("usedVGPRs (per-wavegroup): " << usedVGPRs);
  INFO("maxThreadsPerBlock: " << blockSize);

  // Query occupancy using the kernel's own compiled block size
  int numBlocks = 0;
  HIP_CHECK(hipOccupancyMaxActiveBlocksPerMultiprocessor(
      &numBlocks, wavegroup_kern_vgpr_heavy, blockSize, 0));

  INFO("hipOccupancyMaxActiveBlocksPerMultiprocessor returned: " << numBlocks);

  // The kernel should be launchable. With usedVGPRs per-wavegroup and
  // 1536 VGPRs per SIMD, multiple wavegroups should fit.
  // The buggy occupancy code treats usedVGPRs as per-wave, which can
  // cause alu_limited_threads < blockSize, returning 0 blocks.
  //
  // Correct calculation:
  //   vgprWavegroups = vgprsPerSimd / alignUp(usedVGPRs, granularity)
  //   blocks = min(vgprWavegroups, MaxWavesPerSimd / wavesPerWavegroup)
  //
  // For example, with usedVGPRs=264, vgprsPerSimd=1536, granularity=16:
  //   vgprWavegroups = 1536 / 272 = 5
  //   slotWavegroups = 16 / 4 = 4
  //   expected = min(5, 4) = 4 blocks per CU
  REQUIRE(numBlocks > 0);
}

#endif // WAVEGROUP_SUPPORT
