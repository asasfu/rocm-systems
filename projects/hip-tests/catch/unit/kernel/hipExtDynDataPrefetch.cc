/*
 * Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#include <hip_test_common.hh>
#include <hip_test_kernels.hh>

#include <algorithm>
#include <vector>

// Simple DAXPY kernel: out[i] = A[i] + alpha * B[i]
__global__ void daxpyKernel(const float* A, const float* B, float* out,
                            float alpha, int N) {
  int tid = blockIdx.x * blockDim.x + threadIdx.x;
  if (tid < N) {
    out[tid] = A[tid] + alpha * B[tid];
  }
}

static hipLaunchAttribute makeDynDataPrefetchAttr(
    const hipExtDynDataPrefetchConfig* prefetchConfig) {
  hipLaunchAttribute attr;
  memset(&attr, 0, sizeof(attr));
  attr.id = hipLaunchAttributeExtDynDataPrefetch;
  attr.val.dynDataPrefetch = prefetchConfig;
  return attr;
}

// Verify out[i] == A[i] + alpha * B[i] for a DAXPY result.
static void verifyDaxpy(const std::vector<float>& A, const std::vector<float>& B,
                        const std::vector<float>& C, float alpha) {
  for (size_t i = 0; i < C.size(); ++i) {
    REQUIRE(C[i] == Catch::Approx(A[i] + alpha * B[i]).epsilon(1e-5f));
  }
}

// Build a single 1D prefetch region covering an entire buffer.
static hipExtDynDataPrefetchRegion makeWholeBufferRegion(void* addr, size_t bytes) {
  hipExtDynDataPrefetchRegion region = {};
  region.address = addr;
  region.width   = (bytes / 256) * 256;
  region.height  = 1;
  region.stride  = (bytes / 256) * 256;
  return region;
}

/**
 * @addtogroup hipExtDynDataPrefetch hipExtDynDataPrefetch
 * @{
 * @ingroup KernelTest
 * Test the hipLaunchAttributeExtDynDataPrefetch launch attribute.
 */

