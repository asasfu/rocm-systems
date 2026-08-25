/*
 * Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#include <hip_test_common.hh>
#include <hip/hip_runtime.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>

/**
 * @addtogroup GL2ResidencyPerformance GL2 Residency Performance Tests
 * @{
 * @ingroup PerformanceTest
 * Performance tests for GL2 Residency Control feature
 */

#if HT_AMD

/**
 * Kernel: Weighted sum with repeated access to weights
 * Simulates ML inference - small weight array accessed repeatedly
 *
 * @param weights - Small array that should stay in L2 (read many times)
 * @param inputs - Large input data (streamed through)
 * @param outputs - Results
 * @param numWeights - Size of weight array
 * @param iterations - How many times to reuse weights
 */
__global__ void WeightedSumKernel(const float* weights, const float* inputs,
                                   float* outputs, int numWeights, int iterations,
                                   int numElements) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;

  if (idx >= numElements) return;  // Bounds check

  float result = 0.0f;

  // Repeatedly access the SAME weights array (benefits from persistent L2)
  for (int iter = 0; iter < iterations; iter++) {
    for (int w = 0; w < numWeights; w++) {
      result += weights[w] * inputs[idx * numWeights + w];
    }
  }

  outputs[idx] = result;
}

bool isGL2ResidencySupported() {
#if __linux__
  int deviceId;
  HIP_CHECK(hipGetDevice(&deviceId));
  hipDeviceProp_t props;
  HIP_CHECK(hipGetDeviceProperties(&props, deviceId));

  return props.persistingL2CacheMaxSize > 0;
#else
  return false;
#endif
}

/**
 * Test Description
 * ------------------------
 * Performance scaling test: Measure kernel performance with L2 reservation
 * from 0% to 100% in 25% increments.
 *
 * Expected behavior:
 *   - Higher L2 reservation → weights stay in L2 → faster execution
 *   - Shows performance improvement curve
 *
 * Test source
 * ------------------------
 *  - performance/scenarios/residency/gl2_scaling.cc
 *
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 7.0
 *  - Device: MI450 or later
 *  - Linux only
 */
TEST_CASE("Performance_GL2Residency_Scaling") {
  if (!isGL2ResidencySupported()) {
    HIP_SKIP_TEST("GL2 Residency Control not supported. Requires MI450+");
    return;
  }

  // Get device properties
  hipDeviceProp_t props;
  HIP_CHECK(hipGetDeviceProperties(&props, 0));

  size_t maxL2 = props.persistingL2CacheMaxSize;

  std::cout << "\n========================================" << std::endl;
  std::cout << "GL2 Residency Scaling Performance Test" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "Device: " << props.gcnArchName << std::endl;
  std::cout << "Max L2 Reservation: " << maxL2 / 1024 / 1024 << " MB" << std::endl;
  std::cout << "========================================\n" << std::endl;

  // Test configuration
  const size_t numWeights = 1024;             // 4 KB of weights (fits in L2)
  const size_t numElements = 256 * 1024;      // 256K output elements
  const int iterations = 1000;                // Reuse weights 1000 times
  const int warmupRuns = 3;
  const int benchmarkRuns = 10;

  // Allocate host memory
  std::vector<float> h_weights(numWeights, 1.0f);
  std::vector<float> h_inputs(numElements * numWeights, 2.0f);
  std::vector<float> h_outputs(numElements, 0.0f);

  // Allocate device memory
  float *d_weights, *d_inputs, *d_outputs;
  HIP_CHECK(hipMalloc(&d_weights, numWeights * sizeof(float)));
  HIP_CHECK(hipMalloc(&d_inputs, numElements * numWeights * sizeof(float)));
  HIP_CHECK(hipMalloc(&d_outputs, numElements * sizeof(float)));

  // Copy data to device
  HIP_CHECK(hipMemcpy(d_weights, h_weights.data(), numWeights * sizeof(float),
                      hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(d_inputs, h_inputs.data(), numElements * numWeights * sizeof(float),
                      hipMemcpyHostToDevice));

  // Kernel configuration
  const int blockSize = 256;
  const int gridSize = (numElements + blockSize - 1) / blockSize;

  // Test different L2 reservation levels: 0%, 25%, 50%, 75%, 100%
  std::vector<int> percentages = {0, 25, 50, 75, 100};

  std::cout << std::setw(15) << "L2 Reservation"
            << std::setw(20) << "Avg Time (ms)"
            << std::setw(15) << "Speedup"
            << std::setw(20) << "Bandwidth (GB/s)" << std::endl;
  std::cout << std::string(70, '-') << std::endl;

  double baselineTime = 0.0;

  for (int pct : percentages) {
    size_t l2Size = (maxL2 * pct) / 100;

    // Set L2 reservation
    HIP_CHECK(hipDeviceSetLimit(hipLimitPersistingL2CacheSize, l2Size));

    // Warmup runs
    for (int i = 0; i < warmupRuns; i++) {
      WeightedSumKernel<<<gridSize, blockSize>>>(d_weights, d_inputs, d_outputs,
                                                   numWeights, iterations, numElements);
      HIP_CHECK(hipDeviceSynchronize());
    }

    // Benchmark runs
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < benchmarkRuns; i++) {
      WeightedSumKernel<<<gridSize, blockSize>>>(d_weights, d_inputs, d_outputs,
                                                   numWeights, iterations, numElements);
    }
    HIP_CHECK(hipDeviceSynchronize());

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    double avgTime = elapsed.count() / benchmarkRuns;

    // Calculate bandwidth (reading weights repeatedly + inputs once + writing outputs)
    // Weights are read 'iterations' times
    size_t bytesRead = (numWeights * iterations + numElements * numWeights) * sizeof(float);
    size_t bytesWritten = numElements * sizeof(float);
    size_t totalBytes = bytesRead + bytesWritten;
    double bandwidth = (totalBytes / 1e9) / (avgTime / 1000.0);  // GB/s

    if (pct == 0) {
      baselineTime = avgTime;
    }

    double speedup = baselineTime / avgTime;

    std::cout << std::setw(10) << pct << "% ("
              << std::setw(2) << (l2Size / 1024 / 1024) << " MB)"
              << std::setw(15) << std::fixed << std::setprecision(2) << avgTime
              << std::setw(10) << std::fixed << std::setprecision(2) << speedup << "x"
              << std::setw(20) << std::fixed << std::setprecision(2) << bandwidth
              << std::endl;
  }

  std::cout << "\n========================================" << std::endl;
  std::cout << "Test Configuration:" << std::endl;
  std::cout << "  Weight array size: "
            << numWeights * sizeof(float) / 1024 / 1024 << " MB" << std::endl;
  std::cout << "  Input data size: "
            << numElements * numWeights * sizeof(float) / 1024 / 1024
            << " MB" << std::endl;
  std::cout << "  Weight reuse iterations: " << iterations << std::endl;
  std::cout << "  Benchmark runs: " << benchmarkRuns << std::endl;
  std::cout << "========================================\n" << std::endl;

  // Cleanup
  HIP_CHECK(hipFree(d_weights));
  HIP_CHECK(hipFree(d_inputs));
  HIP_CHECK(hipFree(d_outputs));

  // Restore original L2 setting
  HIP_CHECK(hipDeviceSetLimit(hipLimitPersistingL2CacheSize, 0));

  // The test always "passes" - it's just measuring performance
  REQUIRE(true);
}

#endif  // HT_AMD

/**
 * End doxygen group.
 * @}
 */
