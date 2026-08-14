/*
 * Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

#include <hip_test_common.hh>
#include <hip_test_defgroups.hh>
#include <resource_guards.hh>
#include <utils.hh>

/**
 * @addtogroup hipMemcpyBatchAsync hipMemcpyBatchAsync
 * @{
 * @ingroup MemoryTest
 * `hipError_t hipMemcpyBatchAsync(void** dsts, void** srcs, size_t* sizes,
 * size_t count, hipMemcpyAttributes* attrs, size_t* attrsIdxs, size_t numAttrs,
 * size_t* failIdx, hipStream_t stream __dparm(0))`
 *
 * Perform a batch of 1D copies.
 */

namespace {

constexpr size_t kOneKiB = 1024;
constexpr size_t kSmallCopySize = 4 * kOneKiB;
constexpr size_t kMediumCopySize = 32 * kOneKiB;
constexpr size_t kLargeCopySize = 512 * kOneKiB;
constexpr int kPatternValue = 0x42;

struct BatchConfig {
  size_t copy_count;
  size_t copy_size;
};

enum class PointerPattern {
  kBasePointers,
  kOffsetPointers,
  kUnalignedPointers,
  kBroadcastSource,
};

std::vector<std::pair<int, int>> GetPeerAccessibleDevicePairs() {
  if (HipTest::getDeviceCount() < 2) {
    HIP_SKIP_TEST(HipTest::SkipReason::kFewerThanTwoGpus);
  }

  const int device_count = HipTest::getDeviceCount();
  std::vector<std::pair<int, int>> peer_pairs;
  for (int src_device = 0; src_device < device_count; ++src_device) {
    for (int dst_device = 0; dst_device < device_count; ++dst_device) {
      if (src_device == dst_device) {
        continue;
      }
      int can_access_peer = 0;
      HIP_CHECK(hipDeviceCanAccessPeer(&can_access_peer, src_device, dst_device));
      if (can_access_peer != 0) {
        peer_pairs.emplace_back(src_device, dst_device);
      }
    }
  }

  if (peer_pairs.empty()) {
    HIP_SKIP_TEST(HipTest::SkipReason::kPeerAccessUnavailable);
  }
  return peer_pairs;
}

void EnablePeerAccess(const std::vector<std::pair<int, int>>& peer_pairs) {
  for (const auto& [src_device, dst_device] : peer_pairs) {
    HIP_CHECK(hipSetDevice(src_device));
    hipError_t peer_status = hipDeviceEnablePeerAccess(dst_device, 0);
    if (peer_status != hipSuccess && peer_status != hipErrorPeerAccessAlreadyEnabled) {
      HIP_CHECK(peer_status);
    }
  }
}

void DisablePeerAccess(const std::vector<std::pair<int, int>>& peer_pairs) {
  for (const auto& [src_device, dst_device] : peer_pairs) {
    HIP_CHECK(hipSetDevice(src_device));
    HIP_CHECK(hipDeviceDisablePeerAccess(dst_device));
  }
}

std::vector<LinearAllocGuard<int>> AllocateBatchBuffers(LinearAllocs allocation_type,
                                                        const BatchConfig& config,
                                                        size_t extra_bytes = 0) {
  std::vector<LinearAllocGuard<int>> allocations;

  for (size_t i = 0; i < config.copy_count; ++i) {
    allocations.emplace_back(allocation_type, config.copy_size + extra_bytes);
  }

  return allocations;
}

std::vector<void*> MakeBatchPtrs(std::vector<LinearAllocGuard<int>>& allocations,
                                 size_t offset_bytes = 0) {
  std::vector<void*> ptrs;

  for (LinearAllocGuard<int>& allocation : allocations) {
    ptrs.push_back(static_cast<void*>(reinterpret_cast<char*>(allocation.ptr()) + offset_bytes));
  }

  return ptrs;
}

void FillDeviceBuffers(const std::vector<void*>& ptrs, size_t copy_size, int value) {
  const size_t copy_elements = copy_size / sizeof(int);
  std::vector<int> source(copy_elements);

  for (size_t i = 0; i < ptrs.size(); ++i) {
    std::fill(source.begin(), source.end(), value + static_cast<int>(i));
    HIP_CHECK(hipMemcpy(ptrs[i], source.data(), copy_size, hipMemcpyHostToDevice));
  }
}

void FillHostBuffers(std::vector<LinearAllocGuard<int>>& buffers, size_t copy_size,
                     int value = kPatternValue) {
  const size_t copy_elements = copy_size / sizeof(int);

  for (size_t i = 0; i < buffers.size(); ++i) {
    std::fill_n(buffers[i].host_ptr(), copy_elements, value + static_cast<int>(i));
  }
}

void VerifyArrayFromBothEnds(const int* values, size_t copy_elements, int expected,
                             size_t copy_index) {
  for (size_t offset = 0; offset < (copy_elements + 1) / 2; ++offset) {
    const size_t front_index = offset;
    const size_t back_index = copy_elements - 1 - offset;

    INFO("Array failure at copy index " << copy_index << ", element " << front_index);
    REQUIRE(values[front_index] == expected);

    if (front_index == back_index) {
      continue;
    }

    INFO("Array failure at copy index " << copy_index << ", element " << back_index);
    REQUIRE(values[back_index] == expected);
  }
}

void VerifyDeviceBuffers(const std::vector<void*>& ptrs, size_t copy_size,
                         int expected = kPatternValue, bool add_index = true) {
  const size_t copy_elements = copy_size / sizeof(int);
  std::vector<int> result(copy_elements);

  for (size_t i = 0; i < ptrs.size(); ++i) {
    HIP_CHECK(hipMemcpy(result.data(), ptrs[i], copy_size, hipMemcpyDeviceToHost));
    const int value = expected + (add_index ? static_cast<int>(i) : 0);
    VerifyArrayFromBothEnds(result.data(), copy_elements, value, i);
  }
}

void VerifyHostBuffers(std::vector<LinearAllocGuard<int>>& buffers, size_t copy_size,
                       int expected = kPatternValue) {
  const size_t copy_elements = copy_size / sizeof(int);

  for (size_t i = 0; i < buffers.size(); ++i) {
    VerifyArrayFromBothEnds(buffers[i].host_ptr(), copy_elements, expected + static_cast<int>(i),
                            i);
  }
}

}  // namespace

