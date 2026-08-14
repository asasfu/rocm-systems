/*
 * Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @addtogroup hipWavegroupLaunch
 * @{
 * @ingroup WavegroupTest
 *
 * Tests for wavegroup kernel launch validation.
 * Wavegroup kernels must be launched with the exact block size
 * specified in the kernel definition.
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

#if !defined(__HIP_DEVICE_COMPILE__) || \
    defined(__gfx1260__) || defined(__gfx1310__) || defined(__gfx1370__)
#define WAVEGROUP_KERNEL_ATTR \
    __attribute__((amdgpu_wavegroup_kernel(4, 32, 128, 1, 1)))
// 2-D compile-time block shape (64, 2, 1) — same 128-thread total as above.
#define WAVEGROUP_KERNEL_ATTR_64X2 \
    __attribute__((amdgpu_wavegroup_kernel(4, 32, 64, 2, 1)))
#else
#define WAVEGROUP_KERNEL_ATTR
#define WAVEGROUP_KERNEL_ATTR_64X2
#endif

// Minimal wavegroup kernel (128 threads)
static __global__ WAVEGROUP_KERNEL_ATTR
void wavegroup_kern_128(float* out) {
  out[threadIdx.x] = 1.0f;
}

// Wavegroup kernel with a 2-D compile-time block shape (64, 2, 1).
static __global__ WAVEGROUP_KERNEL_ATTR_64X2
void wavegroup_kern_64x2(float* out) {
  out[threadIdx.y * 64 + threadIdx.x] = 1.0f;
}

/**
 * Test Description
 * ------------------------
 *  - Wavegroup kernels must be launched with the exact block size from the
 *    kernel definition. Any other size — too small, too large, or even a
 *    valid multiple of 128 — must be rejected. Only the declared size (128)
 *    should succeed and produce correct results.
 * Test source
 * ------------------------
 *  - catch/unit/wavegroup/hipWavegroupLaunch.cc
 * Test requirements
 * ------------------------
 *  - Device supports wavegroups
 */
HIP_TEST_CASE(Unit_hipWavegroup_LaunchValidation) {
  SKIP_IF_NOT_WAVEGROUP_DEVICE();
  constexpr int kCorrectSize = 128;
  float* d_out = nullptr;
  HIP_CHECK(hipMalloc(&d_out, 256 * sizeof(float)));

  // Invalid block sizes must all be rejected.
  for (int badSize : {32, 64, 256}) {
    wavegroup_kern_128<<<dim3(1), dim3(badSize), 0, 0>>>(d_out);
    hipError_t err = hipGetLastError();
    INFO("Block size " << badSize << " should be rejected, err=" << err);
    REQUIRE(err != hipSuccess);
  }

  // The declared block size must succeed.
  wavegroup_kern_128<<<dim3(1), dim3(kCorrectSize), 0, 0>>>(d_out);
  HIP_CHECK(hipGetLastError());
  HIP_CHECK(hipDeviceSynchronize());

  // Verify the kernel actually ran.
  std::vector<float> h_out(kCorrectSize);
  HIP_CHECK(hipMemcpy(h_out.data(), d_out, kCorrectSize * sizeof(float), hipMemcpyDeviceToHost));
  for (int i = 0; i < kCorrectSize; i++) {
    REQUIRE(h_out[i] == 1.0f);
  }

  HIP_CHECK(hipFree(d_out));
}

/**
 * Test Description
 * ------------------------
 *  - Wavegroup block-size validation must compare each dimension, not just the
 *    total thread count. A kernel compiled with a 2-D block shape (64, 2, 1)
 *    must reject reshaped launches that have the same product (128) but a
 *    different shape — e.g. (128, 1, 1), (2, 64, 1), (32, 4, 1) — because the
 *    compiler synthesizes threadIdx from the compile-time shape. Only the
 *    declared (64, 2, 1) shape should succeed.
 * Test source
 * ------------------------
 *  - catch/unit/wavegroup/hipWavegroupLaunch.cc
 * Test requirements
 * ------------------------
 *  - Device supports wavegroups
 */
HIP_TEST_CASE(Unit_hipWavegroup_LaunchValidation_PerDimension) {
  SKIP_IF_NOT_WAVEGROUP_DEVICE();
  constexpr int kThreads = 128;  // 64 * 2 * 1
  float* d_out = nullptr;
  HIP_CHECK(hipMalloc(&d_out, kThreads * sizeof(float)));

  // Same total thread count (128) but wrong shape — must all be rejected.
  // The previous product-only check would have incorrectly accepted these.
  for (dim3 badShape : {dim3(128, 1, 1), dim3(2, 64, 1), dim3(32, 4, 1)}) {
    wavegroup_kern_64x2<<<dim3(1), badShape, 0, 0>>>(d_out);
    hipError_t err = hipGetLastError();
    INFO("Block shape (" << badShape.x << ", " << badShape.y << ", " << badShape.z
         << ") should be rejected, err=" << err);
    REQUIRE(err != hipSuccess);
  }

  // The declared block shape (64, 2, 1) must succeed.
  wavegroup_kern_64x2<<<dim3(1), dim3(64, 2, 1), 0, 0>>>(d_out);
  HIP_CHECK(hipGetLastError());
  HIP_CHECK(hipDeviceSynchronize());

  // Verify the kernel actually ran.
  std::vector<float> h_out(kThreads);
  HIP_CHECK(hipMemcpy(h_out.data(), d_out, kThreads * sizeof(float), hipMemcpyDeviceToHost));
  for (int i = 0; i < kThreads; i++) {
    REQUIRE(h_out[i] == 1.0f);
  }

  HIP_CHECK(hipFree(d_out));
}
