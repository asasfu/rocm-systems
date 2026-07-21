/*
 * Copyright (c) Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#include "execution_control_common.hh"

#include <hip_test_common.hh>
#include <hip/hip_runtime_api.h>

/**
 * @addtogroup hipFuncSetAttribute hipFuncSetAttribute
 * @{
 * @ingroup ExecutionTest
 * `hipFuncSetAttribute(const void* func, hipFuncAttribute attr, int value)` -
 * Set attribute for a specific function.
 */

/**
 * Test Description
 * ------------------------
 *  - Sets maximum dynamic shared memory size to the non-default value.
 *    - Expected output: return `hipSuccess`
 * Test source
 * ------------------------
 *  - unit/executionControl/hipFuncSetAttribute.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 5.2
 */
HIP_TEST_CASE(Unit_hipFuncSetAttribute_Positive_MaxDynamicSharedMemorySize) {
  HIP_CHECK(hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                hipFuncAttributeMaxDynamicSharedMemorySize, 1024));

  hipFuncAttributes attributes;
  HIP_CHECK(hipFuncGetAttributes(&attributes, reinterpret_cast<void*>(kernel)));

  REQUIRE(attributes.maxDynamicSharedSizeBytes == 1024);
}

/**
 * Test Description
 * ------------------------
 *  - Sets preferred shared memory carveout to the non-default value.
 *    - Expected output: return `hipSuccess`
 * Test source
 * ------------------------
 *  - unit/executionControl/hipFuncSetAttribute.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 5.2
 */
HIP_TEST_CASE(Unit_hipFuncSetAttribute_Positive_PreferredSharedMemoryCarveout) {
  hipError_t result = hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                          hipFuncAttributePreferredSharedMemoryCarveout, 50);

  if (result == hipErrorNotSupported) {
    // Device doesn't support carveout - this is valid, skip test
    HIP_SKIP_TEST("hipFuncAttributePreferredSharedMemoryCarveout not supported on this device");
    return;
  }
  HIP_CHECK(result);

  hipFuncAttributes attributes;
  HIP_CHECK(hipFuncGetAttributes(&attributes, reinterpret_cast<void*>(kernel)));

  REQUIRE(attributes.preferredShmemCarveout == 50);
}

/**
 * Test Description
 * ------------------------
 *  - Validates handling of valid arguments:
 *    -# When `hipFuncAttributeMaxDynamicSharedMemorySize == 0`
 *      - Expected output: return `hipSuccess`
 *    -# When `hipFuncAttributeMaxDynamicSharedMemorySize == maxSharedMemoryPerBlock -
 * sharedSizeBytes`
 *      - Expected output: return `hipSuccess`
 *    -# When `hipFuncAttributePreferredSharedMemoryCarveout` is 0%
 *      - Expected output: return `hipSuccess`
 *    -# When `hipFuncAttributePreferredSharedMemoryCarveout` is 100%
 *      - Expected output: return `hipSuccess`
 *    -# When `hipFuncAttributePreferredSharedMemoryCarveout` is default (-1)
 *      - Expected output: return `hipSuccess`
 * Test source
 * ------------------------
 *  - unit/executionControl/hipFuncSetAttribute.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 5.2
 */