/**
 * Test Description
 * ------------------------
 * - Verifies API-level negative validation for hipMemcpyBatchAsync.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Negative) {
  constexpr size_t kCount = 3;

  StreamGuard stream_guard(Streams::created);
  std::array<LinearAllocGuard<int>, kCount> src_allocs;
  std::array<LinearAllocGuard<int>, kCount> dst_allocs;
  std::array<void*, kCount> src_ptrs{};
  std::array<void*, kCount> dst_ptrs{};
  std::array<size_t, kCount> sizes{};

  for (size_t i = 0; i < kCount; ++i) {
    src_allocs[i] = LinearAllocGuard<int>(LinearAllocs::hipMalloc, kSmallCopySize);
    dst_allocs[i] = LinearAllocGuard<int>(LinearAllocs::hipMalloc, kSmallCopySize);
    src_ptrs[i] = src_allocs[i].ptr();
    dst_ptrs[i] = dst_allocs[i].ptr();
    sizes[i] = kSmallCopySize;
  }

  size_t attrs_idxs[1] = {0};

  SECTION("Null destination array") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(nullptr, src_ptrs.data(), sizes.data(), kCount, nullptr,
                                        attrs_idxs, 0, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }

  SECTION("Null source array") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), nullptr, sizes.data(), kCount, nullptr,
                                        attrs_idxs, 0, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }

  SECTION("Null sizes array") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), nullptr, kCount, nullptr,
                                        attrs_idxs, 0, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }

  SECTION("Zero count") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), 0, nullptr,
                                        attrs_idxs, 0, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }

  SECTION("Null destination element") {
    dst_ptrs[1] = nullptr;
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount,
                                        nullptr, attrs_idxs, 0, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }

  SECTION("Null source element") {
    src_ptrs[1] = nullptr;
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount,
                                        nullptr, attrs_idxs, 0, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }

  SECTION("Zero size first copy") {
    sizes[0] = 0;
    size_t fail_idx = 0;
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount,
                                        nullptr, attrs_idxs, 0, &fail_idx, stream_guard.stream()),
                    hipErrorInvalidValue);
    REQUIRE(fail_idx == 0);
  }

  SECTION("Zero size middle copy") {
    sizes[1] = 0;
    size_t fail_idx = 0;
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount,
                                        nullptr, attrs_idxs, 0, &fail_idx, stream_guard.stream()),
                    hipErrorInvalidValue);
    REQUIRE(fail_idx == 1);
  }

  SECTION("Zero size last copy") {
    sizes[2] = 0;
    size_t fail_idx = 0;
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount,
                                        nullptr, attrs_idxs, 0, &fail_idx, stream_guard.stream()),
                    hipErrorInvalidValue);
    REQUIRE(fail_idx == 2);
  }

  SECTION("Null fail index on zero size") {
    sizes[1] = 0;
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount,
                                        nullptr, attrs_idxs, 0, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }

  SECTION("Source range exceeds allocation") {
    src_ptrs[1] = static_cast<void*>(src_allocs[1].ptr() + 1);
    sizes[1] = kSmallCopySize;
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount,
                                        nullptr, attrs_idxs, 0, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }

  SECTION("Destination range exceeds allocation") {
    dst_ptrs[1] = static_cast<void*>(dst_allocs[1].ptr() + 1);
    sizes[1] = kSmallCopySize;
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount,
                                        nullptr, attrs_idxs, 0, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }
}

/**
 * Test Description
 * ------------------------
 * - Verifies attribute array validation for hipMemcpyBatchAsync.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Attrs_Negative) {
  constexpr size_t kCount = 2;
  BatchConfig config{kCount, kSmallCopySize};
  StreamGuard stream_guard(Streams::created);
  std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
  std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
  std::vector<void*> src_ptrs = MakeBatchPtrs(src);
  std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);
  std::vector<size_t> sizes(config.copy_count, config.copy_size);
  std::array<hipMemcpyAttributes, 3> attrs{
      hipMemcpyAttributes{hipMemcpySrcAccessOrderStream, {}, {}, hipMemcpyFlagDefault},
      hipMemcpyAttributes{hipMemcpySrcAccessOrderStream, {}, {}, hipMemcpyFlagDefault},
      hipMemcpyAttributes{hipMemcpySrcAccessOrderStream, {}, {}, hipMemcpyFlagDefault},
  };
  std::array<size_t, 3> attrs_idxs{0, 1, 2};

  SECTION("Null attrs with nonzero numAttrs") {
    HIP_CHECK_ERROR(
        hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount, nullptr,
                            attrs_idxs.data(), 1, nullptr, stream_guard.stream()),
        hipErrorInvalidValue);
  }

  SECTION("Null attrsIdxs with nonzero numAttrs") {
    HIP_CHECK_ERROR(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount,
                                        attrs.data(), nullptr, 1, nullptr, stream_guard.stream()),
                    hipErrorInvalidValue);
  }

  SECTION("First attrsIdxs entry is not zero") {
    attrs_idxs[0] = 1;
    HIP_CHECK_ERROR(
        hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount, attrs.data(),
                            attrs_idxs.data(), 1, nullptr, stream_guard.stream()),
        hipErrorInvalidValue);
  }

  SECTION("numAttrs exceeds count") {
    HIP_CHECK_ERROR(
        hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount, attrs.data(),
                            attrs_idxs.data(), kCount + 1, nullptr, stream_guard.stream()),
        hipErrorInvalidValue);
  }

  SECTION("attrsIdxs is not monotonic") {
    attrs_idxs[1] = 0;
    HIP_CHECK_ERROR(
        hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount, attrs.data(),
                            attrs_idxs.data(), 2, nullptr, stream_guard.stream()),
        hipErrorInvalidValue);
  }

  SECTION("attrsIdxs entry is out of range") {
    attrs_idxs[1] = kCount;
    HIP_CHECK_ERROR(
        hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount, attrs.data(),
                            attrs_idxs.data(), 2, nullptr, stream_guard.stream()),
        hipErrorInvalidValue);
  }

  SECTION("Invalid source access order") {
    attrs[0].srcAccessOrder = hipMemcpySrcAccessOrderInvalid;
    HIP_CHECK_ERROR(
        hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount, attrs.data(),
                            attrs_idxs.data(), 1, nullptr, stream_guard.stream()),
        hipErrorInvalidValue);
  }
}

#if HT_AMD
/**
 * Test Description
 * ------------------------
 * - Verifies D2D batch copies with hipMemcpyFlagExtOpSwap.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_D2D_Swap) {
  constexpr size_t copy_count = 3;
  constexpr size_t copy_size = kSmallCopySize;
  constexpr int kSwapSrcValue = 23;
  constexpr int kSwapDstValue = 47;
  BatchConfig config{copy_count, copy_size};
  StreamGuard stream_guard(Streams::created);
  std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
  std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
  std::vector<void*> src_ptrs = MakeBatchPtrs(src);
  std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);
  std::vector<size_t> sizes(src_ptrs.size(), copy_size);
  hipMemcpyAttributes attr{hipMemcpySrcAccessOrderStream, {}, {}, hipMemcpyFlagExtOpSwap};
  size_t attrs_idxs[1] = {0};

  FillDeviceBuffers(src_ptrs, copy_size, kSwapSrcValue);
  FillDeviceBuffers(dst_ptrs, copy_size, kSwapDstValue);

  hipError_t status =
      hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), src_ptrs.size(), &attr,
                          attrs_idxs, 1, nullptr, stream_guard.stream());
  if (status == hipErrorNotSupported) {
    SUCCEED("hipMemcpyFlagExtOpSwap is not supported on this device");
  } else {
    HIP_CHECK(status);
    HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));
    VerifyDeviceBuffers(src_ptrs, copy_size, kSwapDstValue);
    VerifyDeviceBuffers(dst_ptrs, copy_size, kSwapSrcValue);
  }
}
#endif

/**
 * Test Description
 * ------------------------
 * - Verifies D2D batch copies across generated copy sizes, counts, pointer
 * patterns, and copy flags.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_D2D_Functional) {
  const size_t copy_count = GENERATE(1, 8);
  const size_t copy_size = GENERATE(kSmallCopySize, kMediumCopySize, kLargeCopySize);
  const PointerPattern pointer_pattern =
      GENERATE(PointerPattern::kBasePointers, PointerPattern::kOffsetPointers,
               PointerPattern::kUnalignedPointers, PointerPattern::kBroadcastSource);
#if HT_AMD
  const hipMemcpyFlags flag = GENERATE(hipMemcpyFlagDefault, hipMemcpyFlagExtPreferCE);
#else
  const hipMemcpyFlags flag = hipMemcpyFlagDefault;
#endif
  const size_t offset_bytes = pointer_pattern == PointerPattern::kOffsetPointers      ? sizeof(int)
                              : pointer_pattern == PointerPattern::kUnalignedPointers ? 1
                                                                                      : 0;

  BatchConfig config{copy_count, copy_size};
  StreamGuard stream_guard(Streams::created);
  std::vector<LinearAllocGuard<int>> src =
      AllocateBatchBuffers(LinearAllocs::hipMalloc, config, offset_bytes);
  std::vector<LinearAllocGuard<int>> dst =
      AllocateBatchBuffers(LinearAllocs::hipMalloc, config, offset_bytes);
  std::vector<void*> src_ptrs = MakeBatchPtrs(src, offset_bytes);
  std::vector<void*> dst_ptrs = MakeBatchPtrs(dst, offset_bytes);
  std::vector<size_t> sizes(config.copy_count, config.copy_size);
  size_t attrs_idxs[1] = {0};
  hipMemcpyAttributes attr{
      hipMemcpySrcAccessOrderAny, {}, {}, static_cast<unsigned int>(flag)};

  if (pointer_pattern == PointerPattern::kBroadcastSource) {
    FillDeviceBuffers(src_ptrs, copy_size, kPatternValue);
    void* broadcast_src = src_ptrs.front();
    std::fill(src_ptrs.begin(), src_ptrs.end(), broadcast_src);
  } else {
    FillDeviceBuffers(src_ptrs, copy_size, kPatternValue);
  }

  HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), copy_count, &attr,
                                attrs_idxs, 1, nullptr, stream_guard.stream()));
  HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));

  if (pointer_pattern == PointerPattern::kBroadcastSource) {
    VerifyDeviceBuffers(dst_ptrs, copy_size, kPatternValue, false);
  } else {
    VerifyDeviceBuffers(dst_ptrs, copy_size);
  }
}

/**
 * Test Description
 * ------------------------
 * - Verifies H2D batch copies across generated host source allocation types,
 * copy counts, and copy sizes.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_H2D_Functional) {
  const LinearAllocs host_alloc_type = GENERATE(LinearAllocs::malloc, LinearAllocs::hipHostMalloc);
  const size_t copy_count = GENERATE(1, 8);
  const size_t copy_size = GENERATE(kSmallCopySize, kMediumCopySize, kLargeCopySize);

  BatchConfig config{copy_count, copy_size};
  StreamGuard stream_guard(Streams::created);
  std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(host_alloc_type, config);
  std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
  std::vector<void*> src_ptrs = MakeBatchPtrs(src);
  std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);
  std::vector<size_t> sizes(config.copy_count, config.copy_size);

  FillHostBuffers(src, copy_size);

  HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), copy_count, nullptr,
                                nullptr, 0, nullptr, stream_guard.stream()));
  HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));

  VerifyDeviceBuffers(dst_ptrs, copy_size);
}

/**
 * Test Description
 * ------------------------
 * - Verifies that pageable H2D source access is complete before
 * hipMemcpyBatchAsync returns when srcAccessOrder is DuringApiCall.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_H2D_Pageable_DuringApiCall_SourceAccess) {
  constexpr size_t copy_count = 8;
  constexpr size_t copy_size = kLargeCopySize;
  constexpr int kOriginalValue = 17;
  constexpr int kAlteredValue = 23;
  BatchConfig config{copy_count, copy_size};
  StreamGuard stream_guard(Streams::created);
  std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(LinearAllocs::malloc, config);
  std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
  std::vector<void*> src_ptrs = MakeBatchPtrs(src);
  std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);
  std::vector<size_t> sizes(config.copy_count, config.copy_size);
  size_t attrs_idxs[1] = {0};
  hipMemcpyAttributes attr{hipMemcpySrcAccessOrderDuringApiCall, {}, {}, hipMemcpyFlagDefault};

  FillHostBuffers(src, copy_size, kOriginalValue);

  HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), copy_count, &attr,
                                attrs_idxs, 1, nullptr, stream_guard.stream()));
  FillHostBuffers(src, copy_size, kAlteredValue);
  HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));
  VerifyDeviceBuffers(dst_ptrs, copy_size, kOriginalValue);
  VerifyHostBuffers(src, copy_size, kAlteredValue);
}

/**
 * Test Description
 * ------------------------
 * - Verifies that pageable H2D source access observes previous same-stream
 * writes to the source when srcAccessOrder is Stream.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_H2D_Pageable_Stream_SourceAccess) {
  constexpr size_t copy_count = 1;
  size_t copy_size = GENERATE(kSmallCopySize, kLargeCopySize);
  constexpr int kStreamProducedValue = 47;
  BatchConfig config{copy_count, copy_size};
  StreamGuard stream_guard(Streams::created);
  std::vector<LinearAllocGuard<int>> producer =
      AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
  std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(LinearAllocs::malloc, config);
  std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
  std::vector<void*> src_ptrs = MakeBatchPtrs(src);
  std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);
  std::vector<size_t> sizes(config.copy_count, config.copy_size);
  size_t attrs_idxs[1] = {0};
  hipMemcpyAttributes attr{hipMemcpySrcAccessOrderStream, {}, {}, hipMemcpyFlagDefault};

  FillDeviceBuffers(MakeBatchPtrs(producer), copy_size, kStreamProducedValue);

  for (size_t i = 0; i < copy_count; ++i) {
    HIP_CHECK(hipMemcpyAsync(src_ptrs[i], producer[i].ptr(), copy_size, hipMemcpyDeviceToHost,
                             stream_guard.stream()));
  }
  HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), copy_count, &attr,
                                attrs_idxs, 1, nullptr, stream_guard.stream()));
  HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));

  VerifyDeviceBuffers(dst_ptrs, copy_size, kStreamProducedValue);
}

/**
 * Test Description
 * ------------------------
 * - Verifies D2H batch copies across generated host destination allocation
 * types, copy counts, and copy sizes.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_D2H_Functional) {
  const LinearAllocs host_alloc_type = GENERATE(LinearAllocs::malloc, LinearAllocs::hipHostMalloc);
  const size_t copy_count = GENERATE(1, 8);
  const size_t copy_size = GENERATE(kSmallCopySize, kMediumCopySize, kLargeCopySize);

  BatchConfig config{copy_count, copy_size};
  StreamGuard stream_guard(Streams::created);
  std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
  std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(host_alloc_type, config);
  std::vector<void*> src_ptrs = MakeBatchPtrs(src);
  std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);
  std::vector<size_t> sizes(config.copy_count, config.copy_size);

  FillDeviceBuffers(src_ptrs, copy_size, kPatternValue);

  HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), copy_count, nullptr,
                                nullptr, 0, nullptr, stream_guard.stream()));
  HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));

  VerifyHostBuffers(dst, copy_size);
}

/**
 * Test Description
 * ------------------------
 * - Verifies H2H batch copies across generated host allocation types, copy
 * counts, and copy sizes.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_H2H_Functional) {
  const LinearAllocs host_alloc_type = GENERATE(LinearAllocs::malloc, LinearAllocs::hipHostMalloc);
  const size_t copy_count = GENERATE(1, 8);
  const size_t copy_size = kSmallCopySize;

  BatchConfig config{copy_count, copy_size};
  StreamGuard stream_guard(Streams::created);
  std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(host_alloc_type, config);
  std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(host_alloc_type, config);
  std::vector<void*> src_ptrs = MakeBatchPtrs(src);
  std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);
  std::vector<size_t> sizes(config.copy_count, config.copy_size);

  FillHostBuffers(src, copy_size);

  HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), copy_count, nullptr,
                                nullptr, 0, nullptr, stream_guard.stream()));
  HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));

  VerifyHostBuffers(dst, copy_size);
}

/**
 * Test Description
 * ------------------------
 * - Verifies one batch containing H2D, D2D, D2H, and H2H copies.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Mixed_Functional) {
  constexpr size_t copy_size = kMediumCopySize;
  const size_t copy_elements = copy_size / sizeof(int);

  if (HipTest::getDeviceCount() < 3) {
    HIP_SKIP_TEST("Test requires at least three GPUs.");
  }

  HIP_CHECK(hipSetDevice(0));
  StreamGuard stream_guard(Streams::created);

  LinearAllocGuard<int> h2d_src(LinearAllocs::malloc, copy_size);
  LinearAllocGuard<int> h2d_dst(LinearAllocs::hipMalloc, copy_size);

  HIP_CHECK(hipSetDevice(1));
  LinearAllocGuard<int> d2d_src(LinearAllocs::hipMalloc, copy_size);
  LinearAllocGuard<int> d2d_dst(LinearAllocs::hipMalloc, copy_size);

  HIP_CHECK(hipSetDevice(2));
  LinearAllocGuard<int> d2h_src(LinearAllocs::hipMalloc, copy_size);

  LinearAllocGuard<int> d2h_dst(LinearAllocs::malloc, copy_size);
  LinearAllocGuard<int> h2h_src(LinearAllocs::malloc, copy_size);
  LinearAllocGuard<int> h2h_dst(LinearAllocs::malloc, copy_size);

  HIP_CHECK(hipSetDevice(0));
  std::array<void*, 4> src_ptrs{h2d_src.ptr(), d2d_src.ptr(), d2h_src.ptr(), h2h_src.ptr()};
  std::array<void*, 4> dst_ptrs{h2d_dst.ptr(), d2d_dst.ptr(), d2h_dst.ptr(), h2h_dst.ptr()};
  std::array<size_t, 4> sizes{copy_size, copy_size, copy_size, copy_size};

  std::fill_n(h2d_src.host_ptr(), copy_elements, kPatternValue);
  std::fill_n(h2h_src.host_ptr(), copy_elements, kPatternValue + 3);
  std::vector<int> source(copy_elements);
  std::fill(source.begin(), source.end(), kPatternValue + 1);
  HIP_CHECK(hipMemcpy(d2d_src.ptr(), source.data(), copy_size, hipMemcpyHostToDevice));
  std::fill(source.begin(), source.end(), kPatternValue + 2);
  HIP_CHECK(hipMemcpy(d2h_src.ptr(), source.data(), copy_size, hipMemcpyHostToDevice));

  HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), sizes.size(),
                                nullptr, nullptr, 0, nullptr, stream_guard.stream()));
  HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));

  std::vector<int> result(copy_elements);
  HIP_CHECK(hipMemcpy(result.data(), h2d_dst.ptr(), copy_size, hipMemcpyDeviceToHost));
  VerifyArrayFromBothEnds(result.data(), copy_elements, kPatternValue, 0);
  HIP_CHECK(hipMemcpy(result.data(), d2d_dst.ptr(), copy_size, hipMemcpyDeviceToHost));
  VerifyArrayFromBothEnds(result.data(), copy_elements, kPatternValue + 1, 1);
  VerifyArrayFromBothEnds(d2h_dst.host_ptr(), copy_elements, kPatternValue + 2, 2);
  VerifyArrayFromBothEnds(h2h_dst.host_ptr(), copy_elements, kPatternValue + 3, 3);
}

/**
 * Test Description
 * ------------------------
 * - Verifies default stream behavior, same-stream ordering, and event
 * dependency ordering for hipMemcpyBatchAsync.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_Stream) {
  constexpr size_t kCount = 2;
  constexpr size_t kCopyElements = kSmallCopySize / sizeof(int);
  BatchConfig config{kCount, kSmallCopySize};
  std::vector<size_t> sizes(config.copy_count, config.copy_size);

  SECTION("Default stream") {
    std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
    std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
    std::vector<void*> src_ptrs = MakeBatchPtrs(src);
    std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);
    FillDeviceBuffers(src_ptrs, kSmallCopySize, kPatternValue);

    HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount, nullptr,
                                  nullptr, 0, nullptr, nullptr));
    HIP_CHECK(hipStreamSynchronize(nullptr));

    VerifyDeviceBuffers(dst_ptrs, kSmallCopySize);
  }

  SECTION("Ordering after prior kernel on same stream") {
    StreamGuard stream_guard(Streams::created);
    std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
    std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
    std::vector<void*> src_ptrs = MakeBatchPtrs(src);
    std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);

    static_cast<void>(hipGetLastError());
    for (size_t i = 0; i < kCount; ++i) {
      VectorSet<<<1, 256, 0, stream_guard.stream()>>>(
          static_cast<int*>(src_ptrs[i]), kPatternValue + static_cast<int>(i), kCopyElements);
    }
    HIP_CHECK(hipGetLastError());
    HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount, nullptr,
                                  nullptr, 0, nullptr, stream_guard.stream()));
    HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));

    VerifyDeviceBuffers(dst_ptrs, kSmallCopySize);
  }

  SECTION("Ordering via event dependency") {
    StreamGuard producer_stream(Streams::created);
    StreamGuard copy_stream(Streams::created);
    EventsGuard events(1);
    std::vector<LinearAllocGuard<int>> src = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
    std::vector<LinearAllocGuard<int>> dst = AllocateBatchBuffers(LinearAllocs::hipMalloc, config);
    std::vector<void*> src_ptrs = MakeBatchPtrs(src);
    std::vector<void*> dst_ptrs = MakeBatchPtrs(dst);

    static_cast<void>(hipGetLastError());
    for (size_t i = 0; i < kCount; ++i) {
      VectorSet<<<1, 256, 0, producer_stream.stream()>>>(
          static_cast<int*>(src_ptrs[i]), kPatternValue + static_cast<int>(i), kCopyElements);
    }
    HIP_CHECK(hipGetLastError());
    HIP_CHECK(hipEventRecord(events[0], producer_stream.stream()));
    HIP_CHECK(hipStreamWaitEvent(copy_stream.stream(), events[0], 0));
    HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), kCount, nullptr,
                                  nullptr, 0, nullptr, copy_stream.stream()));
    HIP_CHECK(hipStreamSynchronize(copy_stream.stream()));

    VerifyDeviceBuffers(dst_ptrs, kSmallCopySize);
  }
}

/**
 * Test Description
 * ------------------------
 * - Verifies peer-to-peer batches across peer-accessible device pairs.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_P2P_Functional) {
  const std::vector<std::pair<int, int>> peer_pairs = GetPeerAccessibleDevicePairs();
  const int stream_device = peer_pairs.front().first;
#if HT_AMD
  const hipMemcpyFlags flag = GENERATE(hipMemcpyFlagDefault, hipMemcpyFlagExtPreferCE);
#else
  const hipMemcpyFlags flag = hipMemcpyFlagDefault;
#endif

  constexpr size_t copy_size = kSmallCopySize;
  constexpr size_t batch_count = 2;
  const size_t total_copy_count = peer_pairs.size() * batch_count;
  std::vector<LinearAllocGuard<int>> src_allocations;
  std::vector<LinearAllocGuard<int>> dst_allocations;
  std::vector<void*> src_ptrs;
  std::vector<void*> dst_ptrs;
  std::vector<size_t> sizes(total_copy_count, copy_size);
  hipMemcpyAttributes attr{
      hipMemcpySrcAccessOrderStream, {}, {}, static_cast<unsigned int>(flag)};
  size_t attrs_idxs[1] = {0};

  EnablePeerAccess(peer_pairs);
  for (const auto& [src_device, dst_device] : peer_pairs) {
    for (size_t i = 0; i < batch_count; ++i) {
      HIP_CHECK(hipSetDevice(src_device));
      src_allocations.emplace_back(LinearAllocs::hipMalloc, copy_size);
      src_ptrs.push_back(src_allocations.back().ptr());

      HIP_CHECK(hipSetDevice(dst_device));
      dst_allocations.emplace_back(LinearAllocs::hipMalloc, copy_size);
      dst_ptrs.push_back(dst_allocations.back().ptr());
    }
  }

  HIP_CHECK(hipSetDevice(stream_device));
  StreamGuard stream_guard(Streams::created);
  FillDeviceBuffers(src_ptrs, copy_size, kPatternValue);
  HIP_CHECK(hipSetDevice(stream_device));
  HIP_CHECK(hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), sizes.size(), &attr,
                                attrs_idxs, 1, nullptr, stream_guard.stream()));
  HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));

  VerifyDeviceBuffers(dst_ptrs, copy_size);
  DisablePeerAccess(peer_pairs);
  HIP_CHECK(hipSetDevice(stream_device));
}

#if HT_AMD
/**
 * Test Description
 * ------------------------
 * - Verifies H2D hipMemcpyBatchAsync with hipMemcpyFlagExtOpIndirectSrc copies
 * from the host buffer referenced by a pinned pointer slot.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_IndirectSrc) {
  constexpr size_t copy_size = kSmallCopySize;

  StreamGuard stream_guard(Streams::created);
  LinearAllocGuard<int> src(LinearAllocs::hipHostMalloc, copy_size);
  LinearAllocGuard<int> dst(LinearAllocs::hipMalloc, copy_size);
  LinearAllocGuard<char> src_slot(LinearAllocs::hipHostMalloc, sizeof(void*));

  const size_t copy_elements = copy_size / sizeof(int);
  std::fill_n(src.host_ptr(), copy_elements, kPatternValue);

  void* src_ptr = src.ptr();
  std::memcpy(src_slot.ptr(), &src_ptr, sizeof(void*));

  std::vector<void*> dst_ptrs{dst.ptr()};
  std::vector<void*> src_ptrs{src_slot.ptr()};
  std::vector<size_t> sizes{copy_size};
  hipMemcpyAttributes attr{hipMemcpySrcAccessOrderStream, {}, {}, hipMemcpyFlagExtOpIndirectSrc};
  size_t attrs_idx = 0;

  hipError_t status = hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), 1, &attr,
                                          &attrs_idx, 1, nullptr, stream_guard.stream());
  if (status == hipErrorNotSupported) {
    SUCCEED("hipMemcpyFlagExtOpIndirectSrc is not supported on this device");
  } else {
    HIP_CHECK(status);
    HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));
    VerifyDeviceBuffers(dst_ptrs, copy_size);
  }
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
 * - Verifies D2H hipMemcpyBatchAsync with hipMemcpyFlagExtOpIndirectDst copies
 * into the host buffer referenced by a pinned pointer slot.
 * Test source
 * ------------------------
 * - catch/unit/memory/hipMemcpyBatchAsync.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.1
 */
