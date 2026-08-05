/*
Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <hip/amd_detail/amd_device_functions.h>
#include <hip_test_checkers.hh>
#include <hip_test_common.hh>
#include <hip_test_kernels.hh>
#include <vector>

// Sentinel written by primary and read by secondary to verify sync/overlap correctness.
static constexpr int kProgrammaticSentinel = 0x87654321;

// Primary kernel that writes to shared memory before triggering; secondary will verify.
__global__ void primaryKernelWithShared(int* shared) {
  // Initial work: flush a value to global memory that secondary must see after sync
  if (threadIdx.x == 0 && blockIdx.x == 0) {
    *shared = kProgrammaticSentinel;
  }
  // Trigger the secondary kernel (launch-ahead: secondary can start, but will wait at sync)
  hipTriggerProgrammaticLaunchCompletion();

  // Work that can coincide with the secondary kernel
}

// Secondary kernel that verifies it sees primary's write only after hipGridDependencySynchronize.
__global__ void secondaryKernelWithShared(int* output, int totalThreads, const int* shared) {
  // Independent work

  // Block until primary has completed and flushed; then we must see primary's write
  hipGridDependencySynchronize();

  // Dependent work: read what primary wrote; if sync is correct we see kProgrammaticSentinel
  int seen = *shared;
  int tid = threadIdx.x + blockDim.x * blockIdx.x;
  if (tid < totalThreads) {
    output[tid] = (seen == kProgrammaticSentinel) ? (tid * 3) : -1;
  }
}

/**
 * @addtogroup hipLaunchAttributeProgrammaticLaunch
 * @{
 * @ingroup KernelTest
 * Programmatic launch - hipLaunchAttributeProgrammaticStreamSerialization allows the kernel
 * to be launched ahead for programmatic dependent launch (AQL DispatchAheadProgrammatic).
 */

/**
 * Test Description
 * ------------------------
 * Verifies primary-then-secondary launch with correctness check: primary writes
 * a sentinel to global memory and triggers; secondary waits at
 * hipGridDependencySynchronize() then reads the sentinel. Pass only if
 * secondary sees the primary's write (overlap and ordering work as expected).
 *
 * Test source
 * ------------------------
 *   - catch/unit/kernel/hipProgrammaticLaunch.cc
 */
HIP_TEST_CASE(Unit_hipLaunchKernelEx_ProgrammaticLaunch) {
  if (!IsProgrammaticLaunchSupported()) {
    HIP_SKIP_TEST(HipTest::SkipReason::kProgrammaticLaunchUnsupported);
    return;
  }
  const int blockSize = 64;
  const int totalThreads = 256;
  const int numBlocks = (totalThreads + blockSize - 1) / blockSize;

  hipStream_t stream = nullptr;
  HIP_CHECK(hipStreamCreate(&stream));

  hipLaunchConfig_t config = {};
  config.gridDim = dim3{static_cast<uint32_t>(numBlocks), 1, 1};
  config.blockDim = dim3{static_cast<uint32_t>(blockSize), 1, 1};
  config.dynamicSmemBytes = 0;
  config.stream = stream;

  hipLaunchAttribute attrs[2];
  attrs[0].id = hipLaunchAttributeClusterDimension;
  attrs[0].val.clusterDim.x = 1;
  attrs[0].val.clusterDim.y = 1;
  attrs[0].val.clusterDim.z = 1;
  attrs[1].id = hipLaunchAttributeProgrammaticStreamSerialization;
  attrs[1].val.programmaticStreamSerializationAllowed = 1;
  config.attrs = attrs;
  config.numAttrs = 2;

  int* d_output = nullptr;
  int* d_shared = nullptr;
  HIP_CHECK(hipMalloc(&d_output, totalThreads * sizeof(int)));
  HIP_CHECK(hipMalloc(&d_shared, sizeof(int)));
  HIP_CHECK(hipMemset(d_output, 0, totalThreads * sizeof(int)));
  HIP_CHECK(hipMemset(d_shared, 0, sizeof(int)));

  // Primary writes sentinel then triggers; secondary waits at sync then reads sentinel.
  hipLaunchKernelGGL(primaryKernelWithShared, numBlocks, blockSize, 0, stream, d_shared);
  HIP_CHECK(hipLaunchKernelEx(&config, secondaryKernelWithShared, d_output, totalThreads,
                              d_shared));
  HIP_CHECK(hipDeviceSynchronize());

  std::vector<int> h_output(totalThreads);
  HIP_CHECK(hipMemcpy(h_output.data(), d_output, totalThreads * sizeof(int),
                      hipMemcpyDeviceToHost));

  // Correctness: secondary must see kProgrammaticSentinel (writes tid*3); else writes -1.
  for (int i = 0; i < totalThreads; i++) {
    REQUIRE(h_output[i] == i * 3);
  }

  HIP_CHECK(hipStreamDestroy(stream));
  HIP_CHECK(hipFree(d_shared));
  HIP_CHECK(hipFree(d_output));
}

