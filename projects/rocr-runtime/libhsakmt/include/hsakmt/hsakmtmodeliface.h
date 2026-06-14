/*
 * Copyright © 2025 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including
 * the next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _HSAKMTMODELIFACE_H_
#define _HSAKMTMODELIFACE_H_

#include <inttypes.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#ifndef _NTDEF_
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
#endif
#include <d3dkmthk.h>
#endif // _WIN32

// Changelog:
//  1.0: Breaking ABI cleanup: remove unused entry points (create/destroy/
//       set_global_aperture/init_from_topology). Model interface is now
//       create_memfd + handle_ioctl.
//  1.1: Add handle_drm_call for DRM/amdgpu simulation. Old clients (1.0)
//       that lack this field will have no-op success for all DRM calls.
#define HSAKMT_MODEL_INTERFACE_VERSION_MAJOR 1
#define HSAKMT_MODEL_INTERFACE_VERSION_MINOR 1

typedef struct hsakmt_model hsakmt_model_t;
typedef struct hsakmt_model_queue hsakmt_model_queue_t;

// Pointer to a "set event" function.
//
// data is a user-provided opaque pointer.
// event_id is the ID of the event to set (as in amd_signal_s::event_id).
typedef void (*hsakmt_model_set_event_fn)(void *data, unsigned event_id);

// Interface provided by the software model implementation.
//
// Queried from a shared library by calling an export called
// `get_hsakmt_model_functions`
//
// Interface versioning follows the semantic versioning model: clients that
// know about interface version X.Y can use any implementation that provides
// version X.Z with Z >= Y.
//
// The model is designed to support only one VMID space.
struct hsakmt_model_functions {
	uint32_t version_major; // HSAKMT_MODEL_INTERFACE_VERSION_MAJOR
	uint32_t version_minor; // HSAKMT_MODEL_INTERFACE_VERSION_MINOR

#ifdef __linux__
	// Create memfd for model use (v0.7+)
	// FFM owns memfd creation and sizing logic
	// Returns: File descriptor on success, -1 on error (errno set)
	int (*create_memfd)(void);

	// Unified IOCTL handler - FFM owns all IOCTL dispatch logic
	// Returns 0 on success, -1 on error (with errno set)
	int (*handle_ioctl)(unsigned long request, void *arg);

	// DRM/amdgpu call simulation (v1.1+)
	// Routes libdrm/amdgpu API calls through the model.
	// NULL in v1.0 models — callers must degrade gracefully.
	// Returns 0 on success, -1 on error (with errno set)
	int (*handle_drm_call)(unsigned cmd, void *arg);
#elif defined(_WIN32)
#define EVAL(...) __VA_ARGS__
#define HSAKMTSIM_D3DKMT_DO_ALL_EXCEPT_SHARE_OBJECTS(F) \
	EVAL(F(D3DKMTCreateAllocation2, D3DKMT_CREATEALLOCATION* args)) \
	EVAL(F(D3DKMTDestroyAllocation2, CONST D3DKMT_DESTROYALLOCATION2* args)) \
	EVAL(F(D3DKMTMapGpuVirtualAddress, D3DDDI_MAPGPUVIRTUALADDRESS* args)) \
	EVAL(F(D3DKMTReserveGpuVirtualAddress, D3DDDI_RESERVEGPUVIRTUALADDRESS* args)) \
	EVAL(F(D3DKMTFreeGpuVirtualAddress, CONST D3DKMT_FREEGPUVIRTUALADDRESS* args)) \
	EVAL(F(D3DKMTCreateDevice, D3DKMT_CREATEDEVICE* args)) \
	EVAL(F(D3DKMTDestroyDevice, CONST D3DKMT_DESTROYDEVICE* args)) \
	EVAL(F(D3DKMTEnumAdapters2, CONST D3DKMT_ENUMADAPTERS2* args)) \
	EVAL(F(D3DKMTQueryAdapterInfo, CONST D3DKMT_QUERYADAPTERINFO* args)) \
	EVAL(F(D3DKMTCreateContextVirtual, D3DKMT_CREATECONTEXTVIRTUAL* args)) \
	EVAL(F(D3DKMTDestroyContext, CONST D3DKMT_DESTROYCONTEXT* args)) \
	EVAL(F(D3DKMTSubmitCommand, CONST D3DKMT_SUBMITCOMMAND* args)) \
	EVAL(F(D3DKMTCreateSynchronizationObject2, D3DKMT_CREATESYNCHRONIZATIONOBJECT2* args)) \
	EVAL(F(D3DKMTDestroySynchronizationObject, CONST D3DKMT_DESTROYSYNCHRONIZATIONOBJECT* args)) \
	EVAL(F(D3DKMTQueryStatistics, CONST D3DKMT_QUERYSTATISTICS* args)) \
	EVAL(F(D3DKMTEscape, CONST D3DKMT_ESCAPE* args)) \
	EVAL(F(D3DKMTLock2, D3DKMT_LOCK2* args)) \
	EVAL(F(D3DKMTUnlock2, CONST D3DKMT_UNLOCK2* args)) \
	EVAL(F(D3DKMTCreatePagingQueue, D3DKMT_CREATEPAGINGQUEUE* args)) \
	EVAL(F(D3DKMTDestroyPagingQueue, D3DDDI_DESTROYPAGINGQUEUE* args)) \
	EVAL(F(D3DKMTWaitForSynchronizationObjectFromGpu, CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMGPU* args)) \
	EVAL(F(D3DKMTSignalSynchronizationObjectFromGpu, CONST D3DKMT_SIGNALSYNCHRONIZATIONOBJECTFROMGPU* args)) \
	EVAL(F(D3DKMTWaitForSynchronizationObjectFromCpu, CONST D3DKMT_WAITFORSYNCHRONIZATIONOBJECTFROMCPU* args)) \
	EVAL(F(D3DKMTQueryClockCalibration, D3DKMT_QUERYCLOCKCALIBRATION* args)) \
	EVAL(F(D3DKMTMakeResident, D3DDDI_MAKERESIDENT* args)) \
	EVAL(F(D3DKMTEvict, D3DKMT_EVICT* args)) \
	EVAL(F(D3DKMTQueryResourceInfoFromNtHandle, D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE* args)) \
	EVAL(F(D3DKMTOpenResourceFromNtHandle, D3DKMT_OPENRESOURCEFROMNTHANDLE* args)) \
	EVAL(F(D3DKMTCreateHwQueue, D3DKMT_CREATEHWQUEUE* args)) \
	EVAL(F(D3DKMTDestroyHwQueue, CONST D3DKMT_DESTROYHWQUEUE* args)) \
	EVAL(F(D3DKMTSubmitCommandToHwQueue, CONST D3DKMT_SUBMITCOMMANDTOHWQUEUE* args)) \
	EVAL(F(D3DKMTEnumAdapters3, D3DKMT_ENUMADAPTERS3* args)) \
	EVAL(F(D3DKMTQueryResourceInfo, D3DKMT_QUERYRESOURCEINFO* args)) \
	EVAL(F(D3DKMTOpenResource, D3DKMT_OPENRESOURCE* args))
#define HSAKMTSIM_D3DKMT_DO(F) \
	HSAKMTSIM_D3DKMT_DO_ALL_EXCEPT_SHARE_OBJECTS(F) \
	EVAL(F(D3DKMTShareObjects, \
		UINT					cObjects, \
		CONST D3DKMT_HANDLE*	hObjects, \
		OBJECT_ATTRIBUTES*		pObjectAttributes, \
		DWORD					dwDesiredAccess, \
		HANDLE*					phSharedNtHandle))

#define DECLARE_MEMBER_FUNCTION(name, ...) \
	NTSTATUS(*name)(__VA_ARGS__);
HSAKMTSIM_D3DKMT_DO(DECLARE_MEMBER_FUNCTION)
#undef DECLARE_MEMBER_FUNCTION
#endif // _WIN32
};

#ifdef __linux__
// Commands for handle_drm_call (v1.1+)
enum hsakmt_drm_cmd {
	// BO operations
	HSAKMT_DRM_BO_VA_OP,
	HSAKMT_DRM_BO_FREE,
	HSAKMT_DRM_BO_IMPORT,
	HSAKMT_DRM_BO_EXPORT,
	HSAKMT_DRM_BO_CPU_MAP,
	HSAKMT_DRM_BO_QUERY_INFO,
	HSAKMT_DRM_BO_SET_METADATA,
	HSAKMT_DRM_COMMAND_WRITE_READ,
	// Device-level operations
	HSAKMT_DRM_OPEN_RENDER,
	HSAKMT_DRM_CLOSE,
	HSAKMT_DRM_DEVICE_INITIALIZE,
	HSAKMT_DRM_DEVICE_DEINITIALIZE,
	HSAKMT_DRM_DEVICE_GET_FD,
	HSAKMT_DRM_GET_MARKETING_NAME,
	HSAKMT_DRM_QUERY_GPU_INFO,
};

// Arg structs for each hsakmt_drm_cmd. void *bo / void *dev are opaque
// handles whose meaning is private to the model implementation.
struct hsakmt_drm_bo_va_op_args {
	void    *bo;
	uint64_t offset;
	uint64_t size;
	uint64_t addr;
	uint64_t flags;
	uint32_t ops;
};
struct hsakmt_drm_bo_free_args         { void *bo; };
struct hsakmt_drm_bo_import_args       { int fd; uint32_t type; void *result_out; }; // amdgpu_bo_import_result*
struct hsakmt_drm_bo_export_args       { void *bo; uint32_t type; uint32_t *handle_out; };
struct hsakmt_drm_bo_cpu_map_args      { void *bo; void **cpu_ptr_out; };
struct hsakmt_drm_bo_query_info_args   { void *bo; void *info_out; };   // amdgpu_bo_info*
struct hsakmt_drm_bo_set_metadata_args { void *bo; void *metadata; };   // amdgpu_bo_metadata*
struct hsakmt_drm_cmd_write_read_args  { int fd; unsigned long cmd; void *data; unsigned size; };
struct hsakmt_drm_open_render_args     { int minor; int *fd_out; };
struct hsakmt_drm_close_args           { int fd; };
struct hsakmt_drm_device_initialize_args {
	int       fd;
	uint32_t *major_out;
	uint32_t *minor_out;
	void    **dev_out;   // amdgpu_device_handle*
};
struct hsakmt_drm_device_deinitialize_args { void *dev; };
struct hsakmt_drm_device_get_fd_args       { void *dev; int *fd_out; };
struct hsakmt_drm_get_marketing_name_args  { void *dev; const char **name_out; };
struct hsakmt_drm_query_gpu_info_args      { void *dev; void *info_out; }; // amdgpu_gpu_info*
#endif // __linux__

// Type of a shared library export called `get_hsakmt_model_functions`.
typedef const struct hsakmt_model_functions *(*get_hsakmt_model_functions_t)(void);

#endif // _HSAKMTMODELIFACE_H_