/**
 * Test Description
 * ------------------------
 * Negative tests for hipLaunchAttributeExtDynDataPrefetch validation.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_NegativeTests) {
  constexpr int N = 1024;
  float* d_A = nullptr;
  HIP_CHECK(hipMalloc(&d_A, N * sizeof(float)));

  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));

  auto makeConfig = [](hipLaunchAttribute* attr, int numAttrs) {
    hipLaunchConfig_t cfg = {};
    cfg.gridDim  = dim3{4, 1, 1};
    cfg.blockDim = dim3{256, 1, 1};
    cfg.dynamicSmemBytes = 0;
    cfg.stream   = 0;
    cfg.attrs    = attr;
    cfg.numAttrs = numAttrs;
    return cfg;
  };

  // On devices without prefetch support (maxRegions == 0), all attempts to use
  // the attribute return hipErrorNotSupported regardless of other parameters.
  hipError_t unsupportedOrInvalid =
      (maxRegions == 0) ? hipErrorNotSupported : hipErrorInvalidValue;

  SECTION("device query returns non-negative max regions") {
    REQUIRE(maxRegions >= 0);
  }

  SECTION("numRegions == 0 returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = d_A;
    region.width   = 256;
    region.height  = 1;
    region.stride  = 256;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 0;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  SECTION("numRegions > device max returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = d_A;
    region.width   = 256;
    region.height  = 1;
    region.stride  = 256;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = static_cast<unsigned int>(maxRegions) + 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  SECTION("null address in first region returns error") {
    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    // regions[0].address is 0 (null) from memset
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  SECTION("null address in region returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = nullptr;
    region.width   = 256;
    region.height  = 1;
    region.stride  = 256;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  SECTION("unaligned address returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(d_A) + 1);
    region.width   = 256;
    region.height  = 1;
    region.stride  = 256;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  SECTION("width == 0 returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = d_A;
    region.width   = 0;
    region.height  = 1;
    region.stride  = 256;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  SECTION("width not a multiple of 256 returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = d_A;
    region.width   = 300;
    region.height  = 1;
    region.stride  = 300;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  SECTION("height == 0 returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = d_A;
    region.width   = 256;
    region.height  = 0;
    region.stride  = 256;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  // (9) Stride validation: stride must be non-zero, 256-aligned, and >= width.
  SECTION("stride == 0 returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = d_A;
    region.width   = 256;
    region.height  = 2;
    region.stride  = 0;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  SECTION("stride < width returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = d_A;
    region.width   = 512;
    region.height  = 1;
    region.stride  = 256;  // < width

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  SECTION("stride not a multiple of 256 returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = d_A;
    region.width   = 256;
    region.height  = 1;
    region.stride  = 300;  // not 256-aligned

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  // (10) Every populated slot is validated, not just the first: a valid region[0]
  // paired with an invalid region[1] must still be rejected.
  SECTION("second region invalid in multi-region config returns error") {
    hipExtDynDataPrefetchRegion region0 = {};
    region0.address = d_A;
    region0.width   = 256;
    region0.height  = 1;
    region0.stride  = 256;

    hipExtDynDataPrefetchRegion region1 = {};
    region1.address = nullptr;  // invalid
    region1.width   = 256;
    region1.height  = 1;
    region1.stride  = 256;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 2;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region0;
    prefetchConfig.regions[1] = region1;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  // (11) Geometry that exceeds the encodable field range is rejected. width is
  // encoded as (width/256 - 1) in 11 bits, so width/256 must be <= 2048.
  // Note: the runtime validates the encodable field range but does NOT validate
  // the region against the actual allocation size, so an oversized-but-encodable
  // region is not caught here.
  SECTION("width exceeding max burst size returns error") {
    hipExtDynDataPrefetchRegion region = {};
    region.address = d_A;
    region.width   = (2048u + 1u) * 256u;  // width/256 = 2049 > 2048
    region.height  = 1;
    region.stride  = (2048u + 1u) * 256u;

    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = 1;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    prefetchConfig.regions[0] = region;
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    auto cfg = makeConfig(&attr, 1);
    void* args[] = {&d_A, &d_A, &d_A, nullptr, nullptr};
    HIP_CHECK_ERROR(hipLaunchKernelExC(&cfg, reinterpret_cast<void*>(daxpyKernel), args),
                    unsupportedOrInvalid);
  }

  HIP_CHECK(hipFree(d_A));
}

/**
 * Test Description
 * ------------------------
 * Functional test: launch a DAXPY kernel with L2 prefetch of both input
 * buffers via hipLaunchAttributeExtDynDataPrefetch and verify correctness.
 * Devices without dynamic data prefetch support skip this test.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_FunctionalDaxpy) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 2) {
    HIP_SKIP_TEST("Device does not support >= 2 prefetch regions");
    return;
  }

  constexpr int N = 4096;
  constexpr float alpha = 2.0f;

  size_t bytes = N * sizeof(float);
  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N), h_B(N), h_C(N);
  for (int i = 0; i < N; ++i) {
    h_A[i] = static_cast<float>(i);
    h_B[i] = static_cast<float>(i * 0.5f);
  }
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  size_t width = (bytes / 256) * 256;

  hipExtDynDataPrefetchRegion regions[2] = {};
  regions[0].address = d_A;
  regions[0].width   = width;
  regions[0].height  = 1;
  regions[0].stride  = width;

  regions[1].address = d_B;
  regions[1].width   = width;
  regions[1].height  = 1;
  regions[1].stride  = width;

  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 2;
  prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
  prefetchConfig.regions[0] = regions[0];
  prefetchConfig.regions[1] = regions[1];
  hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

  hipLaunchConfig_t config = {};
  constexpr int blockSize = 256;
  config.gridDim  = dim3{(N + blockSize - 1) / blockSize, 1, 1};
  config.blockDim = dim3{blockSize, 1, 1};
  config.dynamicSmemBytes = 0;
  config.stream   = 0;
  config.attrs    = &attr;
  config.numAttrs = 1;

  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};
  HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipDeviceSynchronize());

  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));

  for (int i = 0; i < N; ++i) {
    float expected = h_A[i] + alpha * h_B[i];
    REQUIRE(h_C[i] == Catch::Approx(expected).epsilon(1e-5f));
  }

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * Test Description
 * ------------------------
 * Functional test: launch a kernel with a single prefetch region.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_SingleRegion) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 1024;
  constexpr float alpha = 1.0f;
  size_t bytes = N * sizeof(float);

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N, 1.0f), h_B(N, 2.0f), h_C(N);
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  size_t width = (bytes / 256) * 256;

  hipExtDynDataPrefetchRegion region = {};
  region.address  = d_A;
  region.width    = width;
  region.height   = 1;
  region.stride   = width;

  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 1;
  prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
  prefetchConfig.regions[0] = region;
  hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

  hipLaunchConfig_t config = {};
  constexpr int blockSize = 256;
  config.gridDim  = dim3{(N + blockSize - 1) / blockSize, 1, 1};
  config.blockDim = dim3{blockSize, 1, 1};
  config.dynamicSmemBytes = 0;
  config.stream   = 0;
  config.attrs    = &attr;
  config.numAttrs = 1;

  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};
  HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipDeviceSynchronize());

  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));

  for (int i = 0; i < N; ++i) {
    float expected = 1.0f + alpha * 2.0f;
    REQUIRE(h_C[i] == Catch::Approx(expected).epsilon(1e-5f));
  }

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * Test Description
 * ------------------------
 * Test all temporal hint combinations via hipLaunchAttributeExtDynDataPrefetch.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_TemporalHints) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 256;
  constexpr float alpha = 1.0f;
  size_t bytes = N * sizeof(float);

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N, 3.0f), h_B(N, 4.0f), h_C(N);
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  auto temporal = GENERATE(hipExtDynDataPrefetchTemporalRegular,
                           hipExtDynDataPrefetchTemporalHigh);

  size_t width = (bytes / 256) * 256;

  hipExtDynDataPrefetchRegion region = {};
  region.address = d_A;
  region.width   = width;
  region.height  = 1;
  region.stride  = width;

  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 1;
  prefetchConfig.temporal   = temporal;
  prefetchConfig.regions[0] = region;
  hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

  hipLaunchConfig_t config = {};
  config.gridDim  = dim3{1, 1, 1};
  config.blockDim = dim3{(unsigned)N, 1, 1};
  config.dynamicSmemBytes = 0;
  config.stream   = 0;
  config.attrs    = &attr;
  config.numAttrs = 1;

  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};
  HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipDeviceSynchronize());

  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
  for (int i = 0; i < N; ++i) {
    REQUIRE(h_C[i] == Catch::Approx(3.0f + alpha * 4.0f).epsilon(1e-5f));
  }

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * Test Description
 * ------------------------
 * (1) Functional test: prefetch a true 2D strided region (height > 1 and
 * stride > width) and verify DAXPY correctness. Exercises the row-pitch path
 * that the existing 1D tests (height == 1, stride == width) never cover.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_2DStridedRegion) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 4096;
  constexpr float alpha = 2.0f;
  size_t bytes = N * sizeof(float);

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N), h_B(N), h_C(N);
  for (int i = 0; i < N; ++i) {
    h_A[i] = static_cast<float>(i);
    h_B[i] = static_cast<float>(i) * 0.5f;
  }
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  // 8 rows of 512 B each, row pitch 1024 B (stride > width). Highest byte
  // touched = 7 * 1024 + 512 = 7680 B, well within the 16384 B allocation.
  hipExtDynDataPrefetchRegion region = {};
  region.address = d_A;
  region.width   = 512;
  region.height  = 8;
  region.stride  = 1024;

  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 1;
  prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
  prefetchConfig.regions[0] = region;
  hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

  hipLaunchConfig_t config = {};
  constexpr int blockSize = 256;
  config.gridDim  = dim3{(N + blockSize - 1) / blockSize, 1, 1};
  config.blockDim = dim3{blockSize, 1, 1};
  config.dynamicSmemBytes = 0;
  config.stream   = 0;
  config.attrs    = &attr;
  config.numAttrs = 1;

  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};
  HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipDeviceSynchronize());
  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
  verifyDaxpy(h_A, h_B, h_C, alpha);

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * Test Description
 * ------------------------
 * (2) Prefetch config must not leak across launches: a prefetch launch,
 * followed by a launch with no attributes, followed by another prefetch
 * launch, must each succeed and produce correct results. Guards the
 * preloader state reset (ClearDynDataPrefetchConfig).
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_StateIsolation) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 1024;
  constexpr float alpha = 1.0f;
  size_t bytes = N * sizeof(float);

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N, 1.0f), h_B(N, 2.0f), h_C(N);
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  hipExtDynDataPrefetchRegion region = makeWholeBufferRegion(d_A, bytes);
  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 1;
  prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
  prefetchConfig.regions[0] = region;
  hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

  constexpr int blockSize = 256;
  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};

  hipLaunchConfig_t withPrefetch = {};
  withPrefetch.gridDim  = dim3{(N + blockSize - 1) / blockSize, 1, 1};
  withPrefetch.blockDim = dim3{blockSize, 1, 1};
  withPrefetch.stream   = 0;
  withPrefetch.attrs    = &attr;
  withPrefetch.numAttrs = 1;

  hipLaunchConfig_t noPrefetch = withPrefetch;
  noPrefetch.attrs    = nullptr;
  noPrefetch.numAttrs = 0;

  // Launch 1: with prefetch.
  HIP_CHECK(hipLaunchKernelExC(&withPrefetch, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipDeviceSynchronize());
  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
  verifyDaxpy(h_A, h_B, h_C, alpha);

  // Launch 2: no attributes (must not inherit the previous prefetch state).
  std::fill(h_C.begin(), h_C.end(), 0.0f);
  HIP_CHECK(hipMemcpy(d_C, h_C.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipLaunchKernelExC(&noPrefetch, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipDeviceSynchronize());
  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
  verifyDaxpy(h_A, h_B, h_C, alpha);

  // Launch 3: with prefetch again.
  std::fill(h_C.begin(), h_C.end(), 0.0f);
  HIP_CHECK(hipMemcpy(d_C, h_C.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipLaunchKernelExC(&withPrefetch, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipDeviceSynchronize());
  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
  verifyDaxpy(h_A, h_B, h_C, alpha);

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * Test Description
 * ------------------------
 * (3) Stress the per-queue metadata ring buffer by issuing many prefetch
 * launches in a row so the ring wraps around, then verify the final result.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_RepeatedLaunch) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 256;
  constexpr float alpha = 1.0f;
  constexpr int kIterations = 2048;
  size_t bytes = N * sizeof(float);

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N, 3.0f), h_B(N, 4.0f), h_C(N);
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  hipExtDynDataPrefetchRegion region = makeWholeBufferRegion(d_A, bytes);
  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 1;
  prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
  prefetchConfig.regions[0] = region;
  hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

  hipLaunchConfig_t config = {};
  config.gridDim  = dim3{1, 1, 1};
  config.blockDim = dim3{static_cast<unsigned>(N), 1, 1};
  config.stream   = 0;
  config.attrs    = &attr;
  config.numAttrs = 1;

  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};
  for (int it = 0; it < kIterations; ++it) {
    HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
  }
  HIP_CHECK(hipDeviceSynchronize());
  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
  verifyDaxpy(h_A, h_B, h_C, alpha);

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * Test Description
 * ------------------------
 * (4) Prefetch on a non-default (user-created) stream with async copies.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_NonDefaultStream) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 2048;
  constexpr float alpha = 2.0f;
  size_t bytes = N * sizeof(float);

  hipStream_t stream = nullptr;
  HIP_CHECK(hipStreamCreate(&stream));

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N), h_B(N), h_C(N);
  for (int i = 0; i < N; ++i) {
    h_A[i] = static_cast<float>(i);
    h_B[i] = static_cast<float>(i) * 0.25f;
  }
  HIP_CHECK(hipMemcpyAsync(d_A, h_A.data(), bytes, hipMemcpyHostToDevice, stream));
  HIP_CHECK(hipMemcpyAsync(d_B, h_B.data(), bytes, hipMemcpyHostToDevice, stream));

  hipExtDynDataPrefetchRegion region = makeWholeBufferRegion(d_A, bytes);
  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 1;
  prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
  prefetchConfig.regions[0] = region;
  hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

  hipLaunchConfig_t config = {};
  constexpr int blockSize = 256;
  config.gridDim  = dim3{(N + blockSize - 1) / blockSize, 1, 1};
  config.blockDim = dim3{blockSize, 1, 1};
  config.stream   = stream;
  config.attrs    = &attr;
  config.numAttrs = 1;

  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};
  HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipMemcpyAsync(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost, stream));
  HIP_CHECK(hipStreamSynchronize(stream));
  verifyDaxpy(h_A, h_B, h_C, alpha);

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * (5) Capture a prefetch launch into a hipGraph, instantiate, replay, and
 * verify correctness. Confirms the launch attribute survives the graph path.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_GraphCaptureReplay) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 1024;
  constexpr float alpha = 1.5f;
  size_t bytes = N * sizeof(float);

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N, 2.0f), h_B(N, 3.0f), h_C(N);
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  hipExtDynDataPrefetchRegion region = makeWholeBufferRegion(d_A, bytes);
  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 1;
  prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
  prefetchConfig.regions[0] = region;
  hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

  hipStream_t stream = nullptr;
  HIP_CHECK(hipStreamCreate(&stream));

  hipLaunchConfig_t config = {};
  constexpr int blockSize = 256;
  config.gridDim  = dim3{(N + blockSize - 1) / blockSize, 1, 1};
  config.blockDim = dim3{blockSize, 1, 1};
  config.stream   = stream;
  config.attrs    = &attr;
  config.numAttrs = 1;

  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};

  HIP_CHECK(hipStreamBeginCapture(stream, hipStreamCaptureModeGlobal));
  HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
  hipGraph_t graph = nullptr;
  HIP_CHECK(hipStreamEndCapture(stream, &graph));

  hipGraphExec_t graphExec = nullptr;
  HIP_CHECK(hipGraphInstantiate(&graphExec, graph, nullptr, nullptr, 0));
  HIP_CHECK(hipGraphLaunch(graphExec, stream));
  HIP_CHECK(hipStreamSynchronize(stream));
  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
  verifyDaxpy(h_A, h_B, h_C, alpha);

  HIP_CHECK(hipGraphExecDestroy(graphExec));
  HIP_CHECK(hipGraphDestroy(graph));
  HIP_CHECK(hipStreamDestroy(stream));
  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * Test Description
 * ------------------------
 * (6) Exercise every supported region count from 1 to the device maximum,
 * rather than only the hardcoded 1 and 2 counts the other tests use.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_ParametrizedNumRegions) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 1024;
  constexpr float alpha = 2.0f;
  size_t bytes = N * sizeof(float);

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N), h_B(N), h_C(N);
  for (int i = 0; i < N; ++i) {
    h_A[i] = static_cast<float>(i);
    h_B[i] = static_cast<float>(N - i);
  }
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  // The config's regions[] array is fixed-size, so never index past it even if
  // a device reports a larger maximum than the struct can hold.
  unsigned int testMax = static_cast<unsigned int>(maxRegions);
  if (testMax > HIP_EXT_DYN_DATA_PREFETCH_MAX_REGIONS) {
    testMax = HIP_EXT_DYN_DATA_PREFETCH_MAX_REGIONS;
  }
  for (unsigned int r = 1; r <= testMax; ++r) {
    hipExtDynDataPrefetchConfig prefetchConfig = {};
    prefetchConfig.numRegions = r;
    prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
    // Each region prefetches a distinct 256 B window of d_A, all in-bounds.
    for (unsigned int i = 0; i < r; ++i) {
      hipExtDynDataPrefetchRegion region = {};
      region.address = reinterpret_cast<void*>(
          reinterpret_cast<uintptr_t>(d_A) + static_cast<uintptr_t>(i) * 256u);
      region.width   = 256;
      region.height  = 1;
      region.stride  = 256;
      prefetchConfig.regions[i] = region;
    }
    hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

    hipLaunchConfig_t config = {};
    constexpr int blockSize = 256;
    config.gridDim  = dim3{(N + blockSize - 1) / blockSize, 1, 1};
    config.blockDim = dim3{blockSize, 1, 1};
    config.stream   = 0;
    config.attrs    = &attr;
    config.numAttrs = 1;

    std::fill(h_C.begin(), h_C.end(), 0.0f);
    HIP_CHECK(hipMemcpy(d_C, h_C.data(), bytes, hipMemcpyHostToDevice));

    int n = N;
    void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};
    HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
    HIP_CHECK(hipDeviceSynchronize());
    HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
    verifyDaxpy(h_A, h_B, h_C, alpha);
  }

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * Test Description
 * ------------------------
 * (7) Combine the prefetch attribute with another recognized launch attribute
 * (cooperative, value 0) in the same launch config. Confirms multi-attribute
 * parsing handles the prefetch entry alongside others.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_CombinedAttributes) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 1024;
  constexpr float alpha = 1.0f;
  size_t bytes = N * sizeof(float);

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N, 5.0f), h_B(N, 6.0f), h_C(N);
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  hipExtDynDataPrefetchRegion region = makeWholeBufferRegion(d_A, bytes);
  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 1;
  prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
  prefetchConfig.regions[0] = region;

  hipLaunchAttribute attrs[2];
  attrs[0] = makeDynDataPrefetchAttr(&prefetchConfig);
  memset(&attrs[1], 0, sizeof(attrs[1]));
  attrs[1].id = hipLaunchAttributeCooperative;
  attrs[1].val.cooperative = 0;

  hipLaunchConfig_t config = {};
  constexpr int blockSize = 256;
  config.gridDim  = dim3{(N + blockSize - 1) / blockSize, 1, 1};
  config.blockDim = dim3{blockSize, 1, 1};
  config.stream   = 0;
  config.attrs    = attrs;
  config.numAttrs = 2;

  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};
  HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipDeviceSynchronize());
  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
  verifyDaxpy(h_A, h_B, h_C, alpha);

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * Test Description
 * ------------------------
 * (8) Prefetch a region smaller than the data the kernel actually reads, and
 * verify results are still correct. Prefetch is a hint, so correctness must
 * not depend on the prefetched region covering all accessed memory.
 *
 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipExtDynDataPrefetch.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 6.5
 */