HIP_TEST_CASE(Unit_hipMemcpyBatchAsync_IndirectDst) {
  constexpr size_t copy_size = kSmallCopySize;

  StreamGuard stream_guard(Streams::created);
  LinearAllocGuard<int> src(LinearAllocs::hipMalloc, copy_size);
  LinearAllocGuard<int> dst(LinearAllocs::hipHostMalloc, copy_size);
  LinearAllocGuard<char> dst_slot(LinearAllocs::hipHostMalloc, sizeof(void*));

  const size_t copy_elements = copy_size / sizeof(int);
  std::vector<int> host_pattern(copy_elements, kPatternValue);
  HIP_CHECK(hipMemcpy(src.ptr(), host_pattern.data(), copy_size, hipMemcpyHostToDevice));
  std::fill_n(dst.host_ptr(), copy_elements, 0);

  void* dst_ptr = dst.ptr();
  std::memcpy(dst_slot.ptr(), &dst_ptr, sizeof(void*));

  std::vector<void*> src_ptrs{src.ptr()};
  std::vector<void*> dst_ptrs{dst_slot.ptr()};
  std::vector<size_t> sizes{copy_size};
  hipMemcpyAttributes attr{hipMemcpySrcAccessOrderStream, {}, {}, hipMemcpyFlagExtOpIndirectDst};
  size_t attrs_idx = 0;

  hipError_t status = hipMemcpyBatchAsync(dst_ptrs.data(), src_ptrs.data(), sizes.data(), 1, &attr,
                                          &attrs_idx, 1, nullptr, stream_guard.stream());
  if (status == hipErrorNotSupported) {
    SUCCEED("hipMemcpyFlagExtOpIndirectDst is not supported on this device");
  } else {
    HIP_CHECK(status);
    HIP_CHECK(hipStreamSynchronize(stream_guard.stream()));
    VerifyArrayFromBothEnds(dst.host_ptr(), copy_elements, kPatternValue, 0);
  }
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
 * End doxygen group MemoryTest.
 * @}
 */
