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
 *   void myFunction() {
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

// Direct marker macros (for maximum flexibility)
#define SQTT_MARKER_ENTER(name) sqtt_marker_enter(name)
#define SQTT_MARKER_EXIT(name) sqtt_marker_exit(name)

// Scoped marker helper (RAII-style - automatically exits on scope end)
class SqttScopedMarker {
private:
  const char* name_;
public:
  explicit SqttScopedMarker(const char* name) : name_(name) {
    sqtt_marker_enter(name_);
  }
  ~SqttScopedMarker() {
    sqtt_marker_exit(name_);
  }
  // Prevent copying
  SqttScopedMarker(const SqttScopedMarker&) = delete;
  SqttScopedMarker& operator=(const SqttScopedMarker&) = delete;
};

// RAII-style scoped marker
#define SQTT_SCOPED_MARKER(name) SqttScopedMarker _sqtt_marker_##__LINE__(name)

#else

// No-op macros when SQTT is disabled (zero overhead)
#define SQTT_MARKER_ENTER(name) do {} while(0)
#define SQTT_MARKER_EXIT(name) do {} while(0)
#define SQTT_SCOPED_MARKER(name) do {} while(0)

#endif  // SQTT_ENABLED

#endif  // RCCL_SQTT_MARKERS_H_
