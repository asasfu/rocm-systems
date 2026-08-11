# RCCL SQTT Marker Profiler Plugin

This plugin adds SQTT (SQ Thread Trace) instrumentation markers to RCCL operations for GPU performance analysis and profiling.

## Overview

The SQTT marker plugin integrates with the NCCL profiler API to automatically instrument RCCL operations with SQTT markers. This enables detailed GPU thread trace analysis using ROCm profiling tools.

## Requirements

- ROCm >= 7.13
- SQTT instrumentation support in ROCm (`libsqttinstrumentpass.so`)

## Building

### Standalone Build

```bash
cd plugins/profiler/sqttmarker
cmake -S . -B build
cmake --build build
```

### As Part of RCCL Build

The plugin is automatically built when included in the main RCCL build:

```bash
# In main RCCL directory
./install.sh --sqtt-enable
```

Or with CMake directly:

```bash
cmake -DSQTT_ENABLED=ON ..
```

## Usage

### Loading the Plugin

Set the `NCCL_PROFILER_PLUGIN` environment variable to load the plugin:

```bash
export NCCL_PROFILER_PLUGIN=/path/to/librccl-profiler-sqttmarker.so
```

### Running Applications

Simply run your RCCL application with the plugin loaded:

```bash
export NCCL_PROFILER_PLUGIN=lib/librccl-profiler-sqttmarker.so
./my_rccl_application
```

### Collecting Traces

Use ROCm profiling tools to collect SQTT traces:

```bash
# Example with rocprof
rocprof --sqtt ./my_rccl_application
```

## Instrumented Operations

The plugin adds SQTT markers for:

- **Collective Operations**: AllReduce, Broadcast, Reduce, etc.
- **Point-to-Point Operations**: Send/Recv
- **Proxy Operations**: Network proxy send/recv
- **Group Operations**: Group start/end
- **Kernel Launches**: GPU kernel execution
- **CE Operations**: Compute Engine operations (v6)

## Marker Naming Convention

Markers follow this naming pattern:

- `RCCL_Coll_<type>` - Collective operations
- `RCCL_P2P_Send/Recv` - P2P operations
- `RCCL_Proxy_Send/Recv_Chan<N>` - Proxy operations with channel ID
- `RCCL_Kernel` - Kernel launches
- `RCCL_CE_Coll/Sync/Batch` - CE operations

## Performance Impact

When SQTT tracing is not active, the markers have minimal overhead as they compile to no-ops. When tracing is active, there is small overhead from marker instrumentation.

## Troubleshooting

### Plugin Not Loading

Check that:
1. ROCm version is >= 7.13
2. `libsqttinstrumentpass.so` exists in `/opt/rocm/lib/`
3. Plugin library path is correct in `NCCL_PROFILER_PLUGIN`

### No Markers in Trace

Ensure:
1. SQTT tracing is enabled in the profiling tool
2. Plugin is actually loaded (check RCCL output for initialization message)
3. Application is running RCCL operations

## License

See LICENSE.txt in the RCCL root directory.
