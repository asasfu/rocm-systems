# YAML Tag and Disabled Values Summary

## Per-File Breakdown

### g++.yaml
- **Section:** `g++`
- **Tags:** `compiler`
- **Disabled:** (none)

### gcc.yaml
- **Section:** `gcc`
- **Tags:** `compiler`
- **Disabled:** (none)

### gl_interop.yaml
- **Section:** `gl_interop`
- **Tags:** `gl`, `interop`
- **Disabled:** `amd_wsl`, `amd_linux`

### graph.yaml
- **Section:** `graph`
- **Tags:** `node`, `dependency`, `lifecycle`, `exec`, `multigpu`, `capture`, `query`, `userobj`
- **Disabled:** `amd_windows`, `amd_wsl`, `amd_linux`, `nvidia_windows`

### hip_specific.yaml
- **Section:** `hip_specific`
- **Tags:** `device_lib`, `hip_ext`
- **Disabled:** (none)

### kernel.yaml
- **Section:** `kernel`
- **Tags:** `memory`, `attributes`, `language`, `launch`, `multigpu`
- **Disabled:** `amd_windows`, `amd_wsl`

### launchBounds.yaml
- **Section:** `launchBounds`
- **Tags:** `attributes`, `kernel`
- **Disabled:** `amd_windows`

### library.yaml
- **Section:** `library`
- **Tags:** `module`
- **Disabled:** (none)

### math.yaml
- **Section:** `math`
- **Tags:** `device_lib`
- **Disabled:** `amd_linux`, `nvidia_linux`, `amd_windows`, `nvidia_linux`

### memory.yaml
- **Section:** `memory`
- **Tags:** `memset`, `transfer`, `multigpu`, `hipMemcpy3D`, `hipMemcpyParam2D`, `hipMemcpyParam2DAsync`, `hipMemcpyAtoH`, `hipMemcpyHtoA`, `alloc`, `query`, `vmm`, `stream`, `hipMallocPitch`
- **Disabled:** `amd_wsl`, `nvidia_windows`, `amd_windows`, `amd_linux`

### module.yaml
- **Section:** `module`
- **Tags:** `launch`, `load`, `function`, `resource`, `multigpu`
- **Disabled:** `amd_windows`, `nvidia_windows`, `nvidia_linux`, `amd_linux`, `amd_wsl`

### multiThread.yaml
- **Section:** `multiThread`
- **Tags:** `memory`, `multithread`, `device_mgmt`, `stream`
- **Disabled:** `amd_linux`, `amd_windows`, `amd_wsl`

### occupancy.yaml
- **Section:** `occupancy`
- **Tags:** `multigpu`
- **Disabled:** `amd_windows`, `amd_wsl`

### p2p.yaml
- **Section:** `p2p`
- **Tags:** `peer`
- **Disabled:** (none)

### printf.yaml
- **Section:** `printf`
- **Tags:** `multigpu`
- **Disabled:** `amd_windows`

### rtc.yaml
- **Section:** `rtc`
- **Tags:** `compile`, `integration`, `link`, `header`
- **Disabled:** `amd_windows`, `amd_linux`, `amd_wsl`

---

## Summary: ALL Unique Tag Values (across all 16 files)

| Tag | Files Using It |
|-----|----------------|
| `alloc` | memory |
| `attributes` | kernel, launchBounds |
| `capture` | graph |
| `compiler` | g++, gcc |
| `dependency` | graph |
| `device_lib` | hip_specific, math |
| `device_mgmt` | multiThread |
| `exec` | graph |
| `function` | module |
| `gl` | gl_interop |
| `header` | rtc |
| `hip_ext` | hip_specific |
| `hipMallocPitch` | memory |
| `hipMemcpy3D` | memory |
| `hipMemcpy3DAsync` | memory |
| `hipMemcpyAtoH` | memory |
| `hipMemcpyHtoA` | memory |
| `hipMemcpyParam2D` | memory |
| `hipMemcpyParam2DAsync` | memory |
| `integration` | rtc |
| `interop` | gl_interop |
| `kernel` | launchBounds |
| `language` | kernel |
| `launch` | kernel, module |
| `lifecycle` | graph |
| `link` | rtc |
| `load` | module |
| `memory` | kernel, multiThread |
| `memset` | memory |
| `module` | library |
| `multigpu` | graph, kernel, memory, module, occupancy, printf |
| `multithread` | multiThread |
| `node` | graph |
| `peer` | p2p |
| `query` | graph, memory |
| `resource` | module |
| `stream` | memory, multiThread |
| `transfer` | memory |
| `userobj` | graph |
| `vmm` | memory |

**Total unique tags: 36**

---

## Summary: ALL Unique Disabled Values (across all 16 files)

| Disabled Value | Files Using It |
|----------------|----------------|
| `amd_linux` | gl_interop, graph, math, memory, module, multiThread, rtc |
| `amd_windows` | graph, kernel, launchBounds, math, memory, module, multiThread, occupancy, printf, rtc |
| `amd_wsl` | gl_interop, graph, kernel, memory, module, multiThread, occupancy, rtc |
| `nvidia_linux` | math, module |
| `nvidia_windows` | graph, memory, module |

**Total unique disabled values: 5**
- `amd_linux`
- `amd_windows`
- `amd_wsl`
- `nvidia_linux`
- `nvidia_windows`
