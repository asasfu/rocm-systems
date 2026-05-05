/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palLib.h
 * @brief Defines the Platform Abstraction Library (PAL) initialization and destruction functions.
 ***********************************************************************************************************************
 */

#pragma once

#include "pal.h"
#include "palSysMemory.h"
#include "palDbgPrint.h"

#if PAL_CLIENT_DX
typedef struct _D3DDDI_ADAPTERCALLBACKS DxRtAdapterCallbacks;
#endif

/* PAL_INTERFACE_MAJOR_VERSION and the version table have moved to inc/util/palVersion.h!

 # The below fake define is a hack to make LLPC work. LLPC's cmake code directly scrapes palLib.h's definition of
 # PAL_INTERFACE_MAJOR_VERSION which is a problem because that cmake code won't run the C preprocessor before doing the
 # scraping. That means it's impossible for PAL to safely deprecate anything that any client scrapes in this way. Thus
 # it must be illegal for any client to do this and they must instead add cmake function to PAL which implement whatever
 # functionality they need. We already have the pal_get_current_pal_interface_major_version just for this. Anyway,
 # this hack has to stay here until 26.10 is the oldest branch PAL supports.
#define PAL_INTERFACE_MAJOR_VERSION 974
*/

namespace Pal
{

// Forward declarations
class      IPlatform;

/// This is a list of GPUs that the NULL OS layer can compile shaders to in offline mode.
enum class NullGpuId : uint32
{
    Default = 0,   ///< PAL gives the client an arbitrary supported null device.
    Navi10,        ///< 10.1.0
    Navi12,        ///< 10.1.1
    Navi14,        ///< 10.1.2
#if PAL_BUILD_NAVI21_LITE
    Navi21Lite,    ///< 10.2.0 but we treat it as 10.3.0
#endif
    Navi21,        ///< 10.3.0
    Navi22,        ///< 10.3.1
    Navi23,        ///< 10.3.2
    Navi24,        ///< 10.3.4
#if PAL_BUILD_VAN_GOGH
    VanGogh,       ///< 10.3.3
#endif
    Rembrandt,     ///< 10.3.5
    Raphael,       ///< 10.3.6
    Navi31,        ///< 11.0.0
    Navi32,        ///< 11.0.1
    Navi33,        ///< 11.0.2
    Phoenix1,      ///< 11.0.3
    Phoenix2,      ///< 11.0.3
    Strix1,        ///< 11.5.0
#if PAL_BUILD_GORGON_POINT1
    GorgonPoint1,  ///< 11.5.0
#endif
    StrixHalo,     ///< 11.5.1
    Krackan1,      ///< 11.5.2
#if PAL_BUILD_GORGON_POINT2
    GorgonPoint2,  ///< 11.5.2
#endif
    Krackan2,      ///< 11.5.3
#if PAL_BUILD_MEDUSA1
#if PAL_CLOSED_SOURCE
    Medusa1_A0,    ///< 11.5.FFFE
#endif
    Medusa1,       ///< 11.7.0
#endif
#if PAL_BUILD_MEDUSA2
    Medusa2,       ///< 11.7.1
#endif
#if PAL_BUILD_MEDUSA3
    Medusa3,       ///< 11.7.2
#endif
#if PAL_BUILD_GAINSBOROUGH
    Gainsborough,  ///< 11.5.FFFC
#endif
    Navi44,        ///< 12.0.0
    Navi48,        ///< 12.0.1
#if PAL_CLIENT_INTERFACE_MAJOR_VERSION < 958
#if PAL_BUILD_ALPHA_TRION1
    AlphaTrion1,   ///< 13.0.1
#endif
#endif  // PAL_CLIENT_INTERFACE_MAJOR_VERSION < 958
#if PAL_BUILD_ALPHA_TRION2
    AlphaTrion2,   ///< 13.1.0
#endif
#if PAL_BUILD_AT_LITE3
    AtLite3,       ///< 13.1.65535
#endif
#if PAL_BUILD_GRIMLOCK1
    Grimlock1,     ///< 13.7.0
#endif
    Max,           ///< The maximum count of null devices.
    All,           ///< If you want to enumerate all null devices.
};

/// Specifies which graphics IP level (GFXIP) this device has.
enum class GfxIpLevel : uint32
{
    _None = 0,     ///< @internal The device does not have an GFXIP block, or its level cannot be determined