HIP_TEST_CASE(Unit_hipFuncSetAttribute_Positive_Parameters) {
  SECTION("hipFuncAttributeMaxDynamicSharedMemorySize == 0") {
    HIP_CHECK(hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                  hipFuncAttributeMaxDynamicSharedMemorySize, 0));
  }

  SECTION(
      "hipFuncAttributeMaxDynamicSharedMemorySize == maxSharedMemoryPerBlock - sharedSizeBytes") {
    // The sum of this value and the function attribute sharedSizeBytes cannot exceed the device
    // attribute cudaDevAttrMaxSharedMemoryPerBlockOptin
    int max_shared;
    HIP_CHECK(hipDeviceGetAttribute(&max_shared, hipDeviceAttributeMaxSharedMemoryPerBlock, 0));

    hipFuncAttributes attributes;
    HIP_CHECK(hipFuncGetAttributes(&attributes, reinterpret_cast<void*>(kernel)));

    HIP_CHECK(hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                  hipFuncAttributeMaxDynamicSharedMemorySize,
                                  max_shared - attributes.sharedSizeBytes));
  }

  SECTION("hipFuncAttributePreferredSharedMemoryCarveout == 0") {
    hipError_t result = hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                           hipFuncAttributePreferredSharedMemoryCarveout, 0);
    if (result == hipErrorNotSupported) {
      HIP_SKIP_TEST("hipFuncAttributePreferredSharedMemoryCarveout not supported on this device");
      return;
    }
    HIP_CHECK(result);
  }

  SECTION("hipFuncAttributePreferredSharedMemoryCarveout == 100") {
    hipError_t result = hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                           hipFuncAttributePreferredSharedMemoryCarveout, 100);
    if (result == hipErrorNotSupported) {
      HIP_SKIP_TEST("hipFuncAttributePreferredSharedMemoryCarveout not supported on this device");
      return;
    }
    HIP_CHECK(result);
  }

  SECTION("hipFuncAttributePreferredSharedMemoryCarveout == -1 (default)") {
    hipError_t result = hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                           hipFuncAttributePreferredSharedMemoryCarveout, -1);
    if (result == hipErrorNotSupported) {
      HIP_SKIP_TEST("hipFuncAttributePreferredSharedMemoryCarveout not supported on this device");
      return;
    }
    HIP_CHECK(result);
  }
}

/**
 * Test Description
 * ------------------------
 *  - Validates handling of invalid arguments:
 *    -# When pointer to the kernel function is `nullptr`
 *      - Expected output: return `hipErrorInvalidDeviceFunction`
 *    -# When the attribute is invalid
 *      - Expected output: return `hipErrorInvalidValue`
 *    -# When `hipFuncAttributeMaxDynamicSharedMemorySize < 0`
 *      - Expected output: return `hipErrorInvalidValue`
 *    -# When `hipFuncAttributeMaxDynamicSharedMemorySize > maxSharedMemoryPerBlock -
 * sharedSizeBytes`
 *      - Expected output: return `hipErrorInvalidValue`
 *    -# When `hipFuncAttributePreferredSharedMemoryCarveout` is negative
 *      - Expected output: return `hipErrorInvalidValue`
 *    -# When `hipFuncAttributePreferredSharedMemoryCarveout` is above 100%
 *      - Expected output: return `hipErrorInvalidValue`
 * Test source
 * ------------------------
 *  - unit/executionControl/hipFuncSetAttribute.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 5.2
 */