/**
 * Test Description
 * ------------------------
 * Verifies programmatic launch via hipDrvLaunchKernelEx: primary with
 * hipLaunchKernelGGL, secondary with hipDrvLaunchKernelEx and
 * hipLaunchAttributeProgrammaticStreamSerialization. Same correctness
 * check (secondary must see primary's sentinel after sync).
 *
 * Test source
 * ------------------------
 *   - catch/unit/kernel/hipProgrammaticLaunch.cc
 */
HIP_TEST_CASE(Unit_hipDrvLaunchKernelEx_ProgrammaticLaunch) {
  if (!IsProgrammaticLaunchSupported()) {
    HIP_SKIP_TEST(HipTest::SkipReason::kProgrammaticLaunchUnsupported);
    return;
  }
  const int blockSize = 64;
  const int totalThreads = 256;
  const int numBlocks = (totalThreads + blockSize - 1) / blockSize;

  hipModule_t module = nullptr;
  HIP_CHECK(hipModuleLoad(&module, "programmaticLaunch.code"));
  hipFunction_t secondaryFunc = nullptr;
  HIP_CHECK(hipModuleGetFunction(&secondaryFunc, module, "secondaryKernelWithShared"));

  hipStream_t stream = nullptr;
  HIP_CHECK(hipStreamCreate(&stream));

  HIP_LAUNCH_CONFIG config = {};
  config.gridDimX = numBlocks;
  config.gridDimY = 1;
  config.gridDimZ = 1;
  config.blockDimX = blockSize;
  config.blockDimY = 1;
  config.blockDimZ = 1;
  config.sharedMemBytes = 0;
  config.hStream = stream;

  hipLaunchAttribute attrs[2];
  attrs[0].id = hipLaunchAttributeClusterDimension;
  attrs[0].val.clusterDim.x = 1;
  attrs[0].val.clusterDim.y = 1;
  attrs[0].val.clusterDim.z = 1;
  attrs[1].id = hipLaunchAttributeProgrammaticStreamSerialization;
  attrs[1].val.programmaticStreamSerializationAllowed = 1;
  config.attrs = attrs;
  config.numAttrs = 2;

  int* d_output = nullptr;
  int* d_shared = nullptr;
  HIP_CHECK(hipMalloc(&d_output, totalThreads * sizeof(int)));
  HIP_CHECK(hipMalloc(&d_shared, sizeof(int)));
  HIP_CHECK(hipMemset(d_output, 0, totalThreads * sizeof(int)));
  HIP_CHECK(hipMemset(d_shared, 0, sizeof(int)));

  int totalThreadsParam = totalThreads;
  void* kernelParams[] = {&d_output, &totalThreadsParam, &d_shared};

  hipLaunchKernelGGL(primaryKernelWithShared, numBlocks, blockSize, 0, stream, d_shared);
  HIP_CHECK(hipDrvLaunchKernelEx(&config, secondaryFunc, kernelParams, nullptr));
  HIP_CHECK(hipDeviceSynchronize());

  std::vector<int> h_output(totalThreads);
  HIP_CHECK(hipMemcpy(h_output.data(), d_output, totalThreads * sizeof(int),
                      hipMemcpyDeviceToHost));

  for (int i = 0; i < totalThreads; i++) {
    REQUIRE(h_output[i] == i * 3);
  }

  HIP_CHECK(hipModuleUnload(module));
  HIP_CHECK(hipStreamDestroy(stream));
  HIP_CHECK(hipFree(d_shared));
  HIP_CHECK(hipFree(d_output));
}

// Primary kernel for overlap detection: writes sentinel, triggers, then handshakes
// with the secondary to prove both are running concurrently.
__global__ void primaryOverlapKernel(int* sentinel, int* primary_ready,
                                     int* secondary_alive,
                                     unsigned long long timeout_cycles) {
  if (threadIdx.x == 0 && blockIdx.x == 0) {
    *sentinel = kProgrammaticSentinel;
  }
  __syncthreads();

  hipTriggerProgrammaticLaunchCompletion();

  if (threadIdx.x == 0 && blockIdx.x == 0) {
    atomicExch(primary_ready, 1);
    // Wait for secondary to signal it is alive (bounded by wall clock to avoid deadlock)
    bool secondary_found = false;
    unsigned long long start = wall_clock64();
    while (wall_clock64() - start < timeout_cycles) {
      if (atomicAdd(secondary_alive, 0) == 1) { secondary_found = true; break; }
    }
    if (!secondary_found) {
      atomicExch(primary_ready, 0);
    }
  }
}

