/*
 * Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#include <hip_test_common.hh>
#include <hip_test_defgroups.hh>
/**
 * @addtogroup hipMemcpyBatchAsync hipMemcpyBatchAsync
 * @{
 * @ingroup MemoryTest
 * `hipError_t hipMemcpyBatchAsync(void** dsts, void** srcs, size_t* sizes,
 size_t count, hipMemcpyAttributes* attrs, size_t* attrsIdxs, size_t numAttrs,
                               size_t* failIdx, hipStream_t stream __dparm(0))`
 -
 * Perform Batch of 1D copies.
 */
/**
 * Test Description
 * ------------------------
 * - Test case to verify the 1D batch memory copy.
 * 1. Create Array of device pointers(Src, Dst).
 * 2. Set the MemcpyBatch params. As of now no support for memcpy Attributes.
 * 3. Perform batch memcpy operation from deviceptr to deviceptr.
 * 4. Validate data on host.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
#if HT_AMD
HIP_TEMPLATE_TEST_CASE(Unit_hipMemcpyBatchAsync_D2D_Functional, char, int,
                   float) {
  const size_t count = 2;
  size_t numAttrs = 0;
  const size_t arrSize = 4096;
  const size_t size = 4096 * sizeof(TestType);
  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));
  constexpr auto kfloatval1 = 2.25f;
  constexpr auto kfloatval2 = 0.25f;
  const TestType val1 = std::is_floating_point_v<TestType> ? kfloatval1
                        : std::is_integral_v<TestType>     ? 10
                                                           : 'a';
  const TestType val2 = std::is_floating_point_v<TestType> ? kfloatval2
                        : std::is_integral_v<TestType>     ? 4
                                                           : 'b';

  // Allocate buffers for pointer-ptr copy
  void *srcPtr[count], *dstPtr[count];
  std::vector<std::vector<TestType>> hostPtr1(
      count, std::vector<TestType>(arrSize, val1));
  std::vector<std::vector<TestType>> hostPtr2(
      count, std::vector<TestType>(arrSize, val2));
  size_t sizes[2];
  size_t attrsIdxs[1];
  for (int i = 0; i < count; i++) {
    HIP_CHECK(hipMalloc(&srcPtr[i], size));
    HIP_CHECK(hipMalloc(&dstPtr[i], size));
    HIP_CHECK(
        hipMemcpy(srcPtr[i], hostPtr2[i].data(), size, hipMemcpyHostToDevice));
    sizes[i] = size;
  }
  attrsIdxs[0] = 0;
  size_t failIdx;

  HIP_CHECK(hipMemcpyBatchAsync(dstPtr, srcPtr, sizes, count, nullptr,
                                attrsIdxs, numAttrs, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));
  // validation
  for (int i = 0; i < count; i++) {
    HIP_CHECK(
        hipMemcpy(hostPtr1[i].data(), dstPtr[i], size, hipMemcpyDeviceToHost));
    for (int j = 0; j < arrSize; j++) {
      INFO("Array FAILURE at Index: " << i << " " << j
                                      << "\nval : " << hostPtr1[i][j]);
      REQUIRE(hostPtr1[i][j] == val2);
    }
  }
  // Clean up
  for (int i = 0; i < count; i++) {
    HIP_CHECK(hipFree(srcPtr[i]));
    HIP_CHECK(hipFree(dstPtr[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Test case to verify the 1D batch memory copy.
 * 1. Create Array of device pointers(Src, Dst).
 * 2. Set the MemcpyBatch params. As of now no support for memcpy Attributes.
 * 3. Perform batch memcpy operation From Host to Device.
 * 4. Validate data on host.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEMPLATE_TEST_CASE(Unit_hipMemcpyBatchAsync_H2D_Functional, char, int,
                   float) {
  const size_t count = 2;
  size_t numAttrs = 0;
  const size_t arrSize = 4096;
  const size_t size = 4096 * sizeof(TestType);
  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  // Allocate buffers for pointer-ptr copy
  void *hostSrcPtr[count], *dstPtr[count];
  constexpr auto kfloatval1 = 2.25f;
  constexpr auto kfloatval2 = 0.25f;
  const TestType val1 = std::is_floating_point_v<TestType> ? kfloatval1
                        : std::is_integral_v<TestType>     ? 10
                                                           : 'a';
  const TestType val2 = std::is_floating_point_v<TestType> ? kfloatval2
                        : std::is_integral_v<TestType>     ? 4
                                                           : 'b';
  std::vector<std::vector<TestType>> hostPtr(
      count, std::vector<TestType>(arrSize, val2));
  std::array<TestType, arrSize> arr;
  arr.fill(val1);
  size_t sizes[2];
  size_t attrsIdxs[1];
  for (int i = 0; i < count; i++) {
    hostSrcPtr[i] = arr.data();
    HIP_CHECK(hipMalloc(&dstPtr[i], size));
    sizes[i] = size;
  }
  attrsIdxs[0] = 0;
  size_t failIdx;

  HIP_CHECK(hipMemcpyBatchAsync(dstPtr, hostSrcPtr, sizes, count, nullptr,
                                attrsIdxs, numAttrs, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));
  // validation
  for (int i = 0; i < count; i++) {
    HIP_CHECK(
        hipMemcpy(hostPtr[i].data(), dstPtr[i], size, hipMemcpyDeviceToHost));
    for (int j = 0; j < arrSize; j++) {
      INFO("Array FAILURE at Index: " << i << " " << j
                                      << "\nval : " << hostPtr[i][j]);
      REQUIRE(hostPtr[i][j] == val1);
    }
  }
  // Clean up
  for (int i = 0; i < count; i++) {
    HIP_CHECK(hipFree(dstPtr[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Test case to verify the 1D batch memory copy.
 * 1. Create Array of device pointers(Src, Dst).
 * 2. Set the MemcpyBatch params. As of now no support for memcpy Attributes.
 * 3. Perform batch memcpy operation From Device to Host.
 * 4. Validate data on host.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEMPLATE_TEST_CASE(Unit_hipMemcpyBatchAsync_D2H_Functional, char, int,
                   float) {
  const size_t count = 2;
  size_t numAttrs = 0;
  const size_t arrSize = 4096;
  const size_t size = 4096 * sizeof(TestType);
  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  constexpr auto kfloatval1 = 2.25f;
  constexpr auto kfloatval2 = 0.25f;
  const TestType val1 = std::is_floating_point_v<TestType> ? kfloatval1
                        : std::is_integral_v<TestType>     ? 10
                                                           : 'a';
  const TestType val2 = std::is_floating_point_v<TestType> ? kfloatval2
                        : std::is_integral_v<TestType>     ? 4
                                                           : 'b';
  // Allocate buffers for pointer-ptr copy
  TestType *hostDstPtr[count];
  void *deviceSrcPtr[count];
  std::vector<std::vector<TestType>> hostPtr(
      count, std::vector<TestType>(arrSize, val1));
  std::array<TestType, arrSize> arr;
  arr.fill(val2);
  size_t sizes[2];
  size_t attrsIdxs[1];
  for (int i = 0; i < count; i++) {
    hostDstPtr[i] = arr.data();
    HIP_CHECK(hipMalloc(&deviceSrcPtr[i], size));
    HIP_CHECK(hipMemcpy(deviceSrcPtr[i], hostPtr[i].data(), size,
                        hipMemcpyHostToDevice));
    sizes[i] = size;
  }
  attrsIdxs[0] = 0;
  size_t failIdx;

  HIP_CHECK(hipMemcpyBatchAsync(reinterpret_cast<void **>(hostDstPtr),
                                deviceSrcPtr, sizes, count, nullptr, attrsIdxs,
                                numAttrs, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));
  // validation
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < arrSize; j++) {
      INFO("Array FAILURE at Index: " << i << " " << j
                                      << "\nval : " << hostDstPtr[i][j]);
      REQUIRE(hostDstPtr[i][j] == val1);
    }
  }
  // Clean up
  for (int i = 0; i < count; i++) {
    HIP_CHECK(hipFree(deviceSrcPtr[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Test case to verify the 1D batch memory copy.
 * 1. Create Array of device pointers(Src, Dst).
 * 2. Set the MemcpyBatch params. As of now no support for memcpy Attributes.
 * 3. Perform batch memcpy operation From Host to Host.
 * 4. Validate data on host.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEMPLATE_TEST_CASE(Unit_hipMemcpyBatchAsync_H2H_Functional, char, int,
                   float) {
  const size_t count = 2;
  size_t numAttrs = 0;
  const size_t arrSize = 4096;
  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  constexpr auto kfloatval1 = 2.25;
  const TestType val1 = std::is_floating_point_v<TestType> ? kfloatval1
                        : std::is_integral_v<TestType>     ? 10
                                                           : 'a';
  constexpr auto kfloatval2 = 0.25f;
  const TestType val2 = std::is_floating_point_v<TestType> ? kfloatval2
                        : std::is_integral_v<TestType>     ? 4
                                                           : 'b';

  // Allocate buffers for pointer-ptr copy
  TestType *hostDstPtr[count], *hostSrcPtr[count];
  std::array<TestType, arrSize> arr1, arr2;
  arr1.fill(val1);
  arr2.fill(val2);
  size_t sizes[2];
  size_t attrsIdxs[1];
  for (int i = 0; i < count; i++) {
    hostDstPtr[i] = arr1.data();
    hostSrcPtr[i] = arr2.data();
    sizes[i] = arrSize * sizeof(TestType);
  }
  attrsIdxs[0] = 0;
  size_t failIdx;

  HIP_CHECK(hipMemcpyBatchAsync(reinterpret_cast<void **>(hostDstPtr),
                                reinterpret_cast<void **>(hostSrcPtr), sizes,
                                count, nullptr, attrsIdxs, numAttrs, &failIdx,
                                stream));
  HIP_CHECK(hipStreamSynchronize(stream));
  // validation
  for (int i = 0; i < count; i++) {
    for (int j = 0; j < arrSize; j++) {
      INFO("Array FAILURE at Index: " << i << " " << j
                                      << "\nval : " << hostDstPtr[i][j]);
      REQUIRE(hostDstPtr[i][j] == val2);
    }
  }
  // Clean up
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Verify hipMemcpyBatchAsync with hipMemcpyFlagExtOpSwap exchanges the
 *   contents of two device buffers.
 * 1. Allocate two device buffers and fill with distinct values.
 * 2. Issue hipMemcpyBatchAsync with swap attribute.
 * 3. Read back both buffers and verify values are exchanged.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_swap_cp) {
  constexpr size_t kNumElements = 4096;
  constexpr size_t kSizeBytes = kNumElements * sizeof(int);
  constexpr int kValA = 42;
  constexpr int kValB = 99;

  // Allocate device buffers
  void* d_a = nullptr;
  void* d_b = nullptr;
  HIP_CHECK(hipMalloc(&d_a, kSizeBytes));
  HIP_CHECK(hipMalloc(&d_b, kSizeBytes));

  // Fill with distinct values
  std::vector<int> hostA(kNumElements, kValA);
  std::vector<int> hostB(kNumElements, kValB);
  HIP_CHECK(hipMemcpy(d_a, hostA.data(), kSizeBytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_b, hostB.data(), kSizeBytes, hipMemcpyHostToDevice));

  // Set up batch swap: dst=d_a, src=d_b with swap flag
  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  void* dsts[] = {d_a};
  void* srcs[] = {d_b};
  size_t sizes[] = {kSizeBytes};
  size_t attrsIdxs[] = {0};

  hipMemcpyAttributes attr{};
  attr.flags = hipMemcpyFlagExtOpSwap;
  attr.srcAccessOrder = hipMemcpySrcAccessOrderStream;

  size_t failIdx = 0;
  hipError_t err = hipMemcpyBatchAsync(dsts, srcs, sizes, 1, &attr, attrsIdxs, 1,
                                       &failIdx, stream);
  if (err == hipErrorNotSupported) {
    HIP_CHECK(hipStreamDestroy(stream));
    HIP_CHECK(hipFree(d_a));
    HIP_CHECK(hipFree(d_b));
    HIP_SKIP_TEST(HipTest::SkipReason::kSdmaSwapUnsupported);
    return;
  }
  HIP_CHECK(err);
  HIP_CHECK(hipStreamSynchronize(stream));

  // Read back and verify swap
  std::vector<int> resultA(kNumElements);
  std::vector<int> resultB(kNumElements);
  HIP_CHECK(hipMemcpy(resultA.data(), d_a, kSizeBytes, hipMemcpyDeviceToHost));
  HIP_CHECK(hipMemcpy(resultB.data(), d_b, kSizeBytes, hipMemcpyDeviceToHost));

  for (size_t i = 0; i < kNumElements; i++) {
    REQUIRE(resultA[i] == kValB);
    REQUIRE(resultB[i] == kValA);
  }

  HIP_CHECK(hipFree(d_a));
  HIP_CHECK(hipFree(d_b));
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Verify broadcast copy works with varying numbers of destination buffers
 *   (2, 4, 8, 16) to exercise different fan-out widths.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_VaryingDstCount) {
  constexpr size_t kNumElements = 4096;
  constexpr size_t kSizeBytes = kNumElements * sizeof(int);
  constexpr int kSrcVal = 0xCD;
  constexpr size_t kMaxDsts = 16;

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  void* d_src = nullptr;
  HIP_CHECK(hipMalloc(&d_src, kSizeBytes));
  std::vector<int> hostSrc(kNumElements, kSrcVal);
  HIP_CHECK(hipMemcpy(d_src, hostSrc.data(), kSizeBytes, hipMemcpyHostToDevice));

  // Pre-allocate max destinations
  void* d_dsts[kMaxDsts];
  for (size_t i = 0; i < kMaxDsts; i++) {
    HIP_CHECK(hipMalloc(&d_dsts[i], kSizeBytes));
  }

  auto testBroadcast = [&](size_t numDsts) {
    // Clear destinations
    for (size_t i = 0; i < numDsts; i++) {
      HIP_CHECK(hipMemset(d_dsts[i], 0, kSizeBytes));
    }

    void* srcs[kMaxDsts];
    size_t sizes[kMaxDsts];
    for (size_t i = 0; i < numDsts; i++) {
      srcs[i] = d_src;
      sizes[i] = kSizeBytes;
    }

    hipMemcpyAttributes attr{};
    attr.flags = hipMemcpyFlagExtPreferCE;
    attr.srcAccessOrder = hipMemcpySrcAccessOrderStream;
    size_t attrsIdxs[] = {0};
    size_t failIdx = 0;

    HIP_CHECK(hipMemcpyBatchAsync(d_dsts, srcs, sizes, numDsts, &attr,
                                  attrsIdxs, 1, &failIdx, stream));
    HIP_CHECK(hipStreamSynchronize(stream));

    std::vector<int> hostResult(kNumElements);
    for (size_t d = 0; d < numDsts; d++) {
      HIP_CHECK(hipMemcpy(hostResult.data(), d_dsts[d], kSizeBytes,
                          hipMemcpyDeviceToHost));
      for (size_t j = 0; j < kNumElements; j++) {
        INFO("numDsts=" << numDsts << " dst=" << d << " index=" << j
                        << " expected=" << kSrcVal << " got=" << hostResult[j]);
        REQUIRE(hostResult[j] == kSrcVal);
      }
    }
  };

  SECTION("2 destinations") { testBroadcast(2); }
  SECTION("4 destinations") { testBroadcast(4); }
  SECTION("8 destinations") { testBroadcast(8); }
  SECTION("16 destinations") { testBroadcast(16); }

  // Clean up
  HIP_CHECK(hipFree(d_src));
  for (size_t i = 0; i < kMaxDsts; i++) {
    HIP_CHECK(hipFree(d_dsts[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Verify broadcast copy with a large buffer (16 MB) that requires multiple
 *   SDMA packets to complete.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_LargeBuffer) {
  constexpr size_t kNumDsts = 4;
  constexpr size_t kSizeBytes = 16 * 1024 * 1024;  // 16 MB
  constexpr size_t kNumElements = kSizeBytes / sizeof(int);

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  // Fill source with incrementing pattern for stronger validation
  void* d_src = nullptr;
  HIP_CHECK(hipMalloc(&d_src, kSizeBytes));
  std::vector<int> hostSrc(kNumElements);
  for (size_t i = 0; i < kNumElements; i++) {
    hostSrc[i] = static_cast<int>(i & 0x7FFFFFFF);
  }
  HIP_CHECK(hipMemcpy(d_src, hostSrc.data(), kSizeBytes, hipMemcpyHostToDevice));

  void* d_dsts[kNumDsts];
  for (size_t i = 0; i < kNumDsts; i++) {
    HIP_CHECK(hipMalloc(&d_dsts[i], kSizeBytes));
    HIP_CHECK(hipMemset(d_dsts[i], 0, kSizeBytes));
  }

  void* srcs[kNumDsts];
  size_t sizes[kNumDsts];
  for (size_t i = 0; i < kNumDsts; i++) {
    srcs[i] = d_src;
    sizes[i] = kSizeBytes;
  }

  hipMemcpyAttributes attr{};
  attr.flags = hipMemcpyFlagExtPreferCE;
  attr.srcAccessOrder = hipMemcpySrcAccessOrderStream;
  size_t attrsIdxs[] = {0};
  size_t failIdx = 0;

  HIP_CHECK(hipMemcpyBatchAsync(d_dsts, srcs, sizes, kNumDsts, &attr,
                                attrsIdxs, 1, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));

  // Validate all destinations
  std::vector<int> hostResult(kNumElements);
  for (size_t d = 0; d < kNumDsts; d++) {
    HIP_CHECK(hipMemcpy(hostResult.data(), d_dsts[d], kSizeBytes,
                        hipMemcpyDeviceToHost));
    // Check first, middle, and last elements for efficiency
    REQUIRE(hostResult[0] == hostSrc[0]);
    REQUIRE(hostResult[kNumElements / 2] == hostSrc[kNumElements / 2]);
    REQUIRE(hostResult[kNumElements - 1] == hostSrc[kNumElements - 1]);
    // Full validation
    for (size_t j = 0; j < kNumElements; j++) {
      if (hostResult[j] != hostSrc[j]) {
        INFO("Dst " << d << " mismatch at index " << j
                    << " expected=" << hostSrc[j] << " got=" << hostResult[j]);
        REQUIRE(hostResult[j] == hostSrc[j]);
      }
    }
  }

  HIP_CHECK(hipFree(d_src));
  for (size_t i = 0; i < kNumDsts; i++) {
    HIP_CHECK(hipFree(d_dsts[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Verify a mixed batch containing both broadcast entries (multiple copies
 *   from the same source) and independent linear copies. This tests the CLR
 *   grouping logic that separates broadcast ops from linear ops.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_MixedWithLinear) {
  constexpr size_t kNumElements = 4096;
  constexpr size_t kSizeBytes = kNumElements * sizeof(int);
  constexpr int kBcastVal = 0x11;
  constexpr int kLinearVal = 0x22;

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  // Broadcast source
  void* d_bcast_src = nullptr;
  HIP_CHECK(hipMalloc(&d_bcast_src, kSizeBytes));
  std::vector<int> hostBcast(kNumElements, kBcastVal);
  HIP_CHECK(hipMemcpy(d_bcast_src, hostBcast.data(), kSizeBytes,
                      hipMemcpyHostToDevice));

  // Broadcast destinations (3 copies from same source)
  constexpr size_t kNumBcastDsts = 3;
  void* d_bcast_dsts[kNumBcastDsts];
  for (size_t i = 0; i < kNumBcastDsts; i++) {
    HIP_CHECK(hipMalloc(&d_bcast_dsts[i], kSizeBytes));
    HIP_CHECK(hipMemset(d_bcast_dsts[i], 0, kSizeBytes));
  }

  // Independent linear copy (different source)
  void* d_linear_src = nullptr;
  void* d_linear_dst = nullptr;
  HIP_CHECK(hipMalloc(&d_linear_src, kSizeBytes));
  HIP_CHECK(hipMalloc(&d_linear_dst, kSizeBytes));
  std::vector<int> hostLinear(kNumElements, kLinearVal);
  HIP_CHECK(hipMemcpy(d_linear_src, hostLinear.data(), kSizeBytes,
                      hipMemcpyHostToDevice));
  HIP_CHECK(hipMemset(d_linear_dst, 0, kSizeBytes));

  // Build mixed batch: 3 broadcast + 1 linear = 4 total entries
  constexpr size_t kTotalCount = kNumBcastDsts + 1;
  void* srcs[kTotalCount];
  void* dsts[kTotalCount];
  size_t sizes[kTotalCount];

  // Broadcast entries (same source)
  for (size_t i = 0; i < kNumBcastDsts; i++) {
    srcs[i] = d_bcast_src;
    dsts[i] = d_bcast_dsts[i];
    sizes[i] = kSizeBytes;
  }
  // Linear entry (different source)
  srcs[kNumBcastDsts] = d_linear_src;
  dsts[kNumBcastDsts] = d_linear_dst;
  sizes[kNumBcastDsts] = kSizeBytes;

  hipMemcpyAttributes attr{};
  attr.flags = hipMemcpyFlagExtPreferCE;
  attr.srcAccessOrder = hipMemcpySrcAccessOrderStream;
  size_t attrsIdxs[] = {0};
  size_t failIdx = 0;

  HIP_CHECK(hipMemcpyBatchAsync(dsts, srcs, sizes, kTotalCount, &attr,
                                attrsIdxs, 1, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));

  // Validate broadcast destinations
  std::vector<int> hostResult(kNumElements);
  for (size_t d = 0; d < kNumBcastDsts; d++) {
    HIP_CHECK(hipMemcpy(hostResult.data(), d_bcast_dsts[d], kSizeBytes,
                        hipMemcpyDeviceToHost));
    for (size_t j = 0; j < kNumElements; j++) {
      INFO("Broadcast dst " << d << " index " << j
                            << " expected=" << kBcastVal
                            << " got=" << hostResult[j]);
      REQUIRE(hostResult[j] == kBcastVal);
    }
  }

  // Validate linear destination
  HIP_CHECK(hipMemcpy(hostResult.data(), d_linear_dst, kSizeBytes,
                      hipMemcpyDeviceToHost));
  for (size_t j = 0; j < kNumElements; j++) {
    INFO("Linear dst index " << j << " expected=" << kLinearVal
                             << " got=" << hostResult[j]);
    REQUIRE(hostResult[j] == kLinearVal);
  }

  // Clean up
  HIP_CHECK(hipFree(d_bcast_src));
  for (size_t i = 0; i < kNumBcastDsts; i++) {
    HIP_CHECK(hipFree(d_bcast_dsts[i]));
  }
  HIP_CHECK(hipFree(d_linear_src));
  HIP_CHECK(hipFree(d_linear_dst));
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Template test verifying broadcast copy with typed data (char, int, float)
 *   for data integrity across different element types.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEMPLATE_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_DataIntegrity, char,
                       int, float) {
  constexpr size_t kNumDsts = 4;
  constexpr size_t kArrSize = 4096;
  constexpr size_t kSizeBytes = kArrSize * sizeof(TestType);

  const TestType srcVal = std::is_same_v<TestType, float> ? static_cast<TestType>(3.14f)
                        : std::is_same_v<TestType, char>  ? static_cast<TestType>('Z')
                                                          : static_cast<TestType>(77);

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  // Allocate and fill source
  void* d_src = nullptr;
  HIP_CHECK(hipMalloc(&d_src, kSizeBytes));
  std::vector<TestType> hostSrc(kArrSize, srcVal);
  HIP_CHECK(hipMemcpy(d_src, hostSrc.data(), kSizeBytes, hipMemcpyHostToDevice));

  // Allocate destinations
  void* d_dsts[kNumDsts];
  for (size_t i = 0; i < kNumDsts; i++) {
    HIP_CHECK(hipMalloc(&d_dsts[i], kSizeBytes));
    HIP_CHECK(hipMemset(d_dsts[i], 0, kSizeBytes));
  }

  // Broadcast: all srcs point to same buffer
  void* srcs[kNumDsts];
  size_t sizes[kNumDsts];
  for (size_t i = 0; i < kNumDsts; i++) {
    srcs[i] = d_src;
    sizes[i] = kSizeBytes;
  }

  hipMemcpyAttributes attr{};
  attr.flags = hipMemcpyFlagExtPreferCE;
  attr.srcAccessOrder = hipMemcpySrcAccessOrderStream;
  size_t attrsIdxs[] = {0};
  size_t failIdx = 0;

  HIP_CHECK(hipMemcpyBatchAsync(d_dsts, srcs, sizes, kNumDsts, &attr,
                                attrsIdxs, 1, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));

  // Validate each destination element-by-element
  std::vector<TestType> hostResult(kArrSize);
  for (size_t d = 0; d < kNumDsts; d++) {
    HIP_CHECK(hipMemcpy(hostResult.data(), d_dsts[d], kSizeBytes,
                        hipMemcpyDeviceToHost));
    for (size_t j = 0; j < kArrSize; j++) {
      INFO("Dst " << d << " index " << j << " expected=" << srcVal
                  << " got=" << hostResult[j]);
      REQUIRE(hostResult[j] == srcVal);
    }
  }

  // Clean up
  HIP_CHECK(hipFree(d_src));
  for (size_t i = 0; i < kNumDsts; i++) {
    HIP_CHECK(hipFree(d_dsts[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Verify broadcast copy without hipMemcpyFlagExtPreferCE. Copies go through
 *   the shader copy path (no SDMA multicast), validating correctness of the
 *   default path for the broadcast pattern.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_NoPreferCE) {
  constexpr size_t kNumDsts = 4;
  constexpr size_t kNumElements = 4096;
  constexpr size_t kSizeBytes = kNumElements * sizeof(int);
  constexpr int kSrcVal = 0xBB;

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  void* d_src = nullptr;
  HIP_CHECK(hipMalloc(&d_src, kSizeBytes));
  std::vector<int> hostSrc(kNumElements, kSrcVal);
  HIP_CHECK(hipMemcpy(d_src, hostSrc.data(), kSizeBytes, hipMemcpyHostToDevice));

  void* d_dsts[kNumDsts];
  for (size_t i = 0; i < kNumDsts; i++) {
    HIP_CHECK(hipMalloc(&d_dsts[i], kSizeBytes));
    HIP_CHECK(hipMemset(d_dsts[i], 0, kSizeBytes));
  }

  // No attributes — copies go through shader path, not SDMA
  void* srcs[kNumDsts];
  size_t sizes[kNumDsts];
  for (size_t i = 0; i < kNumDsts; i++) {
    srcs[i] = d_src;
    sizes[i] = kSizeBytes;
  }
  size_t attrsIdxs[] = {0};
  size_t failIdx = 0;

  HIP_CHECK(hipMemcpyBatchAsync(d_dsts, srcs, sizes, kNumDsts, nullptr,
                                attrsIdxs, 0, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));

  std::vector<int> hostResult(kNumElements);
  for (size_t d = 0; d < kNumDsts; d++) {
    HIP_CHECK(hipMemcpy(hostResult.data(), d_dsts[d], kSizeBytes,
                        hipMemcpyDeviceToHost));
    for (size_t j = 0; j < kNumElements; j++) {
      INFO("Dst " << d << " index " << j
                  << " expected=" << kSrcVal << " got=" << hostResult[j]);
      REQUIRE(hostResult[j] == kSrcVal);
    }
  }

  HIP_CHECK(hipFree(d_src));
  for (size_t i = 0; i < kNumDsts; i++) {
    HIP_CHECK(hipFree(d_dsts[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Verify multiple broadcast groups in a single batch: two distinct sources,
 *   each broadcast to 3 destinations. Tests that the CLR grouping logic
 *   creates separate BROADCAST ops per (src, size) key.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_MultipleGroups) {
  constexpr size_t kNumElements = 4096;
  constexpr size_t kSizeBytes = kNumElements * sizeof(int);
  constexpr int kValA = 0x11;
  constexpr int kValB = 0x22;
  constexpr size_t kDstsPerSrc = 3;

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  // Two distinct sources
  void* d_srcA = nullptr;
  void* d_srcB = nullptr;
  HIP_CHECK(hipMalloc(&d_srcA, kSizeBytes));
  HIP_CHECK(hipMalloc(&d_srcB, kSizeBytes));
  std::vector<int> hostA(kNumElements, kValA);
  std::vector<int> hostB(kNumElements, kValB);
  HIP_CHECK(hipMemcpy(d_srcA, hostA.data(), kSizeBytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_srcB, hostB.data(), kSizeBytes, hipMemcpyHostToDevice));

  // 3 destinations per source = 6 total
  constexpr size_t kTotal = kDstsPerSrc * 2;
  void* d_dsts[kTotal];
  for (size_t i = 0; i < kTotal; i++) {
    HIP_CHECK(hipMalloc(&d_dsts[i], kSizeBytes));
    HIP_CHECK(hipMemset(d_dsts[i], 0, kSizeBytes));
  }

  // Interleave sources: A, B, A, B, A, B
  void* srcs[kTotal];
  size_t sizes[kTotal];
  for (size_t i = 0; i < kTotal; i++) {
    srcs[i] = (i % 2 == 0) ? d_srcA : d_srcB;
    sizes[i] = kSizeBytes;
  }

  hipMemcpyAttributes attr{};
  attr.flags = hipMemcpyFlagExtPreferCE;
  attr.srcAccessOrder = hipMemcpySrcAccessOrderStream;
  size_t attrsIdxs[] = {0};
  size_t failIdx = 0;

  HIP_CHECK(hipMemcpyBatchAsync(d_dsts, srcs, sizes, kTotal, &attr,
                                attrsIdxs, 1, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));

  std::vector<int> hostResult(kNumElements);
  for (size_t i = 0; i < kTotal; i++) {
    HIP_CHECK(hipMemcpy(hostResult.data(), d_dsts[i], kSizeBytes,
                        hipMemcpyDeviceToHost));
    int expected = (i % 2 == 0) ? kValA : kValB;
    for (size_t j = 0; j < kNumElements; j++) {
      INFO("Dst " << i << " index " << j
                  << " expected=" << expected << " got=" << hostResult[j]);
      REQUIRE(hostResult[j] == expected);
    }
  }

  HIP_CHECK(hipFree(d_srcA));
  HIP_CHECK(hipFree(d_srcB));
  for (size_t i = 0; i < kTotal; i++) {
    HIP_CHECK(hipFree(d_dsts[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Verify a batch containing both broadcast and swap operations.
 *   3 broadcast entries (same src) + 1 swap entry in a single batch call.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_WithSwap) {
  constexpr size_t kNumElements = 4096;
  constexpr size_t kSizeBytes = kNumElements * sizeof(int);
  constexpr int kBcastVal = 0x33;
  constexpr int kSwapValA = 42;
  constexpr int kSwapValB = 99;
  constexpr size_t kNumBcastDsts = 3;

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  // Broadcast source and destinations
  void* d_bcast_src = nullptr;
  HIP_CHECK(hipMalloc(&d_bcast_src, kSizeBytes));
  std::vector<int> hostBcast(kNumElements, kBcastVal);
  HIP_CHECK(hipMemcpy(d_bcast_src, hostBcast.data(), kSizeBytes, hipMemcpyHostToDevice));

  void* d_bcast_dsts[kNumBcastDsts];
  for (size_t i = 0; i < kNumBcastDsts; i++) {
    HIP_CHECK(hipMalloc(&d_bcast_dsts[i], kSizeBytes));
    HIP_CHECK(hipMemset(d_bcast_dsts[i], 0, kSizeBytes));
  }

  // Swap buffers
  void* d_swapA = nullptr;
  void* d_swapB = nullptr;
  HIP_CHECK(hipMalloc(&d_swapA, kSizeBytes));
  HIP_CHECK(hipMalloc(&d_swapB, kSizeBytes));
  std::vector<int> hostSwapA(kNumElements, kSwapValA);
  std::vector<int> hostSwapB(kNumElements, kSwapValB);
  HIP_CHECK(hipMemcpy(d_swapA, hostSwapA.data(), kSizeBytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_swapB, hostSwapB.data(), kSizeBytes, hipMemcpyHostToDevice));

  // Batch: 3 broadcast + 1 swap = 4 entries
  constexpr size_t kTotal = kNumBcastDsts + 1;
  void* srcs[kTotal];
  void* dsts[kTotal];
  size_t sizes[kTotal];

  for (size_t i = 0; i < kNumBcastDsts; i++) {
    srcs[i] = d_bcast_src;
    dsts[i] = d_bcast_dsts[i];
    sizes[i] = kSizeBytes;
  }
  srcs[kNumBcastDsts] = d_swapB;
  dsts[kNumBcastDsts] = d_swapA;
  sizes[kNumBcastDsts] = kSizeBytes;

  // Two attribute groups: broadcast entries use preferCE, swap entry uses swap flag
  hipMemcpyAttributes attrs[2] = {};
  attrs[0].flags = hipMemcpyFlagExtPreferCE;
  attrs[0].srcAccessOrder = hipMemcpySrcAccessOrderStream;
  attrs[1].flags = hipMemcpyFlagExtOpSwap;
  attrs[1].srcAccessOrder = hipMemcpySrcAccessOrderStream;
  size_t attrsIdxs[] = {0, kNumBcastDsts};
  size_t failIdx = 0;

  hipError_t err = hipMemcpyBatchAsync(dsts, srcs, sizes, kTotal, attrs,
                                       attrsIdxs, 2, &failIdx, stream);
  if (err == hipErrorNotSupported) {
    // Swap not supported on this GPU
    HIP_CHECK(hipStreamDestroy(stream));
    HIP_CHECK(hipFree(d_bcast_src));
    for (size_t i = 0; i < kNumBcastDsts; i++) HIP_CHECK(hipFree(d_bcast_dsts[i]));
    HIP_CHECK(hipFree(d_swapA));
    HIP_CHECK(hipFree(d_swapB));
    HIP_SKIP_TEST(HipTest::SkipReason::kSdmaSwapUnsupported);
    return;
  }
  HIP_CHECK(err);
  HIP_CHECK(hipStreamSynchronize(stream));

  // Validate broadcast destinations
  std::vector<int> hostResult(kNumElements);
  for (size_t d = 0; d < kNumBcastDsts; d++) {
    HIP_CHECK(hipMemcpy(hostResult.data(), d_bcast_dsts[d], kSizeBytes,
                        hipMemcpyDeviceToHost));
    for (size_t j = 0; j < kNumElements; j++) {
      INFO("Broadcast dst " << d << " index " << j
                            << " expected=" << kBcastVal << " got=" << hostResult[j]);
      REQUIRE(hostResult[j] == kBcastVal);
    }
  }

  // Validate swap
  std::vector<int> resultA(kNumElements), resultB(kNumElements);
  HIP_CHECK(hipMemcpy(resultA.data(), d_swapA, kSizeBytes, hipMemcpyDeviceToHost));
  HIP_CHECK(hipMemcpy(resultB.data(), d_swapB, kSizeBytes, hipMemcpyDeviceToHost));
  for (size_t j = 0; j < kNumElements; j++) {
    REQUIRE(resultA[j] == kSwapValB);
    REQUIRE(resultB[j] == kSwapValA);
  }

  HIP_CHECK(hipFree(d_bcast_src));
  for (size_t i = 0; i < kNumBcastDsts; i++) HIP_CHECK(hipFree(d_bcast_dsts[i]));
  HIP_CHECK(hipFree(d_swapA));
  HIP_CHECK(hipFree(d_swapB));
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Verify that a single-destination "broadcast" (1 src → 1 dst) degrades
 *   gracefully to a regular linear copy without errors.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_SingleDst) {
  constexpr size_t kNumElements = 4096;
  constexpr size_t kSizeBytes = kNumElements * sizeof(int);
  constexpr int kSrcVal = 0xDD;

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  void* d_src = nullptr;
  void* d_dst = nullptr;
  HIP_CHECK(hipMalloc(&d_src, kSizeBytes));
  HIP_CHECK(hipMalloc(&d_dst, kSizeBytes));
  std::vector<int> hostSrc(kNumElements, kSrcVal);
  HIP_CHECK(hipMemcpy(d_src, hostSrc.data(), kSizeBytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemset(d_dst, 0, kSizeBytes));

  void* srcs[] = {d_src};
  void* dsts[] = {d_dst};
  size_t sizes[] = {kSizeBytes};

  hipMemcpyAttributes attr{};
  attr.flags = hipMemcpyFlagExtPreferCE;
  attr.srcAccessOrder = hipMemcpySrcAccessOrderStream;
  size_t attrsIdxs[] = {0};
  size_t failIdx = 0;

  HIP_CHECK(hipMemcpyBatchAsync(dsts, srcs, sizes, 1, &attr,
                                attrsIdxs, 1, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));

  std::vector<int> hostResult(kNumElements);
  HIP_CHECK(hipMemcpy(hostResult.data(), d_dst, kSizeBytes, hipMemcpyDeviceToHost));
  for (size_t j = 0; j < kNumElements; j++) {
    INFO("Index " << j << " expected=" << kSrcVal << " got=" << hostResult[j]);
    REQUIRE(hostResult[j] == kSrcVal);
  }

  HIP_CHECK(hipFree(d_src));
  HIP_CHECK(hipFree(d_dst));
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Verify that entries sharing the same source pointer but with different
 *   sizes are NOT grouped as broadcast (grouping key is (src, size)).
 *   Each entry should be copied independently with its own size.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_DifferentSizes) {
  constexpr size_t kNumEntries = 3;
  constexpr int kSrcVal = 0xEE;
  // Source is large enough for the biggest copy
  constexpr size_t kMaxElements = 8192;
  constexpr size_t kMaxBytes = kMaxElements * sizeof(int);
  // Each entry copies a different number of elements
  const size_t entrySizes[] = {1024 * sizeof(int), 2048 * sizeof(int), 4096 * sizeof(int)};
  const size_t entryElements[] = {1024, 2048, 4096};

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  void* d_src = nullptr;
  HIP_CHECK(hipMalloc(&d_src, kMaxBytes));
  std::vector<int> hostSrc(kMaxElements, kSrcVal);
  HIP_CHECK(hipMemcpy(d_src, hostSrc.data(), kMaxBytes, hipMemcpyHostToDevice));

  void* d_dsts[kNumEntries];
  for (size_t i = 0; i < kNumEntries; i++) {
    HIP_CHECK(hipMalloc(&d_dsts[i], kMaxBytes));
    HIP_CHECK(hipMemset(d_dsts[i], 0, kMaxBytes));
  }

  // Same source, different sizes — should NOT group as broadcast
  void* srcs[kNumEntries];
  size_t sizes[kNumEntries];
  for (size_t i = 0; i < kNumEntries; i++) {
    srcs[i] = d_src;
    sizes[i] = entrySizes[i];
  }

  hipMemcpyAttributes attr{};
  attr.flags = hipMemcpyFlagExtPreferCE;
  attr.srcAccessOrder = hipMemcpySrcAccessOrderStream;
  size_t attrsIdxs[] = {0};
  size_t failIdx = 0;

  HIP_CHECK(hipMemcpyBatchAsync(d_dsts, srcs, sizes, kNumEntries, &attr,
                                attrsIdxs, 1, &failIdx, stream));
  HIP_CHECK(hipStreamSynchronize(stream));

  // Validate: each destination should have kSrcVal for its copy size,
  // and zero beyond that
  std::vector<int> hostResult(kMaxElements);
  for (size_t i = 0; i < kNumEntries; i++) {
    HIP_CHECK(hipMemcpy(hostResult.data(), d_dsts[i], kMaxBytes,
                        hipMemcpyDeviceToHost));
    // Copied region should match source
    for (size_t j = 0; j < entryElements[i]; j++) {
      INFO("Entry " << i << " index " << j
                    << " expected=" << kSrcVal << " got=" << hostResult[j]);
      REQUIRE(hostResult[j] == kSrcVal);
    }
    // Beyond copied region should still be zero
    if (entryElements[i] < kMaxElements) {
      INFO("Entry " << i << " beyond-copy index " << entryElements[i]
                    << " expected=0 got=" << hostResult[entryElements[i]]);
      REQUIRE(hostResult[entryElements[i]] == 0);
    }
  }

  HIP_CHECK(hipFree(d_src));
  for (size_t i = 0; i < kNumEntries; i++) {
    HIP_CHECK(hipFree(d_dsts[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}

/**
 * Test Description
 * ------------------------
 * - Negative test: verify that broadcast batch with a zero-size entry
 *   returns hipErrorInvalidValue.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Broadcast_ZeroSize) {
  constexpr size_t kSizeBytes = 4096 * sizeof(int);

  hipStream_t stream;
  HIP_CHECK(hipStreamCreate(&stream));

  void* d_src = nullptr;
  void* d_dst1 = nullptr;
  void* d_dst2 = nullptr;
  HIP_CHECK(hipMalloc(&d_src, kSizeBytes));
  HIP_CHECK(hipMalloc(&d_dst1, kSizeBytes));
  HIP_CHECK(hipMalloc(&d_dst2, kSizeBytes));

  void* srcs[] = {d_src, d_src};
  void* dsts[] = {d_dst1, d_dst2};
  size_t sizes[] = {kSizeBytes, 0};  // second entry has zero size

  size_t attrsIdxs[] = {0};
  size_t failIdx = 0;

  HIP_CHECK_ERROR(hipMemcpyBatchAsync(dsts, srcs, sizes, 2, nullptr,
                                      attrsIdxs, 0, &failIdx, stream),
                  hipErrorInvalidValue);

  HIP_CHECK(hipFree(d_src));
  HIP_CHECK(hipFree(d_dst1));
  HIP_CHECK(hipFree(d_dst2));
  HIP_CHECK(hipStreamDestroy(stream));
}
#endif
/**
 * Test Description
 * ------------------------
 * - Test case to verify the negative cases of hipMemcpyBatchAsync.
 * 1. Dst Array as nullptr.
 * 2. Src Array as nullptr.
 * 3. Operations Count as 0.
 * 4. Num of attributes as 0.
 * 5. Sizes Array as nullptr.
 * 6. Attr Array as nullptr.
 * 7. AttrsIdxs Array as nullptr.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_NegativeTsts) {
  const size_t count = 2;
  size_t numAttrs = 0;
  size_t sizes[2];
  size_t attrsIdxs[1];
  const size_t size = 4096 * sizeof(char);
  hipStream_t stream = NULL;
  HIP_CHECK(hipStreamCreate(&stream));
  void *srcPtr[count], *dstPtr[count];
  for (int i = 0; i < count; i++) {
    HIP_CHECK(hipMalloc(&srcPtr[i], size));
    HIP_CHECK(hipMalloc(&dstPtr[i], size));
    sizes[i] = size;
  }

  attrsIdxs[0] = 0;
  size_t failIdx;
  SECTION("Dst Array as nullptr") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(nullptr, srcPtr, sizes, count, nullptr,
                                        attrsIdxs, numAttrs, &failIdx, stream),
                    hipErrorInvalidValue);
  }
  SECTION("Src Array as nullptr") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dstPtr, nullptr, sizes, count, nullptr,
                                        attrsIdxs, numAttrs, &failIdx, stream),
                    hipErrorInvalidValue);
  }
  SECTION("Count as zero") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dstPtr, srcPtr, sizes, 0, nullptr,
                                        attrsIdxs, numAttrs, &failIdx, stream),
                    hipErrorInvalidValue);
  }
  SECTION("sizes Array as nullptr") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dstPtr, srcPtr, nullptr, count, nullptr,
                                        attrsIdxs, numAttrs, &failIdx, stream),
                    hipErrorInvalidValue);
  }
#if 0 // Enable these tests when support for memcpy attributes is enabled.
  SECTION("Number of Attributes as zero") {
    HIP_CHECK_ERROR(
        hipMemcpyBatchAsync(dstPtr, srcPtr, sizes, count, attr, attrsIdxs, 0, &failIdx, stream),
        hipErrorInvalidValue);
  }
  SECTION("Attr Array as nullptr") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dstPtr, srcPtr, sizes, count, nullptr, attrsIdxs, numAttrs,
                                        &failIdx, stream),
                    hipErrorInvalidValue);
  }

  SECTION("attrsIdxs Array as nullptr") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dstPtr, srcPtr, sizes, count, attr, nullptr, numAttrs,
                                        &failIdx, stream),
                    hipErrorInvalidValue);
  }
#endif
  // Clean up
  for (int i = 0; i < count; i++) {
    HIP_CHECK(hipFree(srcPtr[i]));
    HIP_CHECK(hipFree(dstPtr[i]));
  }
  HIP_CHECK(hipStreamDestroy(stream));
}
/**
 * End doxygen group MemoryTest.
 * @}
 */