HIP_TEST_CASE(Unit_hipFuncSetAttribute_Negative_Parameters) {
  SECTION("func == nullptr") {
    HIP_CHECK_ERROR(hipFuncSetAttribute(nullptr, hipFuncAttributePreferredSharedMemoryCarveout, 50),
                    hipErrorInvalidDeviceFunction);
  }

  SECTION("invalid attribute") {
    HIP_CHECK_ERROR(
        hipFuncSetAttribute(reinterpret_cast<void*>(kernel), static_cast<hipFuncAttribute>(-1), 50),
        hipErrorInvalidValue);
  }

  SECTION("hipFuncAttributeMaxDynamicSharedMemorySize < 0") {
    HIP_CHECK_ERROR(hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                        hipFuncAttributeMaxDynamicSharedMemorySize, -1),
                    hipErrorInvalidValue);
  }

  SECTION(
      "hipFuncAttributeMaxDynamicSharedMemorySize > maxSharedMemoryPerBlock - sharedSizeBytes") {
    // The sum of this value and the function attribute sharedSizeBytes cannot exceed the device
    // attribute cudaDevAttrMaxSharedMemoryPerBlockOptin
    int max_shared;
    HIP_CHECK(hipDeviceGetAttribute(&max_shared, hipDeviceAttributeSharedMemPerBlockOptin, 0));

    hipFuncAttributes attributes;
    HIP_CHECK(hipFuncGetAttributes(&attributes, reinterpret_cast<void*>(kernel)));

    HIP_CHECK_ERROR(hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                        hipFuncAttributeMaxDynamicSharedMemorySize,
                                        max_shared - attributes.sharedSizeBytes + 1),
                    hipErrorInvalidValue);
  }

  SECTION("hipFuncAttributePreferredSharedMemoryCarveout < -1") {
    hipError_t localError = hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                        hipFuncAttributePreferredSharedMemoryCarveout, -2);
    // If feature is not supported, hipErrorNotSupported is returned before parameter validation
    // If feature is supported, hipErrorInvalidValue should be returned for invalid parameters
    REQUIRE((localError == hipErrorInvalidValue || localError == hipErrorNotSupported));
  }

  SECTION("hipFuncAttributePreferredSharedMemoryCarveout > 100") {
    hipError_t localError = hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                        hipFuncAttributePreferredSharedMemoryCarveout, 101);
    // If feature is not supported, hipErrorNotSupported is returned before parameter validation
    // If feature is supported, hipErrorInvalidValue should be returned for invalid parameters
    REQUIRE((localError == hipErrorInvalidValue || localError == hipErrorNotSupported));
  }
}

/**
 * Test Description
 * ------------------------
 *  - Sets `hipFuncAttributeMaxDynamicSharedMemorySize` to an invalid (too large) value
 *    - Expected output: return `hipErrorInvalidValue`
 * Test source
 * ------------------------
 *  - unit/executionControl/hipFuncSetAttribute.cc
 * Test requirements
 * ------------------------
 *  - Platform specific (AMD)
 *  - HIP_VERSION >= 5.2
 */
HIP_TEST_CASE(Unit_hipFuncSetAttribute_Positive_MaxDynamicSharedMemorySize_Not_Supported) {
#if HT_NVIDIA
  HIP_SKIP_TEST(HipTest::SkipReason::kApiUnsupportedOnNvidia);
#endif

  hipFuncAttributes old_attributes;
  HIP_CHECK(hipFuncGetAttributes(&old_attributes, reinterpret_cast<void*>(kernel)));

  // Get the maximum settable value for this kernel
  // hipFuncGetAttribute returns availableLDSSize - localMemSize (accounts for WGP mode)
  int max_settable_value = 0;
  HIP_CHECK(hipFuncGetAttribute(&max_settable_value,
                                 HIP_FUNC_ATTRIBUTE_MAX_DYNAMIC_SHARED_SIZE_BYTES,
                                 reinterpret_cast<hipFunction_t>(kernel)));

  // Set to a value that exceeds the maximum - guaranteed to be invalid
  int invalid_value = max_settable_value + 1;
  HIP_CHECK_ERROR(hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                      hipFuncAttributeMaxDynamicSharedMemorySize, invalid_value),
                  hipErrorInvalidValue);

  hipFuncAttributes new_attributes;
  HIP_CHECK(hipFuncGetAttributes(&new_attributes, reinterpret_cast<void*>(kernel)));

  REQUIRE(old_attributes.maxDynamicSharedSizeBytes == new_attributes.maxDynamicSharedSizeBytes);
}

/**
 * Test Description
 * ------------------------
 *  - Sets `hipFuncAttributePreferredSharedMemoryCarveout` to the non-supported value
 *    - Expected output: return `hipErrorNotSupported`
 * Test source
 * ------------------------
 *  - unit/executionControl/hipFuncSetAttribute.cc
 * Test requirements
 * ------------------------
 *  - Platform specific (AMD)
 *  - HIP_VERSION >= 5.2
 */