    // Unfortunately for Linux clients, X.h includes a "#define None 0" macro.  Clients have their choice of either
    // undefing None before including this header or using _None when dealing with PAL.
#ifndef None
    None  = _None, ///< The device does not have an GFXIP block, or its level cannot be determined
#endif
    GfxIp10_1,     ///< GFXIP 10.1 (Navi1x)
    GfxIp10_3,     ///< GFXIP 10.3 (Navi2x, Rembrandt, Raphael, Mendocino)
    GfxIp11_0,     ///< GFXIP 11.0 (Navi3x, Phoenix)
    GfxIp11_5,     ///< GFXIP 11.5 (Strix)
#if (PAL_BUILD_MEDUSA1 || PAL_BUILD_MEDUSA2 || PAL_BUILD_MEDUSA3)
    GfxIp11_7,     ///< GFXIP 11.7 (Medusa)
#endif
    GfxIp12,       ///< GFXIP 12.0 (Navi4x)
#if PAL_BUILD_GFX13
    GfxIp13,       ///< GFXIP 13.0 (AT)
#if PAL_BUILD_ALPHA_TRION2
    GfxIp13_1,     ///< GFXIP 13.1 (AT)
#endif
#if PAL_BUILD_GRIMLOCK1
    GfxIp13_7,     ///< GFXIP 13.7 (GLK, AT)
#endif
#endif // PAL_BUILD_GFX13
};

/// Specifies the hardware revision. Some AMD tools hard-code these values so we cannot change them. New ASICs should
/// be added at the end of the list and be given the next highest value.
enum class AsicRevision : uint32
{
    Unknown          = 0x00,
    Navi10           = 0x1F, ///< 10.1.0
    Navi12           = 0x21, ///< 10.1.1
    Navi14           = 0x23, ///< 10.1.2
    Navi21           = 0x24, ///< 10.3.0
    Navi22           = 0x25, ///< 10.3.1
    Navi23           = 0x26, ///< 10.3.2
    Navi24           = 0x27, ///< 10.3.4
#if PAL_BUILD_NAVI21_LITE
    Navi21Lite       = 0x28, ///< 10.2.0, but we treat it as 10.3.
#endif
#if PAL_BUILD_VAN_GOGH
    VanGogh          = 0x29, ///< 10.3.3
#endif
    Navi31           = 0x2C, ///< 11.0.0
    Navi32           = 0x2D, ///< 11.0.1
    Navi33           = 0x2E, ///< 11.0.2
    Rembrandt        = 0x2F, ///< 10.3.5
    Strix1           = 0x33, ///< 11.5.0
    Raphael          = 0x34, ///< 10.3.6
    Phoenix1         = 0x35, ///< 11.0.3
    Phoenix2         = 0x38, ///< 11.0.3
    HawkPoint1       = 0x39, ///< 11.0.3
    HawkPoint2       = 0x3A, ///< 11.0.3
    Krackan1         = 0x3B, ///< 11.5.2
    StrixHalo        = 0x3C, ///< 11.5.1
    Navi44           = 0x3D, ///< 12.0.0
    Navi48           = 0x3E, ///< 12.0.1
    Krackan2         = 0x3F, ///< 11.5.3
#if PAL_BUILD_AT_LITE3
#if PAL_CLIENT_INTERFACE_MAJOR_VERSION < 958
    AlphaTrion1      = 0x40, ///< 13.0.x
#else
    AlphaTrionX      = 0x40, ///< 13.0.x
#endif  // PAL_CLIENT_INTERFACE_MAJOR_VERSION < 958
#endif
#if PAL_BUILD_MEDUSA1
#if PAL_CLOSED_SOURCE
    Medusa1_A0       = 0x41, ///< 11.5.FFFE
#endif
    Medusa1          = 0x42, ///< 11.7.0
#endif
#if PAL_BUILD_MEDUSA2
    Medusa2          = 0x43, ///< 11.7.1
#endif
#if PAL_BUILD_MEDUSA3
    Medusa3          = 0x44, ///< 11.7.2
#endif
#if PAL_BUILD_GORGON_POINT1
    GorgonPoint1     = 0x45, ///< 11.5.0
#endif
#if PAL_BUILD_GORGON_POINT2
    GorgonPoint2     = 0x46, ///< 11.5.2
#endif
#if PAL_BUILD_GAINSBOROUGH
    Gainsborough     = 0x47, ///< 11.5.FFFC
#endif
#if PAL_BUILD_GORGON_HALO
    GorgonHalo       = 0x48, ///< 11.5.1
#endif
#if PAL_BUILD_ALPHA_TRION2
    AlphaTrion2      = 0x49, ///< 13.1.x
#endif
#if PAL_BUILD_GRIMLOCK1
    Grimlock1        = 0x4B, ///< 13.7.x
#endif
};

/// Maps a null GPU ID to its associated text name.
struct NullGpuInfo
{
    NullGpuId   nullGpuId;  ///< ID of an ASIC that PAL supports for override purposes
    const char* pGpuName;   ///< Text name of the ASIC specified by nullGpuId
};

/// Various IDs and info associated with a particular GPU.
struct GpuInfo
{
    AsicRevision asicRev;     ///< PAL specific ASIC revision identifier.
    NullGpuId    nullId;      ///< PAL specific GPU ID supported by the NULL OS layer.
    GfxIpLevel   gfxIpLevel;  ///< PAL specific identifier for the device's graphics IP level (GFXIP).
    uint32       familyId;    ///< Hardware family ID. Driver-defined identifier for a particular family of devices.
    uint32       eRevId;      ///< GPU emulation/internal revision ID.
    uint32       revisionId;  ///< GPU revision. HW-specific value differentiating between different SKUs or revisions.
    uint32       gfxEngineId; ///< Coarse-grain GFX engine ID (R800, SI, etc.).
    uint32       deviceId;    ///< PCI device ID (e.g., Hawaii XT = 0x67B0).
    const char*  pGpuName;    ///< ASIC name and AMDGPU target name (e.g., "NAVI31:gfx1100").
};

#if PAL_CLIENT_OCL
/// The client UMD must identify its API using this enum. Some UMD builds may implement multiple APIs so they must
/// specify which API they're implementing at runtime. Note that the PAL_CLIENT macros are the preferred way to
/// implement client-specific behavior; runtime ClientApi checks should only be used when necessary.
enum class ClientApi : uint32
{
    OpenCl,
    Hip
};
#endif

/// Specifies properties for @ref IPlatform creation. Input structure to Pal::CreatePlatform().
struct PlatformCreateInfo
{
    const Util::AllocCallbacks*  pAllocCb;      ///< Optional client-provided callbacks. If non-null, PAL will call the
                                                ///  specified callbacks to allocate and free all internal system
                                                ///  memory. If null, PAL will manage memory on its own through the C
                                                ///  runtime library.
    const Util::LogCallbackInfo* pLogInfo;      ///< Optional client-provided callback info.  If non-null, Pal will
                                                ///  call the callback to pass debug prints to the client.

