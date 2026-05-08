/*
 * Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#include "execution_control_common.hh"

#include <hip_test_common.hh>
#include <hip/hip_cooperative_groups.h>

__global__ void kernel() {}

__global__ void kernel2() {}

__global__ void kernel_42(int* val) { *val = 42; }

__global__ void coop_kernel() {
  cooperative_groups::grid_group grid = cooperative_groups::this_grid();
  grid.sync();
}

// Uses 64KB of LDS (16384 ints * 4 bytes). Each thread writes its tid into the
// shared array, syncs, then thread 0 writes lds[0] back to global memory so the
// compiler cannot optimise the allocation away.
__global__ void kernel_lds_64k(int* output) {
  __shared__ int lds[16384];  // 16384 * 4 = 65536 bytes = 64 KB
  int tid = threadIdx.x;
  for (int i = tid; i < 16384; i += blockDim.x) {
    lds[i] = i;
  }
  __syncthreads();
  if (tid == 0 && output) {
    output[0] = lds[0];
  }
}