// Secondary kernel for overlap detection: signals alive, checks if primary is
// still running (proves overlap), then waits at sync and verifies data.
__global__ void secondaryOverlapKernel(int* overlap_flag, const int* sentinel,
                                       int* primary_ready, int* secondary_alive) {
  if (threadIdx.x == 0 && blockIdx.x == 0) {
    atomicExch(secondary_alive, 1);
    if (atomicAdd(primary_ready, 0) == 1) {
      atomicExch(overlap_flag, 1);
    }
  }

  hipGridDependencySynchronize();

  if (threadIdx.x == 0 && blockIdx.x == 0) {
    if (*sentinel != kProgrammaticSentinel) {
      atomicExch(overlap_flag, -1);
    }
  }
}

/**
 * Test Description
 * ------------------------
 * Verifies that programmatic dependent launch actually overlaps execution of
 * primary and secondary kernels. Primary triggers then waits for secondary to
 * signal it is alive; secondary checks if primary is still running. If both
 * observe each other, overlap is confirmed. Data visibility after
 * hipGridDependencySynchronize is also validated.
 *
 * Test source
 * ------------------------
 *   - catch/unit/kernel/hipProgrammaticLaunch.cc
 */
HIP_TEST_CASE(Unit_hipLaunchKernelEx_ProgrammaticLaunch_Overlap) {
  if (!IsProgrammaticLaunchSupported()) {
    HIP_SKIP_TEST(HipTest::SkipReason::kProgrammaticLaunchUnsupported);
    return;
  }

  hipStream_t stream = nullptr;
  HIP_CHECK(hipStreamCreate(&stream));

  hipLaunchConfig_t config = {};
  config.gridDim = dim3{1, 1, 1};
  config.blockDim = dim3{1, 1, 1};
  config.dynamicSmemBytes = 0;
  config.stream = stream;

  hipLaunchAttribute attrs[2];
  attrs[0].id = hipLaunchAttributeClusterDimension;
  attrs[0].val.clusterDim.x = 1;
  attrs[0].val.clusterDim.y = 1;
  attrs[0].val.clusterDim.z = 1;
  attrs[1].id = hipLaunchAttributeProgrammaticStreamSerialization;
  attrs[1].val.programmaticStreamSerializationAllowed = 1;
  config.attrs = attrs;
  config.numAttrs = 2;

  int* d_sentinel = nullptr;
  int* d_primary_ready = nullptr;
  int* d_secondary_alive = nullptr;
  int* d_overlap_flag = nullptr;
  HIP_CHECK(hipMalloc(&d_sentinel, sizeof(int)));
  HIP_CHECK(hipMalloc(&d_primary_ready, sizeof(int)));
  HIP_CHECK(hipMalloc(&d_secondary_alive, sizeof(int)));
  HIP_CHECK(hipMalloc(&d_overlap_flag, sizeof(int)));
  HIP_CHECK(hipMemset(d_sentinel, 0, sizeof(int)));
  HIP_CHECK(hipMemset(d_primary_ready, 0, sizeof(int)));
  HIP_CHECK(hipMemset(d_secondary_alive, 0, sizeof(int)));
  HIP_CHECK(hipMemset(d_overlap_flag, 0, sizeof(int)));

  // Timeout in GPU clock cycles (~1ms at 1GHz)
  const unsigned long long timeout_cycles = 1000000ULL;
  hipLaunchKernelGGL(primaryOverlapKernel, 1, 1, 0, stream,
                     d_sentinel, d_primary_ready, d_secondary_alive, timeout_cycles);
  HIP_CHECK(hipLaunchKernelEx(&config, secondaryOverlapKernel,
                              d_overlap_flag, d_sentinel,
                              d_primary_ready, d_secondary_alive));
  HIP_CHECK(hipDeviceSynchronize());

  int h_overlap_flag = 0;
  HIP_CHECK(hipMemcpy(&h_overlap_flag, d_overlap_flag, sizeof(int), hipMemcpyDeviceToHost));

  // -1 = data corruption (hipGridDependencySynchronize did not wait properly)
  //  0 = no overlap (programmatic launch not active)
  //  1 = overlap confirmed
  REQUIRE(h_overlap_flag == 1);

  HIP_CHECK(hipStreamDestroy(stream));
  HIP_CHECK(hipFree(d_overlap_flag));
  HIP_CHECK(hipFree(d_secondary_alive));
  HIP_CHECK(hipFree(d_primary_ready));
  HIP_CHECK(hipFree(d_sentinel));
}

/**
 * End doxygen group hipLaunchAttributeProgrammaticLaunch.
 * @}
 */