HIP_TEST_CASE(Unit_hipExtDynDataPrefetch_PrefetchSubset) {
  int maxRegions = 0;
  HIP_CHECK(hipDeviceGetAttribute(&maxRegions, hipDeviceAttributeMaxDynDataPrefetchRegions, 0));
  if (maxRegions < 1) {
    HIP_SKIP_TEST("Device does not support prefetch regions");
    return;
  }

  constexpr int N = 4096;
  constexpr float alpha = 2.0f;
  size_t bytes = N * sizeof(float);

  float* d_A = nullptr;
  float* d_B = nullptr;
  float* d_C = nullptr;
  HIP_CHECK(hipMalloc(&d_A, bytes));
  HIP_CHECK(hipMalloc(&d_B, bytes));
  HIP_CHECK(hipMalloc(&d_C, bytes));

  std::vector<float> h_A(N), h_B(N), h_C(N);
  for (int i = 0; i < N; ++i) {
    h_A[i] = static_cast<float>(i);
    h_B[i] = static_cast<float>(i) * 0.5f;
  }
  HIP_CHECK(hipMemcpy(d_A, h_A.data(), bytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_B, h_B.data(), bytes, hipMemcpyHostToDevice));

  // Prefetch only the first 256 B of d_A; the kernel reads all 16384 B.
  hipExtDynDataPrefetchRegion region = {};
  region.address = d_A;
  region.width   = 256;
  region.height  = 1;
  region.stride  = 256;

  hipExtDynDataPrefetchConfig prefetchConfig = {};
  prefetchConfig.numRegions = 1;
  prefetchConfig.temporal   = hipExtDynDataPrefetchTemporalRegular;
  prefetchConfig.regions[0] = region;
  hipLaunchAttribute attr = makeDynDataPrefetchAttr(&prefetchConfig);

  hipLaunchConfig_t config = {};
  constexpr int blockSize = 256;
  config.gridDim  = dim3{(N + blockSize - 1) / blockSize, 1, 1};
  config.blockDim = dim3{blockSize, 1, 1};
  config.stream   = 0;
  config.attrs    = &attr;
  config.numAttrs = 1;

  int n = N;
  void* args[] = {&d_A, &d_B, &d_C, const_cast<float*>(&alpha), &n};
  HIP_CHECK(hipLaunchKernelExC(&config, reinterpret_cast<void*>(daxpyKernel), args));
  HIP_CHECK(hipDeviceSynchronize());
  HIP_CHECK(hipMemcpy(h_C.data(), d_C, bytes, hipMemcpyDeviceToHost));
  verifyDaxpy(h_A, h_B, h_C, alpha);

  HIP_CHECK(hipFree(d_A));
  HIP_CHECK(hipFree(d_B));
  HIP_CHECK(hipFree(d_C));
}

/**
 * End doxygen group KernelTest.
 * @}
 */