    const char*                  pSettingsPath; ///< A null-terminated string describing the path to where settings are
                                                ///  located on the system. For example, on Windows, this will refer to
                                                ///  which UMD subkey to look in under a device's key. For Linux, this
                                                ///  is the path to the settings file.

    union
    {
        struct
        {
            uint32 disableGpuTimeout              :  1; ///< Disables GPU timeout detection (Windows only)
            uint32 force32BitVaSpace              :  1; ///< Forces 32bit VA space for the flat address with 32bit ISA
            uint32 createNullDevice               :  1; ///< Set to create a null device, so "nullGpuId" below for the
                                                        ///  ID of the GPU the created device will be based on.  Null
                                                        ///  devices operate in IFH mode; useful for off-line shader
                                                        ///  compilations.
            uint32 enableSvmMode                  :  1; ///< Enable SVM mode. When this bit is set, PAL will reserve
                                                        ///  cpu va range with size "maxSvmSize", and allow client to
                                                        ///  to create gpu or pinned memory for use of Svm.
                                                        ///  For detail of SVM, please refer to CreateSvmGpuMemory
            uint32 requestShadowDescriptorVaRange :  1; ///< Requests that PAL provides support for the client to use
                                                        ///  the @ref VaRange::ShadowDescriptorTable virtual-address
                                                        ///  range. Some GPU's may not be capable of supporting this,
                                                        ///  even when requested by the client.
            uint32 disableInternalResidencyOpts   :  1; ///< Disables residency optimizations for internal GPU memory
                                                        ///  allocations.  Some clients may wish to have them turned
                                                        ///  off to save on system resources.
            uint32 supportRgpTraces               :  1; ///< Indicates that the client supports RGP tracing. PAL will
                                                        ///  use this flag and the hardware support flag to setup the
                                                        ///  DevDriver RgpServer.
            uint32 dontOpenPrimaryNode            :  1; ///< No primary node is needed (Linux only)
            uint32 disableDevDriver               :  1; ///< If no DevDriverMgr should be created with this Platform.
            uint32 reserved                       : 23; ///< Reserved for future use.
        };
        uint32 u32All;                                  ///< Flags packed as 32-bit uint.
    } flags;                                            ///< Platform-wide creation flags.

#if PAL_CLIENT_DX
    DxRuntimeHandle              hDxRtAdapter;          ///< DX rumtime adapter handle.
    const DxRtAdapterCallbacks*  pDxRtAdapterCallbacks; ///< DX runtime adapter callbacks.
#endif
#if PAL_CLIENT_OCL
    ClientApi clientApiId; ///< Client API ID.
#endif
    NullGpuId nullGpuId;   ///< ID for the null device. Ignored unless the above flags.createNullDevice bit is set.
    uint16    apiMajorVer; ///< Major API version number to be used by RGP. Should be set by client based on their
                           ///  contract with RGP.
    uint16    apiMinorVer; ///< Minor API version number to be used by RGP. Should be set by client based on their
                           ///  contract with RGP.
    uint32    instrApiVer; ///  Instrumentation specification version for API-specific SQTT instrumentation fields.
                           ///  Should be set by client based on the SQTT instrumentation spec version being targeted.
    gpusize   maxSvmSize;  ///< Maximum amount of virtual address space that will be reserved for SVM
};

/**
************************************************************************************************************************
* @brief Determines the amount of system memory required for a Platform object.
*
* This function must be called before any other interaction with PAL. An allocation of this amount of memory must be
* provided in the pPlacementAddr parameter of Pal::CreatePlatform.
*
* @ingroup LibInit
*
* @returns Size, in bytes, of system memory required for an IPlatform object.
************************************************************************************************************************
*/
size_t PAL_STDCALL GetPlatformSize();

/**
 ***********************************************************************************************************************
 * @brief Creates the Platform Abstraction Library.
 *
 * On execution of CreatePlatform(), PAL will establish a connection for OS and KMD communication, install the specified
 * system memory allocation callbacks, and initialize any global internal services.  Finally, the client will be
 * returned an object pointer to the instantiated platform object, which is used to query the capabilities of the
 * system.
 *
 * @ingroup LibInit
 *
 * @param [in]  createInfo     Parameters indicating the client requirements for the platform such as allocation
                               callbacks or the settings path.
 * @param [in]  pPlacementAddr Pointer to the location where PAL should construct this object.  There must be as
 *                             much size available here as reported by calling GetPlatformSize().
 * @param [out] ppPlatform     Platform object pointer to the instantiated platform. Must not be null.
 *
 * @returns Success if the initialization completed successfully.  Otherwise, one of the following error codes may be
 *          returned:
 *          + ErrorInvalidPointer will be returned if:
 *              - pPlatform is null.
 *              - pPlacementAddr is null.
 *              - createInfo.pAllocCb is non-null but pfnAlloc and/or pfnFree is null.
 *              - createInfo.pSettingsPath is null.
 *          + ErrorInitializationFailed will be returned if PAL is unable to open a connection to the OS.
 *          + ErrorUnavailable will be returned if none of the GPUs in this system are supported.
 ***********************************************************************************************************************
 */
Result PAL_STDCALL CreatePlatform(
    const PlatformCreateInfo&   createInfo,
    void*                       pPlacementAddr,
    IPlatform**                 ppPlatform);

/**
 ***********************************************************************************************************************
 * @brief Provides an association of NULL devices and their associated text name.  NULL devices operate in IFH mode
 *        and are primarily intended for off-line shader compilation mode.  The text name is provided for end-user
 *        identification of the GPU device being created.
 *
 * @param [in,out] pNullDeviceCount   On input, this is the size of the "pNullDevices" array.  On output, this
 *                                    reflects the number of valid entries in the "pNullDevices" array.
 * @param [out]    pNullDevices       Includes information on the valid NULL devices supported by the system.  If
 *                                    this is NULL, then pNullDeviceCount reflects the maximum possible size of the
 *                                    null-devices array.
 *
 * @returns Success if the initialization completed successfully.  Otherwise, one of the following error codes may be
 *          returned:
 *          + ErrorInvalidPointer will be returned if either input is NULL.
 ***********************************************************************************************************************
 */
Result PAL_STDCALL EnumerateNullDevices(
    uint32*       pNullDeviceCount,
    NullGpuInfo*  pNullDevices);

/**
 ***********************************************************************************************************************
 * @brief Provides the NULL device GpuInfo data for the specified NullGpuId.
 *
 * @param [in]  nullGpuId Null GPU ID to lookup.
 * @param [out] pGpuInfo  GpuInfo data on successful lookup. Must not be null.
 *
 * @returns Success if the lookup completed successfully. Otherwise, one of the following error codes may be returned:
 *          + ErrorInvalidPointer will be returned if pGpuInfo is NULL.
 *          + NotFound will be returned if the Null GPU ID was not found.
 ***********************************************************************************************************************
 */
Result PAL_STDCALL GetNullGpuInfoForNullGpuId(
    NullGpuId nullGpuId,
    GpuInfo*  pGpuInfo);

#if PAL_CLIENT_INTERFACE_MAJOR_VERSION < 933
inline Result PAL_STDCALL GetGpuInfoForNullGpuId(
    NullGpuId nullGpuId,
    GpuInfo*  pGpuInfo)
{
    return GetNullGpuInfoForNullGpuId(nullGpuId, pGpuInfo);
}
#endif

/**
 ***********************************************************************************************************************
 * @brief Provides the NULL device GpuInfo data for the specified GPU name string.
 *
 * @param [in]  pGpuName Name string of the GPU to lookup (e.g., "NAVI10").
 * @param [out] pGpuInfo GpuInfo data on successful lookup. Must not be null.
 *
 * @returns Success if the lookup completed successfully. Otherwise, one of the following error codes may be returned:
 *          + ErrorInvalidPointer will be returned if pGpuName or pGpuInfo are NULL.
 *          + NotFound will be returned if the Name string was not found.
 ***********************************************************************************************************************
 */
Result PAL_STDCALL GetNullGpuInfoForName(
    const char* pGpuName,
    GpuInfo*    pGpuInfo);

#if PAL_CLIENT_INTERFACE_MAJOR_VERSION < 933
inline Result PAL_STDCALL GetGpuInfoForName(
    const char* pGpuName,
    GpuInfo*    pGpuInfo)
{
    return GetNullGpuInfoForName(pGpuName, pGpuInfo);
}
#endif

/**
 ***********************************************************************************************************************
 * @brief Provides the NULL device GpuInfo data for the specified hardware revision.
 *
 * @param [in]  asicRevision Hardware revision to lookup.
 * @param [out] pGpuInfo     GpuInfo data on successful lookup. Must not be null.
 *
 * @returns Success if the lookup completed successfully. Otherwise, one of the following error codes may be returned:
 *          + ErrorInvalidPointer will be returned if pGpuInfo is NULL.
 *          + NotFound will be returned if the hardware revision was not found.
 ***********************************************************************************************************************
 */
Result PAL_STDCALL GetNullGpuInfoForAsicRevision(
    AsicRevision asicRevision,
    GpuInfo*     pGpuInfo);

#if PAL_CLIENT_INTERFACE_MAJOR_VERSION < 933
inline Result PAL_STDCALL GetGpuInfoForAsicRevision(
    AsicRevision asicRevision,
    GpuInfo*     pGpuInfo)
{
    return GetNullGpuInfoForAsicRevision(asicRevision, pGpuInfo);
}
#endif

/**
 ***********************************************************************************************************************
 * @defgroup LibInit Library Initialization and Destruction
 *
 * Before initializing PAL, it is important to make sure that the interface version is consistent with the client's
 * expectations.  The client should check @ref PAL_INTERFACE_MAJOR_VERSION to ensure the major interface version has not
 * changed since the last PAL integration.  Ideally, this should be performed with a compile-time assert comparing
 * @ref PAL_INTERFACE_MAJOR_VERSION against a client-maintained expected major version.   Minor interface version
 * changes should be backward compatible, and do not require a client change to maintain previous levels of
 * functionality.
 *
 * On startup, the client's first call to PAL must be GetPlatformSize() followed by CreatePlatform().  This function
 * gives an opportunity for PAL to perform any necessary platform-wide initialization such as opening a connection for
 * communication with the operating system and kernel mode driver or initializing tracking facilities for system memory
 * management.  CreatePlatform() returns a created IPlatform object for future interaction with PAL.
 *
 * PAL optionally allows the client to specify a set of memory management callbacks during initialization.  If
 * specified, PAL will not allocate or free any memory directly from the runtime, instead calling back to the client.
 * The client (or application, if the client forwards on the requests) may be able to implement a more efficient
 * allocation scheme.
 *
 * After a successful call to CreatePlatform(), the client should call @ref IPlatform::EnumerateDevices() in order to
 * get a list of supported devices attached to the system.  This function returns an array of @ref IDevice objects
 * which are used by the client to query properties of the devicess and eventually execute work on those devices.
 * IPlatform::EnumerateDevices() is not available to util-only clients (PAL_BUILD_CORE=0).
 *
 * The client may re-enumerate devices at any time by calling IPlatform::EnumerateDevices().  The client must make sure
 * there is no active work on any device and that all objects associated with those devices have been destroyed.
 * IPlatform::EnumerateDevices() will destroy all previously reported @ref IDevice objects and return a fresh set.
 * The client is required to re-enumerate devices when it receives a ErrorDeviceLost error from PAL.
 *
 * After enumerating devices, either during start-up or when recovering from an ErrorDeviceLost error, the client must
 * setup and finalize PAL's per-device settings.  See IDevice::GetPublicSettings(), IDevice::SetDxRuntimeData(),
 * IDevice::CommitSettingsAndInit(), and IDevice::Finalize() for details.
 *
 * After enumerating devices and finalizing them, the client may query the set of available screens. This is done by
 * calling the @ref IPlatform::GetScreens() function.  Note that screens are not available for DX clients.  Each screen
 * is accessible by zero or more of the enumerated devices. Most screens are accessible from a "main" device as well as
 * several other devices which can perform cross-display Flip presents to the screen. In some configurations, screens
 * may not be directly to any of PAL's devices, in which case fullscreen presents are unavailable to that screen. (This
 * typically only occurs in PowerExpress configurations.) Note that when IPlatform::EnumerateDevices() is called, any
 * enumerated @ref IScreen objects which existed prior to that call are invalidated for the specified platform and
 * IPlatform::GetScreens() needs to be called again to get the updated list of screens.
 *
 * On shutdown, the client should call @ref IPlatform::Destroy() to allow PAL to cleanup and free any remaining
 * platform-wide resources.  The client must ensure this call is not made until all other created objects are idle and
 * destroyed (if destroyable).
 *
 * When the client is asked to destroy a device it may call IDevice::Cleanup() to explicitly clean up the device. Some
 * clients will find it necessary to call Cleanup(), for example, if their devices have OS handles that become invalid.
 * Note that Cleanup() doesn't destroy the device; it will return to its initial state, as if it was newly enumerated.
 ***********************************************************************************************************************
 */
#if PAL_CLOSED_SOURCE
/**
 ***********************************************************************************************************************
 * @page Build Building PAL
 *
 * Client-Integrated Builds
 * ------------------------
 * PAL is a _source deliverable_.  Clients will periodically promote PAL's source from //depot/stg/pal_prm into their
 * own tree and build a static pal.lib as part of their build process.  This process matches what is done for other
 * shared components in our driver stack such as SC, AddrLib, and VAM.
 *
 * ### Internal Pipeline Compiler Component
 *
 *  PAL is delivered alongside a module which can compile pipeline binaries in ELF format.  This module, named SCPC, is
 *  based on the AMD proprietary shader compiler (SC).  The following build options in PAL are used to control how SCPC
 *  is included in the PAL build.
 *
 *      __PAL_BUILD_SCPC__: Defaults to 1.  Controls whether or not the SCPC component is built as part of the PAL
 *      build.  Clients should only change this to zero if they are using something besides SCPC for compiling their
 *      pipeline binaries.
 *
 * ### External Shader Compiler
 * PAL must be linked with an SC library built by the client.  The client must specify the location of the SC interface
 * header files with this build parameter:
 *
 * + __PAL_SC_DIR__: Root of SC source (PAL will include headers from both the Interface and IL/inc subdirectories).
 *
 * The client is responsible for providing a version of the SC library that is compatible with PAL.  PAL will fail to
 * build if SC's major interface version isn't supported.  Since PAL handles all interaction with SC, PAL is responsible
 * for defining the SC_CLIENT_INTERFACE_MAJOR_VERSION variable on behalf of the client.
 *
 * ### Build Options
 * The following build options control PAL's behavior, and can be set as desired by the client:
 *
 * + __Required__:
 *     - __PAL_SC_DIR__: As described above.
 * + __Optional__:
 *     - __PAL_CLOSED_SOURCE__: Defaults to 1.  Set to 0 to build only open source-able code.
 * @cond PAL_CLOSED_SOURCE
 *     - __Client-type specifier__.  These build flags are used to tell PAL which client it is being built for, and will
 *       allow compile-time selection of execution path or optimizations based on removing features that aren't needed
 *       by a particular client.  Current choices (only one of these may be set):
 *         * __PAL_CLIENT_VULKAN__: Set to 1 for the Vulkan ICD.
 *         * __PAL_CLIENT_DX9__:  Set to 1 for the DX9 user mode driver.
 *         * __PAL_CLIENT_DX11__: Set to 1 for the DX11 user mode driver.
 *         * __PAL_CLIENT_DX12__: Set to 1 for the DX12 user mode driver.
 * @endcond
 *     - __PAL_BUILD_CORE__: Defaults to 1.  Set to 0 to build only the PAL utility companion functionality (only the
 *       Util namespace will be usable).
 *     - The following build options allow specific IP support to be explicitly included or excluded:
 *         + __PAL_BUILD_GFX9__: Defaults to 1.  Set to 0 to exclude support for GFXIP 9 layer.
  *     - __PAL_ENABLE_PRINTS_ASSERTS__: Enables debug printing and assertions.  Even if enabled at build time, debug
 *       prints and asserts can be filtered based on category/severity via runtime setting.  Defaults to 1 on debug
 *       builds.
 *     - __PAL_MEMTRACK__: Enables memory leak and buffer overrun tracking.  Defaults to 1 on debug builds if debug
 *       prints are also enabled.  A report of leaked memory will be printed during IPlatform::Destroy().
 *     - __PAL_DEVELOPER_BUILD__: Defaults to 0. If 1, enables developer-specific interfaces for development purposes.
 *
 * @note Some Util functionality is inline/macro based, and therefore the appropriate defines must be set when building
 *       client files that include PAL headers.  In particular, PAL_MEMTRACK and PAL_ENABLE_PRINTS_ASSERTS are used in
 *       palAssert.h and palSysMemory.h, and must match the setting used when building PAL even when included outside
 *       of the PAL library.
 *
 * Next: @ref UtilOverview
 ***********************************************************************************************************************
 */
#endif

} // Pal
