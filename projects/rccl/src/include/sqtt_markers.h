/*************************************************************************
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * SQTT (SQ Thread Trace) marker support for RCCL
 *
 * This header provides conditional SQTT markers that can be used
 * throughout the RCCL codebase for fine-grained performance instrumentation.
 *
 * Usage:
 *   #include "sqtt_markers.h"
 *
 *   __device__ void myDeviceFunction() {
 *     SQTT_MARKER_ENTER("myFunction");
 *     // ... function implementation ...
 *     SQTT_MARKER_EXIT("myFunction");
 *   }
 *************************************************************************/

#ifndef RCCL_SQTT_MARKERS_H_
#define RCCL_SQTT_MARKERS_H_

#if SQTT_ENABLED

// Include the SQTT marker implementation
#include </opt/rocm/include/rocprof-trace-decoder/rocprof_trace_decoder/cxx/markers.hpp>

// SQTT markers are device-only. Wrap them to be no-ops on host.
#ifdef __HIP_DEVICE_COMPILE__
  // Device code: use actual SQTT markers
  #define SQTT_MARKER_ENTER(name) sqtt_marker_enter(name)
  #define SQTT_MARKER_EXIT(name) sqtt_marker_exit(name)
  #define SQTT_MARKER_POINT(name) sqtt_marker_point(name)
  #define SQTT_MARKER_DATA(name, data) sqtt_marker_data(name, data)
#else
  // Host code: no-op (SQTT markers are device-only)
  #define SQTT_MARKER_ENTER(name) do {} while(0)
  #define SQTT_MARKER_EXIT(name) do {} while(0)
  #define SQTT_MARKER_POINT(name) do {} while(0)
  #define SQTT_MARKER_DATA(name, data) do {} while(0)
#endif

// Scoped marker helper (RAII-style - automatically exits on scope end)
// Only works in device code
#ifdef __HIP_DEVICE_COMPILE__
class SqttScopedMarker {
private:
  const char* name_;
public:
  __device__ explicit SqttScopedMarker(const char* name) : name_(name) {
    sqtt_marker_enter(name_);
  }
  __device__ ~SqttScopedMarker() {
    sqtt_marker_exit(name_);
  }
  // Prevent copying
  SqttScopedMarker(const SqttScopedMarker&) = delete;
  SqttScopedMarker& operator=(const SqttScopedMarker&) = delete;
};
#define SQTT_SCOPED_MARKER(name) SqttScopedMarker _sqtt_marker_##__LINE__(name)
#else
#define SQTT_SCOPED_MARKER(name) do {} while(0)
#endif

#else

// No-op macros when SQTT is disabled (zero overhead)
#define SQTT_MARKER_ENTER(name) do {} while(0)
#define SQTT_MARKER_EXIT(name) do {} while(0)
#define SQTT_MARKER_POINT(name) do {} while(0)
#define SQTT_MARKER_DATA(name, data) do {} while(0)
#define SQTT_SCOPED_MARKER(name) do {} while(0)

#endif  // SQTT_ENABLED

#endif  // RCCL_SQTT_MARKERS_H_
