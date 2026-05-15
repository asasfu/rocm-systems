/*
Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include <hip_test_kernels.hh>
#include <hip_test_common.hh>
#define ALIGNSIZE 4096

// The tests will verify kernel monitor based on new memory based synchronization builtins on
// gfx1250/1251

#if defined(__gfx1250__) || defined(__gfx1251__)
#define __mem_based_sync_support__ 1
__device__ constexpr int hint_and_scope = 2 << 3;  // temporal + Device
// HW uses first 7 bits to apply wait.
__device__ constexpr short duration = static_cast<short>(0x7f);
#endif

__global__ void
gpu_ping(int* ptr, const int Num, int *runNum) {
#if __mem_based_sync_support__
  int run = 0;
  *ptr = 1;
  while (run++ < Num) {
    while (__builtin_amdgcn_flat_load_monitor_b32(ptr, hint_and_scope) == 1) {
      __builtin_amdgcn_s_monitor_sleep(duration);
    }
    *ptr = 1;
  }
  *runNum = run;
#else
  if (ptr && runNum) *runNum = Num + 1; // Dummy
#endif
}

__global__ void
gpu_pong(int* ptr, const int Num, int *runNum) {
#if __mem_based_sync_support__
  int run = 0;
  while (run++ < Num) {
    while (__builtin_amdgcn_flat_load_monitor_b32(ptr, hint_and_scope) == 0) {
        __builtin_amdgcn_s_monitor_sleep(duration);
    }
    *ptr = 0;
  }
  *runNum = run;
#else
  if (ptr && runNum) *runNum = Num + 1; // Dummy
#endif
}

__global__ void
gpu_ping(int *ptr0, int *ptr1, const int expected0, const int expected1,
              const int toUpdate0, const int toUpdate1, const int Num, int *runNum) {
#if __mem_based_sync_support__
  int run = 0;
  *ptr1 = toUpdate0;
  while (run++ < Num) {
    while (__builtin_amdgcn_flat_load_monitor_b32(ptr0, hint_and_scope) != expected0) {
        __builtin_amdgcn_s_monitor_sleep(duration);
    }
    *ptr1 = toUpdate1;
    while (__builtin_amdgcn_flat_load_monitor_b32(ptr0, hint_and_scope) != expected1) {
        __builtin_amdgcn_s_monitor_sleep(duration);
    }
    *ptr1 = toUpdate0;
  }
  *runNum = run;
#else
  if (ptr0 && ptr1 && expected0 != expected1 &&
      toUpdate0 != toUpdate1 && runNum) *runNum = Num + 1; // Dummy
#endif
}

__global__ void
gpu_pong(int *ptr0, int *ptr1, const int expected0, const int expected1,
              const int toUpdate0, const int toUpdate1, const int Num, int *runNum) {
#if __mem_based_sync_support__
  int run = 0;
  while (run++ < Num) {
    while (__builtin_amdgcn_flat_load_monitor_b32(ptr0, hint_and_scope) != expected0) {
        __builtin_amdgcn_s_monitor_sleep(duration);
    }
    *ptr1 = toUpdate0;
    while (__builtin_amdgcn_flat_load_monitor_b32(ptr0, hint_and_scope) != expected1) {
        __builtin_amdgcn_s_monitor_sleep(duration);
    }
    *ptr1 = toUpdate1;
  }
  *runNum = run;
#else
  if (ptr0 && ptr1 && expected0 != expected1 &&
      toUpdate0 != toUpdate1 && runNum) *runNum = Num + 1; // Dummy
#endif
}

static void test_1_cacheline() {
  const int Num =100;
  int *A_d; // A buffer whose cacheline will be updated and checked in 2 threads
  int *runNum; // Used to keep iteration number for verification
  HIP_CHECK(hipMalloc(reinterpret_cast<void**>(&A_d), sizeof(*A_d)));
  HIP_CHECK(hipHostMalloc(reinterpret_cast<void**>(&runNum), 2 * sizeof(*runNum)));
  HIP_CHECK(hipMemset(A_d, 0, sizeof(int)));
  runNum[0] = runNum[1] = 0;

  hipStream_t stream[2];
  HIP_CHECK(hipStreamCreate(&stream[0]));
  HIP_CHECK(hipStreamCreate(&stream[1]));

  hipLaunchKernelGGL(gpu_ping, dim3(1), dim3(1), 0, stream[0], A_d, Num, runNum);
  HIP_CHECK(hipGetLastError());
  hipLaunchKernelGGL(gpu_pong, dim3(1), dim3(1), 0, stream[1], A_d, Num, runNum + 1);
  HIP_CHECK(hipGetLastError());

  HIP_CHECK(hipStreamSynchronize(stream[0]));
  HIP_CHECK(hipStreamSynchronize(stream[1]));
  REQUIRE(runNum[0] == Num + 1);
  REQUIRE(runNum[1] == Num + 1);
  HIP_CHECK(hipStreamDestroy(stream[0]));
  HIP_CHECK(hipStreamDestroy(stream[1]));
  HIP_CHECK(hipFree(A_d));
  HIP_CHECK(hipHostFree(runNum));
}

static void test_2_cachelines() {
  const int Num =100;
  int *A_d; // A buffer whose cacheline will be updated in thread 2 and checked in thread 1
  int *B_d; // A different buffer whose cacheline will be updated in thread 1 and checked in thread 2
  int *runNum; // Used to keep iteration number for verification
  HIP_CHECK(hipMalloc(reinterpret_cast<void**>(&A_d), sizeof(*A_d)));
  HIP_CHECK(hipMalloc(reinterpret_cast<void**>(&B_d), sizeof(*B_d)));

  // Device memory is ALIGNSIZE aligned during allocation, so this can guarantee that A_d and B_d
  // have different cache lines as cache line size (usually 64) is much smaller than ALIGNSIZE.
  REQUIRE((reinterpret_cast<uintptr_t>(A_d) % ALIGNSIZE) == 0);
  REQUIRE((reinterpret_cast<uintptr_t>(B_d) % ALIGNSIZE) == 0);
  const auto distance = (B_d > A_d ? (B_d - A_d) : (A_d - B_d)) * sizeof(*A_d);
  REQUIRE(distance >= ALIGNSIZE);

  HIP_CHECK(hipHostMalloc(reinterpret_cast<void**>(&runNum), 2 * sizeof(*runNum)));
  HIP_CHECK(hipMemset(A_d, 0, sizeof(*A_d)));
  HIP_CHECK(hipMemset(B_d, 0, sizeof(*B_d)));
  runNum[0] = runNum[1] = 0;

  hipStream_t stream[2];
  HIP_CHECK(hipStreamCreate(&stream[0]));
  HIP_CHECK(hipStreamCreate(&stream[1]));
  const int val[2][2] = {{34594, 6137394}, {571490, 739840}}; // Random
  hipLaunchKernelGGL(gpu_ping, dim3(1), dim3(1), 0, stream[0], A_d, B_d,
                     val[0][0], val[0][1], val[1][0], val[1][1], Num, runNum);
  HIP_CHECK(hipGetLastError());
  hipLaunchKernelGGL(gpu_pong, dim3(1), dim3(1), 0, stream[1], B_d, A_d,
                     val[1][0], val[1][1], val[0][0], val[0][1], Num, runNum + 1);
  HIP_CHECK(hipGetLastError());

  HIP_CHECK(hipStreamSynchronize(stream[0]));
  HIP_CHECK(hipStreamSynchronize(stream[1]));
  REQUIRE(runNum[0] == Num + 1);
  REQUIRE(runNum[1] == Num + 1);
  HIP_CHECK(hipStreamDestroy(stream[0]));
  HIP_CHECK(hipStreamDestroy(stream[1]));
  HIP_CHECK(hipFree(A_d));
  HIP_CHECK(hipFree(B_d));
  HIP_CHECK(hipHostFree(runNum));
}

HIP_TEST_CASE(Unit_test_kernel_monitor) {
  hipDeviceProp_t props;
  HIP_CHECK(hipSetDevice(0));
  HIP_CHECK(hipGetDeviceProperties(&props, 0));
  if (props.major == 12 && props.minor == 5) {
      SECTION("with_1_cacheline") {
        test_1_cacheline();
      }
      SECTION("test_2_cachelines") {
        test_2_cachelines();
      }
  } else {
    HipTest::HIP_SKIP_TEST("Unit_test_kernel_monitor isn't supported on the device!");
  }
}
