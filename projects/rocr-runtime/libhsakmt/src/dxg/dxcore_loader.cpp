/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "dxcore_loader.h"
#include "librocdxg.h"
#include "hsakmt/hsakmtmodeliface.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ntstatus.h>
#include <util/os.h>

extern bool hsakmt_use_model;

namespace wsl {
namespace thunk {
namespace dxcore {

DxcoreLoader::DxcoreLoader()
    : library_handle_(nullptr)
    , init_flag_()
    , pfn_D3DKMTCreateAllocation2(nullptr)
    , pfn_D3DKMTDestroyAllocation2(nullptr)
    , pfn_D3DKMTMapGpuVirtualAddress(nullptr)
    , pfn_D3DKMTReserveGpuVirtualAddress(nullptr)
    , pfn_D3DKMTFreeGpuVirtualAddress(nullptr)
    , pfn_D3DKMTCreateDevice(nullptr)
    , pfn_D3DKMTDestroyDevice(nullptr)
    , pfn_D3DKMTEnumAdapters2(nullptr)
    , pfn_D3DKMTQueryAdapterInfo(nullptr)
    , pfn_D3DKMTCreateContextVirtual(nullptr)
    , pfn_D3DKMTDestroyContext(nullptr)
    , pfn_D3DKMTSubmitCommand(nullptr)
    , pfn_D3DKMTCreateSynchronizationObject2(nullptr)
    , pfn_D3DKMTDestroySynchronizationObject(nullptr)
    , pfn_D3DKMTQueryStatistics(nullptr)
    , pfn_D3DKMTEscape(nullptr)
    , pfn_D3DKMTLock2(nullptr)
    , pfn_D3DKMTUnlock2(nullptr)
    , pfn_D3DKMTCreatePagingQueue(nullptr)
    , pfn_D3DKMTDestroyPagingQueue(nullptr)
    , pfn_D3DKMTWaitForSynchronizationObjectFromGpu(nullptr)
    , pfn_D3DKMTSignalSynchronizationObjectFromGpu(nullptr)
    , pfn_D3DKMTWaitForSynchronizationObjectFromCpu(nullptr)
    , pfn_D3DKMTQueryClockCalibration(nullptr)
    , pfn_D3DKMTMakeResident(nullptr)
    , pfn_D3DKMTEvict(nullptr)
    , pfn_D3DKMTShareObjects(nullptr)
    , pfn_D3DKMTQueryResourceInfoFromNtHandle(nullptr)
    , pfn_D3DKMTOpenResourceFromNtHandle(nullptr)
    , pfn_D3DKMTOpenSyncObjectFromNtHandle2(nullptr)
    , pfn_D3DKMTCreateHwQueue(nullptr)
    , pfn_D3DKMTDestroyHwQueue(nullptr)
    , pfn_D3DKMTSubmitCommandToHwQueue(nullptr)
    , pfn_D3DKMTEnumAdapters3(nullptr)
    , pfn_D3DKMTQueryResourceInfo(nullptr)
    , pfn_D3DKMTOpenResource(nullptr) {
}

DxcoreLoader::~DxcoreLoader() { Shutdown(); }

bool DxcoreLoader::Initialize() {
    std::ignore = rocr::os::DlError(); // Clear error

#if defined(_WIN32)
    /* Load model library first to get interface functions */
    if (!rocr::os::GetEnvVar("HSA_MODEL_TOPOLOGY").empty()) {
        return LoadModelApis();
    }
#endif  // defined(_WIN32)

#if defined(__linux__)
    library_name_ = "libdxcore.so";
#else
    library_name_ = "Gdi32.dll";
#endif
    library_handle_ = rocr::os::LoadLib(library_name_.c_str());
    if (!library_handle_) {
        pr_err("[DxcoreLoader] Cannot load %s: %s\n", library_name_.c_str(), rocr::os::DlError());
        return false;
    }

    pr_info("[DxcoreLoader] %s loaded successfully\n", library_name_.c_str());
    if (!LoadDxcoreApis()) {
        // If API loading failed, close the handle to indicate failure
        rocr::os::CloseLib(library_handle_);
        library_handle_ = nullptr;
        return false;
    }

    return IsLoaded();
}

void DxcoreLoader::Shutdown() {
    if (library_handle_) {
        if (rocr::os::CloseLib(library_handle_) != 0) {
            pr_err("[DxcoreLoader] Cannot unload %s: %s\n", library_name_.c_str(), rocr::os::DlError());
        } else {
            pr_info("[DxcoreLoader] %s unloaded successfully\n", library_name_.c_str());
        }
        library_handle_ = nullptr;
        library_name_.clear();
    }
}

#if defined(_WIN32)
bool DxcoreLoader::LoadModelApis() {
    library_name_ = rocr::os::GetEnvVar("HSA_MODEL_LIB");
    if (library_name_.empty()) {
        pr_err("model: HSA_MODEL_LIB environment variable must be set to the FFM model library\n");
        abort();
    }

    library_handle_ = rocr::os::LoadLib(library_name_);
    if (!library_handle_) {
        pr_err("[DxcoreLoader] Cannot load %s: %s\n", library_name_.c_str(), rocr::os::DlError());
        abort();
    }

    auto getter = static_cast<get_hsakmt_model_functions_t>(
        rocr::os::GetExportAddress(library_handle_, "get_hsakmt_model_functions"));
    if (!getter) {
        pr_err("model: Failed to get hsakmt_model_functions\n");
        return false;
    }

    auto model_functions = getter();
    constexpr auto expected_version_major = HSAKMT_MODEL_INTERFACE_VERSION_MAJOR;
    constexpr auto expected_version_minor = HSAKMT_MODEL_INTERFACE_VERSION_MINOR;

    if (model_functions->version_major != expected_version_major ||
        model_functions->version_minor < expected_version_minor) {
        pr_err("[MODEL] FATAL: Major version mismatch (breaking API change)!\n");
        pr_err("[MODEL]   Model file: %s\n", library_name_.c_str());
        pr_err("[MODEL]   Model version: %u.%u, expected major %u\n", model_functions->version_major,
               model_functions->version_minor, expected_version_major);
        return false;
    }

// Load all D3DKMT functions
#define LOAD_MODEL_API(func_name)                                                                  \
    DXCORE_PFN(func_name) = reinterpret_cast<DXCORE_DEF(func_name)*>(model_functions->func_name);

    LOAD_MODEL_API(D3DKMTCreateAllocation2);
    LOAD_MODEL_API(D3DKMTDestroyAllocation2);
    LOAD_MODEL_API(D3DKMTMapGpuVirtualAddress);
    LOAD_MODEL_API(D3DKMTReserveGpuVirtualAddress);
    LOAD_MODEL_API(D3DKMTFreeGpuVirtualAddress);
    LOAD_MODEL_API(D3DKMTCreateDevice);
    LOAD_MODEL_API(D3DKMTDestroyDevice);
    LOAD_MODEL_API(D3DKMTEnumAdapters2);
    LOAD_MODEL_API(D3DKMTEnumAdapters3);
    LOAD_MODEL_API(D3DKMTQueryAdapterInfo);
    LOAD_MODEL_API(D3DKMTCreateContextVirtual);
    LOAD_MODEL_API(D3DKMTDestroyContext);
    LOAD_MODEL_API(D3DKMTSubmitCommand);
    LOAD_MODEL_API(D3DKMTCreateSynchronizationObject2);
    LOAD_MODEL_API(D3DKMTDestroySynchronizationObject);
    LOAD_MODEL_API(D3DKMTQueryStatistics);
    LOAD_MODEL_API(D3DKMTEscape);
    LOAD_MODEL_API(D3DKMTLock2);
    LOAD_MODEL_API(D3DKMTUnlock2);
    LOAD_MODEL_API(D3DKMTCreatePagingQueue);
    LOAD_MODEL_API(D3DKMTDestroyPagingQueue);
    LOAD_MODEL_API(D3DKMTWaitForSynchronizationObjectFromGpu);
    LOAD_MODEL_API(D3DKMTSignalSynchronizationObjectFromGpu);
    LOAD_MODEL_API(D3DKMTWaitForSynchronizationObjectFromCpu);
    LOAD_MODEL_API(D3DKMTQueryClockCalibration);
    LOAD_MODEL_API(D3DKMTMakeResident);
    LOAD_MODEL_API(D3DKMTEvict);
    LOAD_MODEL_API(D3DKMTShareObjects);
    LOAD_MODEL_API(D3DKMTQueryResourceInfoFromNtHandle);
    LOAD_MODEL_API(D3DKMTQueryResourceInfo);
    LOAD_MODEL_API(D3DKMTOpenResourceFromNtHandle);
    LOAD_MODEL_API(D3DKMTOpenResource);
    LOAD_MODEL_API(D3DKMTCreateHwQueue);
    LOAD_MODEL_API(D3DKMTDestroyHwQueue);
    LOAD_MODEL_API(D3DKMTSubmitCommandToHwQueue);

#undef LOAD_MODEL_API

    hsakmt_use_model = true;

    pr_info("[DxcoreLoader] All D3DKMT Model APIs loaded successfully\n");
    return true;
}
#endif  // _WIN32

bool DxcoreLoader::LoadDxcoreApis() {
    if (!library_handle_) {
        pr_err("[DxcoreLoader] Error: library_handle_ is null\n");
        return false;
    }

    std::ignore = rocr::os::DlError(); // Clear error

    // Load all D3DKMT functions
    #define LOAD_DXCORE_API(func_name) \
        DXCORE_PFN(func_name) = (DXCORE_DEF(func_name)*)rocr::os::GetExportAddress(library_handle_, #func_name); \
        if (!DXCORE_PFN(func_name)) { \
            pr_err("[DxcoreLoader] Failed to load " #func_name ": %s\n", rocr::os::DlError()); \
            goto ERROR_LOAD; \
        }

    LOAD_DXCORE_API(D3DKMTCreateAllocation2);
    LOAD_DXCORE_API(D3DKMTDestroyAllocation2);
    LOAD_DXCORE_API(D3DKMTMapGpuVirtualAddress);
    LOAD_DXCORE_API(D3DKMTReserveGpuVirtualAddress);
    LOAD_DXCORE_API(D3DKMTFreeGpuVirtualAddress);
    LOAD_DXCORE_API(D3DKMTCreateDevice);
    LOAD_DXCORE_API(D3DKMTDestroyDevice);
    LOAD_DXCORE_API(D3DKMTEnumAdapters2);
    LOAD_DXCORE_API(D3DKMTEnumAdapters3);
    LOAD_DXCORE_API(D3DKMTQueryAdapterInfo);
    LOAD_DXCORE_API(D3DKMTCreateContextVirtual);
    LOAD_DXCORE_API(D3DKMTDestroyContext);
    LOAD_DXCORE_API(D3DKMTSubmitCommand);
    LOAD_DXCORE_API(D3DKMTCreateSynchronizationObject2);
    LOAD_DXCORE_API(D3DKMTDestroySynchronizationObject);
    LOAD_DXCORE_API(D3DKMTQueryStatistics);
    LOAD_DXCORE_API(D3DKMTEscape);
    LOAD_DXCORE_API(D3DKMTLock2);
    LOAD_DXCORE_API(D3DKMTUnlock2);
    LOAD_DXCORE_API(D3DKMTCreatePagingQueue);
    LOAD_DXCORE_API(D3DKMTDestroyPagingQueue);
    LOAD_DXCORE_API(D3DKMTWaitForSynchronizationObjectFromGpu);
    LOAD_DXCORE_API(D3DKMTSignalSynchronizationObjectFromGpu);
    LOAD_DXCORE_API(D3DKMTWaitForSynchronizationObjectFromCpu);
    LOAD_DXCORE_API(D3DKMTQueryClockCalibration);
    LOAD_DXCORE_API(D3DKMTMakeResident);
    LOAD_DXCORE_API(D3DKMTEvict);
    LOAD_DXCORE_API(D3DKMTShareObjects);
    LOAD_DXCORE_API(D3DKMTQueryResourceInfoFromNtHandle);
    LOAD_DXCORE_API(D3DKMTQueryResourceInfo);
    LOAD_DXCORE_API(D3DKMTOpenResourceFromNtHandle);
    LOAD_DXCORE_API(D3DKMTOpenSyncObjectFromNtHandle2);
    LOAD_DXCORE_API(D3DKMTOpenResource);
    LOAD_DXCORE_API(D3DKMTCreateHwQueue);
    LOAD_DXCORE_API(D3DKMTDestroyHwQueue);
    LOAD_DXCORE_API(D3DKMTSubmitCommandToHwQueue);

    #undef LOAD_DXCORE_API

    pr_info("[DxcoreLoader] All DXCore APIs loaded successfully\n");
    return true;
ERROR_LOAD:
    pr_err("[DxcoreLoader] Failed to load DXCore APIs\n");
    return false;
}

} // namespace dxcore
} // namespace thunk
} // namespace wsl