HIP_TEST_CASE(Unit_hipFuncSetAttribute_Positive_PreferredSharedMemoryCarveout_Not_Supported) {
#if HT_NVIDIA
  HIP_SKIP_TEST(HipTest::SkipReason::kApiUnsupportedOnNvidia);
#endif

  // Check if the feature is supported by attempting to get the attribute
  int carveout_value = 0;
  hipError_t get_result = hipFuncGetAttribute(&carveout_value,
                                               HIP_FUNC_ATTRIBUTE_PREFERRED_SHARED_MEMORY_CARVEOUT,
                                               reinterpret_cast<hipFunction_t>(kernel));
  if (get_result != hipErrorNotSupported) {
    HIP_SKIP_TEST("This test requires hardware without groupMemCarveout support");
    return;
  }

  // Verify hipFuncGetAttributes returns success with carveout = 0 on unsupported hardware
  hipFuncAttributes attributes;
  HIP_CHECK(hipFuncGetAttributes(&attributes, reinterpret_cast<void*>(kernel)));
  REQUIRE(attributes.preferredShmemCarveout == 0);

  // Verify SET also returns hipErrorNotSupported
  HIP_CHECK_ERROR(hipFuncSetAttribute(reinterpret_cast<void*>(kernel),
                                      hipFuncAttributePreferredSharedMemoryCarveout, 50),
                  hipErrorNotSupported);
}

/**
 * Test Description
 * ------------------------
 *  - Launches a kernel that uses 64 KB of LDS with hipFuncAttributePreferredSharedMemoryCarveout
 *    set to 90% and 10%.
 * Test source
 * ------------------------
 *  - unit/executionControl/hipFuncSetAttribute.cc
 * Test requirements
 * ------------------------
 *  - HIP_VERSION >= 5.2
 */
HIP_TEST_CASE(Unit_hipFuncSetAttribute_LDS64K_Carveout) {
#if HT_NVIDIA
  HIP_SKIP_TEST("This is an AMD specific test");
  return;
#endif

  // Probe whether this device supports the carveout attribute.
  hipError_t probe = hipFuncSetAttribute(reinterpret_cast<void*>(kernel_lds_64k),
                                         hipFuncAttributePreferredSharedMemoryCarveout, 50);
  if (probe == hipErrorNotSupported) {
    HIP_SKIP_TEST("hipFuncAttributePreferredSharedMemoryCarveout not supported on this device");
    return;
  }
  HIP_CHECK(probe);

  int* d_output;
  HIP_CHECK(hipMalloc(&d_output, sizeof(int)));

  SECTION("hipFuncAttributePreferredSharedMemoryCarveout==90") {
    HIP_CHECK(hipFuncSetAttribute(reinterpret_cast<void*>(kernel_lds_64k),
                                  hipFuncAttributePreferredSharedMemoryCarveout, 90));

    kernel_lds_64k<<<1, 256>>>(d_output);
    HIP_CHECK(hipGetLastError());
    HIP_CHECK(hipDeviceSynchronize());

    int result = -1;
    HIP_CHECK(hipMemcpy(&result, d_output, sizeof(int), hipMemcpyDeviceToHost));
    REQUIRE(result == 0);
  }

  // The carveout setting is a hint. The following test should still pass.
  SECTION("hipFuncAttributePreferredSharedMemoryCarveout==10") {
    HIP_CHECK(hipFuncSetAttribute(reinterpret_cast<void*>(kernel_lds_64k),
                                  hipFuncAttributePreferredSharedMemoryCarveout, 10));

    kernel_lds_64k<<<1, 256>>>(d_output);
    HIP_CHECK(hipGetLastError());
    HIP_CHECK(hipDeviceSynchronize());
  }

  HIP_CHECK(hipFree(d_output));
}

/**
 * End doxygen group ExecutionTest.
 * @}
 */
