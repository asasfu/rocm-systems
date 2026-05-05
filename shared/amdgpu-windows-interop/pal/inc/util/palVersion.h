/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palVersion.h
 * @brief Defines the Platform Abstraction Library (PAL) Interface Version
 ***********************************************************************************************************************
 */

#pragma once

/// Major interface version.  Note that the interface version is distinct from the PAL version itself, which is returned
/// in @ref Pal::PlatformProperties.
///
/// @attention Updates to the major version indicate an interface change that is not backward compatible and may require
///            action from each client during their next integration.  When determining if a change is backward
///            compatible, it is assumed that the client will default-initialize all structs.
///
/// @ingroup LibInit
#define PAL_INTERFACE_MAJOR_VERSION 986

/// Minimum major interface version. This is the minimum interface version PAL supports in order to support backward
/// compatibility. When it is equal to PAL_INTERFACE_MAJOR_VERSION, only the latest interface version is supported.
///
/// @ingroup LibInit
#define PAL_MINIMUM_INTERFACE_MAJOR_VERSION 916

/// Minimum supported major interface version for devdriver library. This is the minimum interface version of the
/// devdriver library that PAL is backwards compatible to.
///
/// @ingroup LibInit
#define PAL_MINIMUM_GPUOPEN_INTERFACE_MAJOR_VERSION 38

/**
 ***********************************************************************************************************************
 * @def     PAL_INTERFACE_VERSION
 * @ingroup LibInit
 * @brief   Current PAL interface version packed into a 32-bit unsigned integer. The low 16 bits are always zero.
 *          They used to contain the interface minor version and remain as a placeholder in case we add it back.
 *
 * @see PAL_INTERFACE_MAJOR_VERSION
 *
 * @cond PAL_CLOSED_SOURCE
 * @copydoc VersionHistory
 * @endcond
 * @hideinitializer
 ***********************************************************************************************************************
 */
#define PAL_INTERFACE_VERSION (PAL_INTERFACE_MAJOR_VERSION << 16)

//# This PAL_COMPILE_TYPE check is a temporary hack to prevent breaking release branches that contain versions of LLPC
//# and/or ABCD that directly include PAL headers instead of linking against "pal" or "palUtil" in cmake.
//#
//# If some client directly includes PAL headers without linking in cmake, none of PAL's target_compile_definitions()
//# calls run so all PAL_* macros look like they're set to zero. This completely breaks all major interface deprecation
//# checks which is why it's illegal. It also makes it impossible to roll out the below static_asserts in the palUtil
//# includes without breaking compatibility with the old broken versions of LLPC and ABCD. The best thing I could come
//# up with was checking a PAL_* macro that we know should always be non-zero like PAL_COMPILE_TYPE. If it is zero that
//# means the client isn't using cmake properly.
//#
//# The LLPC and ABCD bugs should be fixed in 26.10. Once 26.10 is the oldest branch PAL supports we should remove this
//# hack and see if anything breaks. Anything that breaks is probably a new illegal include that snuck in during this
//# deprecation window and the whole dance repeats...
#if PAL_COMPILE_TYPE != 0
// Static asserts to ensure clients define PAL_CLIENT_INTERFACE_MAJOR_VERSION and that it falls in the supported range.
#ifndef PAL_CLIENT_INTERFACE_MAJOR_VERSION
    static_assert(false, "The client must link against 'palUtil' or 'pal' in CMake!");
#else
    static_assert((PAL_CLIENT_INTERFACE_MAJOR_VERSION >= PAL_MINIMUM_INTERFACE_MAJOR_VERSION) &&
                  (PAL_CLIENT_INTERFACE_MAJOR_VERSION <= PAL_INTERFACE_MAJOR_VERSION),
                  "The specified PAL_CLIENT_INTERFACE_MAJOR_VERSION is not supported.");
#endif
#endif

#if PAL_CLOSED_SOURCE // Version history details are closed source (descriptions may contain internal details)
/**
 ***********************************************************************************************************************
 * @page VersionHistory
 * %Version History
 * ---------------
 *
 * | %Version | Change Description                                                                                     |
 * | -------- | ------------------------------------------------------------------------------------------------------ |
 * |   986.0  | Add TileOptMode::BlockBased                                                                            |
 * |   985.0  | Add "available_threads_per_wg" field to pipeline metadata.                                             |
 * |   984.0  | Remove/split chipProps.gfxip.max3dDispatchInterleaveProduct to track GFX and ACE separately            |
 * |   983.0  | Add unique export names for mesh and vertex Work Graphs SRL functions. Remove the old overloads.       |
 * |   982.0  | Remove aggregate initialization for IpTriple. Use IpLevel in cases where you wish to express a certain |
 * |          | group of HW features. IpTriple should only be used in cases where you have a specific stepping value.  |
 * |          | Note that many IpTriple <> IpLevel operators exist, e.g. "if (gfxTriple >= IpLevel(12, 0))".           |
 * |   981.0  | Add IFence::Reset() to be able to be used in cases where only 1 fence needs a reset.                   |
 * |   980.0  | Add ResolveMode::SampleZero to be used for existing depth and stencil resolves. ResolveMode::Average   |
 * |          | can now be used for depth that considers all samples similar to the unchanged behavior for color.      |
 * |   979.0  | Remove PAL_BUILD_STRIX_HALO, PAL_BUILD_HAWK_POINT1, PAL_BUILD_HAWK_POINT2, PAL_BUILD_KRACKAN1, and     |
 * |          | PAL_BUILD_KRACKAN2. Clients should use PAL_BUILD_GFX9 instead.                                         |
 * |   978.0  | Remove createInfo from GetGraphicsPipelineSize and GetComputePipelineSize.                             |
 * |   977.0  | Unify Pal::ShaderType with the Pipeline ABI shader stage enum; keep legacy spellings for older clients.|
 * |   976.0  | Removes old Work Graphs SRL interface for mesh & vertex shaders.  Also removes support for cross-group |
 * |          | sharing for Mesh nodes, which was never actually part of the WG API.                                   |
 * |   975.0  | Note: This interface change was backed out. Upgrading to this version number changes nothing in PAL.   |
 * |   974.0  | Replace IGpuMemory with gpuva for SetHipTrapHandler                                                    |
 * |   973.0  | Remove a bunch of unused ICmdBuffer functions: CmdIf, CmdElse, CmdEndIf, CmdWhile, CmdEndWhile,        |
 * |          | CmdMemoryAtomic, CmdCopyRegisterToMemory, CmdWaitRegisterValue, plus some supporting enums and flags.  |
 * |   972.0  | Remove MaxPayloadSize and allow arbitrary length strings and buffers in CmdCommentString and CmdNop.   |
 * |          | Also, CmdCommentString uses StringView<char> now so the strings don't need a NULL-terminator!          |
 * |          | Also also, StringView can call Strlen at compile time and it now has DropFront and DropBack            |
 * |   971.0  | Change PipelineCreateFlags::reverseWorkgroupOrder to an enum in ComputePipelineCreateInfo.             |
 * |   970.0  | Add "vgpr_count_max" and "vgpr_count_rts" fields to ELF metadata.                                      |
 * |   969.0  | Add GpuVaHintFlags for ICmdBuffer::CmdCopyMemoryByGpuVa to indicate if memory is TMZ Protected.        |
 * |          | Also add ICmdBuffer::CmdCopyMemoryToImageByGpuVa and ICmdBuffer::CmdCopyImageToMemoryByGpuVa.          |
 * |   968.0  | Deprecate Util::RemoveCvref. Use std::remove_cvref instead.                                            |
 * |   967.0  | Deprecate CoherClear, plase use CoherCopyDst instead.                                                  |
 * |   966.0  | Adds a new Work Graphs SRL interface for mesh & vertex shaders which more closely parallels the one    |
 * |          | used for compute nodes.  This will eventually replace the existing mesh & vertex shader SRL interface. |
 * |   965.0  | Remove arguments from CmdRestoreComputeState, rename ComputeStateAll, and add ComputeStateTreatAsBlt.  |
 * |   964.0  | Remove memAlign from VpeTonemapParams and adds a VpeIpCapabilities sub-struct into VpeIpProperties     |
 * |   963.0  | Deprecate image creation flag fullResolveDstOnly.                                                      |
 * |   962.0  | move frameId from DX PrivatePresentInfo to PresentDirectInfo                                           |
 * |   961.0  | Deprecate EQAA from Image create and ClearRTV/DSV. Fragments and samples no longer can be different.   |
 * |          | Also change the max supported image sample value from dynamic to statically specified.                 |
 * |   960.0  | Deprecate image creation flag fullCopyDstOnly.                                                         |
 * |   959.0  | Deprecate LateAllocVs from GfxPipeline.                                                                |
 * |   958.0  | Rename Pal::AsicRevision::AlphaTrion1 to Pal::AsicRevision::AlphaTrionX. Remove AlphaTrion1 and Navi52 |
 * |   957.0  | Deprecate Image repetitiveResolve flag.                                                                |
 * |   956.0  | Deprecate some public settings related to GFX10                                                        |
 * |   955.0  | Rename CopyControlFlags to CopyImageControlFlags; add copy control flag CopyMemroyToImageControlFlags  |
 * |          | for CmdCopyMemoryToImage(); allow clear and copy to initialize metadata via newly defined flags.       |
 * |   954.0  | Add a DispatchAqlFeedback* argument to CmdDispatchAql to return information about the dispatch.        |
 * |   953.0  | Change interface of GetFullSubresourceRange() to make it easier to call.                               |
 * |   952.0  | Remove imageVaLocked from PAL's color targets and depth/stencil targets. We have long required that    |
 * |          | imageVaLocked = 1 so no existing clients should be rebinding image memory and using old view objects.  |
 * |   951.0  | Add max thread group count fields for mesh shaders.                                                    |
 * |   950.0  | Remove DepthClampMode::ZeroToOne.                                                                      |
 * |   949.0  | Adds compression option to ITraceSource                                                                |
 * |   948.0  | Removes unsupported XDMA, HW Composition, and MGPU SLS interfaces.                                     |
 * |   947.0  | changes the CmdResolveEncoderOutputOptionalMetadata parameter to include output to GpuMemory           |
 * |   946.0  | Removes PAL_WORK_GRAPHS_SUPPORT_2_0.  Use PAL_WORK_GRAPHS_SUPPORT instead.                             |
 * |   945.0  | Remove Navi44_A0, Navi48_A0, Strix_A0 support from PAL                                                 |
 * |   944.0  | changes the vpeIpProperties structure to include multi-instances                                       |
 * |   943.0  | Removes WDDM SetMgpuMode() and SetupHardwareCompositing() as KMD no longer supports these via Escape().|
 * |   942.0  | Add "named_bar_cnt" field to ELF                                                                       |
 * |   941.0  | changes the meaning of the existing notifyOnly flag:PAL must not execute a present if this is true but |
 * |          | may update internal frame tracking state                                                               |
 * |   940.0  | Change SettingsFileMgr::Init to remove defaulted pciId parameter. Add overload for back-compat.        |
 * |   939.0  | Add Postamble phase to TraceSession.                                                                   |
 * |   938.0  | Replace separate mip and slice fields in PrtPlusImageResolveRegion with src/dst SubresId fields        |
 * |   937.0  | Removes PAL_BUILD_NAVI4X, PAL_BUILD_NAVI44, and PAL_BUILD_NAVI48. Use PAL_BUILD_GFX12 instead.         |
 * |   936.0  | Remove MaxDescriptorSets, PrtFeatureNonStandardImage3D, forceLoadObjectFailure, catalystAI,            |
 * |          | disableScManager, allowNonIeeeOperations, and appendBufPerWaveAtomic.                                  |
 * |   935.0  | Removes PAL_BUILD_GFX11, use PAL_BUILD_GFX9 instead. Removes SystemEventClientId::Scpc.                |
 * |   934.0  | Remove enablePresentThread from SwapChainCreateInfo.                                                   |
 * |   933.0  | Rename GetGpuInfo helpers to GetNullGpuInfo to clarify they only support null device use cases.        |
 * |   932.0  | Support for 128-bit hash in member name in archive pipeline. Also remove unused retention ID support.  |
 * |   931.0  | Deprecate Util::HashLiteralString(). Use Util::CompileTimeHashString() instead.                        |
 * |   930.0  | Util::operator~ for unscoped enums.                                                                    |
 * |   929.0  | Implement 4 multimedia formats with 4 components and 10bit/12bit precision                             |
 * |   928.0  | Deprecate ICmdBuffer::CmdBarrier().                                                                    |
 * |   927.0  | Deprecate supportSplitReleaseAcquire from queueProperties.                                             |
 * |   926.0  | Rename GpuBlock::RlcUser to GpuBlock::RlcLocal                                                         |
 * |   925.0  | Implement 5 planar YUV formats - YV16, YV24, NV24, P410, P416                                          |
 * |   924.0  | Implement 9 tri-planar YUV formats - YUV444P10, YUV420P10, etc.                                        |
 * |   923.0  | Implement P216 format.                                                                                 |
 * |   922.0  | Use std::chrono types in Util::File::Stat.                                                             |
 * |   921.0  | Define Util::IsValidHandle(). Clients should use this helper instead of defining their own.            |
 * |   920.0  | Replace amd_kernel_code_t with llvm::amdhsa::kernel_descriptor_t in DispatchAqlParams                  |
 * |   919.0  | Deprecate Util::PalWcslen(), Util::PalWcsrchr(), Util::Wcslen(), and Util::Wcsrchr().                  |
 * |          | Use std::wcslen() and std::wcsrchr() instead.                                                          |
 * |   918.0  | ABI and API changes for NPRT mesh nodes.                                                               |
 * |   917.0  | Remove PAL_BUILD_GFX115, PAL_BUILD_STRIX, and PAL_BUILD_STRIX1, use PAL_BUILD_GFX11 instead.           |
 * |   916.0  | Add instrApiVer to PlatformCreateInfo. Client driver should set this to the RGP SQTT instrumentation   |
 * |          | API version being targeted.                                                                            |
 * |   915.0  | Deprecate ClientApi except in PAL_CLIENT_OCL builds because only the OCL UMD supports multiple APIs.   |
 * |          | We should always prefer the relevant PAL_CLIENT_X macros to fence client specific behavior.            |
 * |   914.0  | Deprecate MemBarrier::flags::globallyAvailable and constant engine support.                            |
 * |   913.0  | Integrate image clone support into CmdCopyImage(). CmdCloneImageData() still exists but will be        |
 * |          | removed at a future version bump.                                                                      |
 * |   912.0  | Remove SubresLayout::defaultGfxLayout.                                                                 |
 * |   911.0  | Deprecate HardwareStageMetadata::entryPoint (enum).  Add ::entryPointSymbol (string).                  |
 * |   910.0  | Remove CmdClearBufferView and CmdClearImageView. Clients must instead call DecodeBufferViewSrd or      |
 * |          | DecodeImageViewSrd to unpack the SRD and then call one of the other CmdClear functions.                |
 * |   909.0  | Add DispatchInfoFlags to CmdBuffer CmdDispatch and DeveloperHooks DrawDispatchDispatchArgs.            |
 * |   908.0  | Update ITraceController interface to provide a command buffer to ITraceSources on the prep phase of a  |
 * |          | trace to allow for work to be recorded outside of the trace execution window to avoid perf hits.       |
 * |   907.0  | Reimplement ArchiveFile as a memory mapped file on both linux and windows.                             |
 * |   906.0  | Add multi-ELF graphics support to PipelineAbiReader.  Must now be constructed from a Span<const void>. |
 * |          | Rename GetPipelineSymbol/GetGenericSymbol to GetSymbolHeader, add GetSymbol and CopySymbol.            |
 * |   905.0  | Add integer types with specific size in enum ValueType, and deprecate ValueType::Int/Uint.             |
 * |   904.0  | Remove 'UavExportTable' from the pipeline ABI's UserDataMapping and a 'uavExportSingleDraw' flag from  |
 * |          | GraphicsPipelineCreateInfo.cbState                                                                     |
 * |   903.0  | Change palCmdBuffer::VideoProcessorFrameInfo::inputStreamsInfo from static array size to dynamic array |
 * |   902.0  | Change BarrierData::transition from BarrierTransition to ImgBarrier.                                   |
 * |   901.0  | Change Developer::AcquirePoint from scoped enum to C-style enum.                                       |
 * |   900.0  | Change GpaSampleConfig::timing::preSample/postSample from HwPipePoint to PipelineStageFlag and retire  |
 * |          | HwPipePoint version of CmdSetEvent/CmdResetEvent/CmdWriteTimestamp/CmdWriteImmediate.                  |
 * |   899.0  | Add threadTraceEnableExec to ThreadTraceInfo                                                           |
 * |   898.0  | De-alias CacheCoherencyUsageFlags::CoherCp and CacheCoherencyUsageFlags::CoherTimestamp with explicit  |
 * |          | requirement for use of CoherTimestamp in barriers that enforce the completion of CmdWriteTimestamp.    |
 * |   897.0  | Remove EnableTcpBigPageTranslationCoalescing from PAL public settings                                  |
 * |   896.0  | Move the RenderOp enum out of GpuUtil::RenderOpTraceController and deprecate `RecordRenderOp` function |
 * |   895.0  | Add GFX13 GpuBlock Definitions to palExperiment                                                        |
 * |   894.0  | Change ScreenMode.refreshRate to Rational in palScreen.h                                               |
 * |   893.0  | Remove supportReleaseAcquireInterface from DeviceProperties as it's supported on all ASICs now.        |
 * |   892.0  | Report typed and untyped buffer SRD sizes separately in gfxipProperties                                |
 * |   891.0  | Deprecate unused fields in InheritedStateParams.                                                       |
 * |   890.0  | Deprecate legacy settings RPC service interface and part of code-gen.                                  |
 * |   889.0  | Add Padding into IndirectParamType as part of IndirectCmdGeneratorCreateInfo                           |
 * |   888.0  | Removed many old cmake and C++ interfaces for gfx6-9:                                                  |
 * |          | - Removed cmake variables: PAL_BUILD_GFX6, PAL_BUILD_OSS, PAL_BUILD_OSS2_4, and PAL_BUILD_PHOENIX2.    |
 * |          | - Removed pipeline metadata: pos_scaling_enable, pos_x_scaling, pos_y_scaling.                         |
 * |          | - Removed EngineTypeVceEncode, EngineTypeUvdDecode, EngineTypeUvdEncode, EngineTypeSpu.                |
 * |          | - Removed ICmdBuffer::CmdXdmaWaitFlipPending, XDMA isn't supported in gfx10+.                          |
 * |          | - Refactored ImageDataAddrMgrSurfInfo in palDeveloperHooks.h.                                          |
 * |          | - Removed PrivatePalGfx6Key.                                                                           |
 * |          | - Removed OssIpLevel, VceIpLevel, and UvdIpLevel.                                                      |
 * |          | - Removed VcnIpLevel::VcnIp1 and VcnIpLevel::VcnIp2_2 (gfx9 APUs) but not VcnIpLevel::VcnIp2 (Navi1x). |
 * |          | - Removed VceIpProperties, UvdIpProperties and VideoIpType.                                            |
 * |          | - Removed RayTracingIpLevel::RtIp1_5.                                                                  |
 * |          | - Removed all gfx6-9 values in the following enums: NullGpuId, AsicRevision, GfxIpLevel,               |
 * |          |   AmdGpuMachineType, GfxIpStepping.                                                                    |
 * |   887.0  | Replace numSlices with srcSlices and dstSlices in ImageScaledCopyRegion                                |
 * |   886.0  | Change SubresRange member data types to save size.                                                     |
 * |   885.0  | Change CmdRelease() and CmdAcquire() sync token from uint32 to structure ReleaseToken.                 |
 * |   884.0  | AbiProcessor::LoadFromBuffer and ElfProcessor::LoadFromBuffer no longer implicitly initialize          |
 * |   883.0  | Move supportSplitReleaseAcquire from gfxipProperrties to queue queueProperties.                        |
 * |   882.0  | Moves syncFdSignalValue from the ExternalQueueSemaphoreOpenInfo struct to inputs of                    |
 * |          | IDevice::AssociateNativeFence. This migration allows for copying the state of a sync fd without        |
 * |          | destroying the underlying semaphore/fence object in LibDXG Android builds.                             |
 * |   881.0  | Change WsiPlatform to scoped enum.                                                                     |
 * |   880.0  | Remove unused member box from ImgBarrier and memory from MemBarrier.                                   |
 * |   879.0  | Remove DbgLoggerDevDriver::Cleanup(), the m_logEventProvider will be destroyed in its destructor.      |
 * |   878.0  | Assert that clients don't define min/max macros.                                                       |
 * |   877.0  | Adds overload method ICmdBuffer::CmdSetVertexBuffers() to support D3D12_VERTEX_BUFFER_VIEW structure   |
 * |   876.0  | Create new type for ImageCreateInfo::clientCompressionMode                                             |
 * |   875.0  | Depreciate DeferredBatchBinMode::DeferredBatchBinDisabled and adjust enum value                        |
 * |   874.0  | Change names of marker enums to indicate SPM or SQTT application                                       |
 * |   873.0  | Move Util::fseconds, Util::fmilliseconds, Util::fmicroseconds, Util::fnanoseconds,                     |
 * |          | Util::SecondsSinceEpoch, and Util::TimeoutCast from palUtil.h to palTime.h.                            |
 * |   872.0  | Add flipIntervalOverride to Pal::PresentSwapChainInfo flags.                                           |
 * |   871.0  | Public NOMINMAX and WIN32_LEAN_AND_MEAN                                                                |
 * |   870.0  | Use std::chrono for Util::Sleep() (formerly Util::SleepMs()), IQueue::Delay()                          |
 * |          | and IQueue::DelayAfterVsync.                                                                           |
 * |   869.0  | Remove RasterizerState.forceShadingRate. Pls use forceSampleRateShading in MsaaStateCreateInfo instead.|
 * |   868.0  | Add Util::GetProcessIntegrityLevel() and Util::IsProcessInAppContainer().                              |
 * |   867.0  | Add image pointer argument for OptimizeAcqRelReleaseInfo() to allow more aggressive optimization.      |
 * |   866.0  | Add Util::PalToHResult() and Util::HResultToPal().                                                     |
 * |   865.0  | Add UMD_FRAME_ID interface to CmdBufInfo to help with present to flip correspondence.                  |
 * |   864.0  | Change interface for OptimizeAcqRelReleaseInfo() to allow more aggressive optimization.                |
 * |   863.0  | Use std::chrono for IDevice::WaitForFences(), IDevice::WaitForSemaphores(), Event::Wait(),             |
 * |          | IQueueSempahore::WaitSemaphoreValue(), Util::RemoveFilesOfDirOlderThan(), Util::GetStatusOfDir(),      |
 * |          | ConditionVariable::Wait(), RingBuffer::GetBufferForWriting(), Semaphore::Wait(),                       |
 * |          | and AcquireNextImageInfo::timeout.                                                                     |
 * |   862.0  | Change interface for CreateDevDriverLogger to handle initialization and return a Result                |
 * |   861.0  | Modify GetCacheFilePath() & GetDebugFilePath() to return UTF-8 char arrays on Windows.                 |
 * |   860.0  | Add additional shader cache EventTypes and a ShaderCache pointer to developer callbacks.               |
 * |   859.0  | Remove deprecated flag update3DLut for vpelib.                                                         |
 * |   858.0  | Add BarrierType argument for OptimizeAcqRelReleaseInfo() for more potential optimizations.             |
 * |   857.0  | Remove unused interface ICmdBuffer::OptimizeBarrierReleaseInfo().                                      |
 * |   856.0  | Change interface for CmdWaitMemoryValue from IGpuMemory base addr + offset to a GPU virtual address    |
 * |   855.0  | Deprecate CreateInfo argument to Msaa, DepthStencil and ColorBlend state object Size methods.          |
 * |   854.0  | Compact MsaaStateCreateInfo structure.                                                                 |
 * |   853.0  | Compact ColorBlendStateCreateInfo structure.                                                           |
 * |   852.0  | Change GetShaderFunctionCode to accept a StringView, this is needed for the refactor                   |
 * |          | of ShaderLibraryCreateInfo                                                                             |
 * |   851.0  | Compact TriangleRasterStateParams structure.                                                           |
 * |   850.0  | Add override1024VgprsWithGranularity16 to DeviceProperties                                             |
 * |   849.0  | Add Resize interface to ISwapChain. Reserve three bits for swapInterval in PresentSwapChainInfo.flags. |
 * |   848.0  | Compact InputAssemblyStateParams structure.                                                            |
 * |   847.0  | Replace Offset2d in MsaaQuadSamplePattern with smaller SampleLocation struct.                          |
 * |   846.0  | Remove the following "gfx11" cmake interface macros but not PAL_BUILD_GFX11 itself:                    |
 * |          |   PAL_BUILD_NAVI3X, PAL_BUILD_NAVI31, PAL_BUILD_NAVI32, PAL_BUILD_NAVI33, PAL_BUILD_PHOENIX, and       |
 * |          |   PAL_BUILD_PHOENIX1. Client cmakes should set PAL_BUILD_GFX11 if they want those features.            |
 * |   845.0  | Remove the following "gfx10" cmake interface macros, this support is now controlled by PAL_BUILD_GFX9: |
 * |          |   PAL_BUILD_GFX10, PAL_BUILD_NAVI12, PAL_BUILD_NAVI14, PAL_BUILD_GFX103,                               |
 * |          |   PAL_BUILD_NAVI2X, PAL_BUILD_NAVI21, PAL_BUILD_NAVI22, PAL_BUILD_NAVI23, PAL_BUILD_NAVI24,            |
 * |          |   PAL_BUILD_REMBRANDT, PAL_BUILD_RAPHAEL, PAL_BUILD_MENDOCINO                                          |
 * |   844.0  | Add UpdateFrameTraceController() to palPlatform. Client must call once per frame.                      |
 * |   843.0  | Add offsets to the HIP trap handler base addresses                                                     |
 * |   842.0  | Rework to Graphics Pipeline Dynamic State structures. Changes reduce footprint as well as make it more |
 * |          | clear what dirty bits actually cover.                                                                  |
 * |   841.0  | Add support for work graphs draw nodes                                                                 |
 * |   840.0  | Adds SetMlPowerOptimization() to IDevice interface to allow clients to request certain power           |
 * |          | optimizations for DirectML/ROCm workloads.                                                             |
 * |   839.0  | Fix #ifdefs surrounding old interface for CmdDrawIndirectMulti and and CmdDrawIndexedIndirectMulti     |
 * |   838.0  | Change interface for CmdDispatchIndirect, CmdDrawIndirectMulti, CmdDrawIndexedIndirectMulti,           |
 * |          | CmdExecuteIndirectCmds, and CmdDispatchMeshIndirectMulti from IGpuMemory base addr + offset            |
 * |          | to a GPU virtual address                                                                               |
 * |   837.0  | Remove CmdDispatchDynamic support.                                                                     |
 * |   836.0  | Add the srcImageLayout parameter to CmdProcessFrameInfo.                                               |
 * |   835.0  | Add PipelineStagePostPrefetch and PipelineStageSampleRate. Add PipelineStageFlag versions of           |
 * |          | CmdSetEvent, CmdResetEvent, CmdWriteTimestamp, and CmdWriteImmediate.                                  |
 * |   834.0  | Remove p2pCopyToInvisibleHeapIllegal since this is a vega-specific workaround.                         |
 * |   833.0  | Change argument type of GetShaderFunctionStats() from char * to StringView<char>                       |
 * |   832.0  | Add BindPipelineValidation callback PM4 Instrumentor Layer to report pipelineCmdSize. Remove           |
 * |          | pipelineCmdSize from DrawDispatchValidation callback.                                                  |
 * |   831.0  | Removed PAL_INTERFACE_MINOR_VERSION and now PAL clients must always default-initialize all structs.    |
 * |   830.0  | Add FenceCreateInfo::flags::sharable for supporting sharing when backed by a WDDM monitored fence.     |
 * |          | Add QueueSemaphoreExportInfo::syncFdWaitValue and ExternalQueueSemaphoreOpenInfo::syncFdSignalValue to |
 * |          | communicate timeline values provided by applications when exporting or importing a timeline semaphore  |
 * |          | via a syncFd handle in libdxg builds.                                                                  |
 * |          | Expose the supportOpaqueFdSemaphore, supportSyncFileSemaphore, and supportSyncFileFence OS properties  |
 * |          | for libdxg builds.                                                                                     |
 * |          | Expose the GetSwapchainGrallocUsage, AssociateNativeFence, ImportSurfaceFlingerBufferFromFd, and       |
 * |          | SignalNativeFence functions in libdxg Android builds.                                                  |
 * |   829.0  | Deprecate Util::ListDir(). Instead use Util::CountFilesInDir() and Util::GetFileNamesInDir().          |
 * |   828.3  | Add TraceSession::ReportError function to log TraceSource errors as an RDF chunk.                      |
 * |   828.2  | Add AMDLOG interface for PAL                                                                           |
 * |   828.1  | Add aqlPacketList to queue private data and HipSetRuntimeState function for the HIP debugger           |
 * |   828.0  | Bump CMake minimum version to 3.21.                                                                    |
 * |   827.0  | Remove function list and count in ShaderLibraryCreateInfo, PAL should retrieve them from the given ELF.|
 * |          | Change return type of GetShaderLibFunctionList() from ShaderLibraryFunctionInfo* to                    |
 * |          | Util::Span<ShaderLibraryFunctionInfo> and remove GetShaderLibFunctionCount().                          |
 * |          | Add Util::StringView<char> symbolName to ShaderLibraryFunctionInfo and remove const char* pSymbolName. |
 * |   826.0  | GPU Work Graphs:  Add support for node input record sharing.                                           |
 * |   825.0  | Deprecate PAL_PREDICT_TRUE and PAL_PREDICT_FALSE. Use [[likely]] and [[unlikely]] instead.             |
 * |   824.3  | Add QueryResultOnlyPrimNeeded flag into QueryResultFlags for streamout stats query                     |
 * |   824.2  | Add UniqueId in ICmdBuffer.                                                                            |
 * |   824.1  | Add GetGpuInfo lookup helpers for IDs and info. Move AsicRevision and GfxIpLevel enums to palLib.h.    |
 * |   824.0  | Add tokenMask for SQTT in GpaSampleConfig.                                                             |
 * |   823.0  | Redefine DispatchInterleaveSize enum, primarily to add 1D_64 thread explicit value.                    |
 * |   822.1  | Add disableFreeMux flag into FullScreenFrameMetadataControlFlags as an output field.                   |
 * |   822.0  | Add CmdBufInfo::vidPnSourceId for DirectCapture, clients must set a valid value in this field when     |
 * |          | pDirectCapMemory is nullptr and privateFlip flag is set.                                               |
 * |   821.0  | Add new member adaptiveDummyClear in SwapChainCreateInfo.                                              |
 * |   820.0  | Add KMD-UMD interface change FLIP_INTERVAL_OVERRIDE, for KMD to request flip interval override from UMD|
 * |   819.6  | Add isCompressed flag to GpuMemoryDesc.                                                                |
 * |   819.5  | Add CompilerStackSizes to ShaderLibStats.                                                              |
 * |   819.4  | Add MetroHash::HashFunc for less overhead using MetroHash as a lookup key in other containers.         |
 * |   819.3  | Add NullGpuId::Default for tools which need to create a null device before enumerating them.           |
 * |   819.2  | Add numInstances and collaborationMode for VPE 1.1 interface.                                          |
 * |   819.1  | Added AMF client to ClientApi                                                                          |
 * |   819.0  | Add thread trace config stallAllSimds for TT 3.3.                                                      |
 * |   818.3  | Add thread trace token type flag RealTime for TT 3.3.                                                  |
 * |   818.2  | Add forceShaderRingToVMem to palPublicSettings to allow app detects for forcing the shaderRings into   |
 * |          | video memory instead of allowing them in system memory in cases of low vram.                           |
 * |   818.1  | Added IsRaytracingShaderDataTokenRequested()                                                           |
 * |   818.0  | Adds ac01WaNotNeeded to PalPublicSettings and enables the workaround by default.                       |
 * |   817.0  | Add ScaledCopyFlags.srcAsNorm for ScaledCopyImage, so OGLP can treat srcImage as UNORM format when     |
 * |          | there is no need to do gamma correction.                                                               |
 * |   816.1  | Expose optimal sharing ID.                                                                             |
 * |   816.0  | Add flag isGraphics in LibraryCreateFlags to support create graphics shader library.                   |
 * |          | Add ppShaderLibraries and numShaderLibrary in GraphicsPipelineCreateInfo to support create graphics    |
 * |          | pipeline from graphics shader library.                                                                 |
 * |          | Add new interface GetCodeObjectWithShaderType to query code object per shader stage.                   |
 * |   815.0  | Remove GWG input sharing port from GraphNodeSuccessorInfo and GraphOutputPortInfoEx, this was added    |
 * |          | to skip validation for the output limits on internal input sharing ports. The spec and runtime have    |
 * |          | been updated to include input sharing in the maximum output limits for each node.                      |
 * |   814.2  | Add support for VPE ToneMapping                                                                        |
 * |   814.1  | Add Strix Halo support                                                                                 |
 * |   814.0  | Rename Util::RemoveFilesOfDir to Util::RemoveFilesOfDirOlderThan to differentiate it from the new      |
 * |          | function Util::RemoveOldestFilesOfDirUntilSize                                                         |
 * |   813.0  | Add disableDccStateTracking to ImageCreateFlags to help DXCP harden itself against bad apps.           |
 * |   812.1  | Add UserDataMapping::SampleInfo to the Pipeline ABI. This is to avoid recompiling the pipeline which   |
 * |          | uses dynamic rasterization samples.                                                                    |
 * |   812.0  | Expose distributed compression tuning controls.                                                        |
 * |   811.1  | Update PAL_ENABLE_SYSTEM_EVENTS as public.                                                             |
 * |   811.0  | Add new member allow256KBSwizzleModes in PresentableImageCreateInfo. This allows clients to enable     |
 * |          | 256 KiB swizzle modes on GFX11                                                                         |
 * |   810.0  | Rework of IPerfExperiment's SpmTraceLayout, SpmCounterData, and client SPM parsing requirements.       |
 * |          | You now get sane data if the SPM ring wraps (last N samples) and PAL can return 32-bit SPM counters.   |
 * |   809.1  | Add PAL_EVENTS_CONFIG_FILE and PAL_EVENTS_BINARY_DIR to generate ETW events for client.                |
 * |   809.0  | Add Pal::TriState enum, add groupLaunchGuarantee to *PipelineCreateInfo (Compute/Graphics)             |
 * |   808.2  | Add RsFeatureType::delag.hotkeyInd and RsFeatureTypeBoost, RsFeatureInfo::boost.hotkeyInd              |
 * |   808.1  | Add support for AQL packet ID. aqlPacketIndex field was added in Pal::DispatchAqlParams structure      |
 * |   808.0  | Add support for 2D Dispatch Interleave types. Also add DeviceProperties cap bits to check for 1D       |
 * |          | and 2D Dispatch Interleave Support.                                                                    |
 * |   807.2  | Add support for Krackan1 ASIC                                                                          |
 * |   807.1  | Add new RPM-related BarrierReasons: ResolveImage, PerPixelCopy, GenerateMipmaps.                       |
 * |   807.0  | Add flags to support Dispatch PingPong Order on the Pipeline and CmdBuffer.                            |
 * |   806.1  | Add riscV support to SecurityIpProperties.                                                             |
 * |   806.0  | Add flags to support clear-on-allocating for Pal memary on Linux.                                      |
 * |   805.1  | Add DecodeBufferViewSrd and DecodeImageViewSrd methods to IDevice.                                     |
 * |   805.0  | Add UniquePresentKey to Pal::CmdPostProcessDebugOverlayInfo and PresentationModeData needed to         |
 * |          | identify the target of a Present operation so that debug layers can track frames-per-second or         |
 * |          | other statistics correctly when applications render to multiple displays or windows.                   |
 * |   804.0  | Add edgeRule to rasterizerState for run-time selection                                                 |
 * |   803.2  | Add a new interface CmdScaledCopyTypedBufferToImage. This interface can scaled copy typed buffer to 2D |
 * |          | image.                                                                                                 |
 * |   803.1  | Add CreatePrintLogger() and DestroyPrintLogger() to Util::DbgLoggerPrint, set PAL_ENABLE_LOGGING       |
 * |          | defaulted ON for all Debug builds and allowed to be forced ON for non-Debug builds                     |
 * |   803.0  | Added a new CmdAllocType, LargeEmbeddedDataAlloc                                                       |
 * |   802.0  | Add enablePresentThread to SwapChainCreateInfo.                                                        |
 * |   801.0  | Add support for drm format modifier when sharing images. Add GetModifiersList to obtain avaliable      |
 * |          | modifiers. Add GetModifierSubresourceLayout to get dcc and display dcc offset. Add GetModifierInfo to  |
 * |          | extract swizzle mode and dcc parameters from modifier.                                                 |
 * |   800.1  | Add supportCooperativeMatrix to gfxipProperties.                                                       |
 * |   800.0  | Add VideoDecodeFrameInfo.dynamicDpbTier3 and VideoDecodeFrameInfo.dbwIndex to support Unified Decode   |
 * |          | target feature for VCN5 IP                                                                             |
 * |   799.0  | Add enableVmAlwaysValid to public settings.                                                            |
 * |   798.1  | Add QueryResultPreferShaderPath to QueryResultFlags.                                                   |
 * |   798.0  | Add compressedFormatEn to Pal::BvhInfo. This is a new bvh T# descriptor bit in RTIP3.1. Also added     |
 * |          | RtIp3_1 in RayTracingIpLevel.                                                                          |
 * |   797.0  | Add a type parameter to IPipeline::GetStackSizeInBytes().                                              |
 * |   796.1  | Exposed maxScratchRingSizeBaseline and maxScratchRingSizeScalePct as public settings.                  |
 * |   796.0  | Implement linux query reset status in CheckExecutionState(), cancel CheckExecutionState()              |
 * |          | const attribute.                                                                                       |
 * |   795.0  | Add frameGenCaps in osProperties struct and revise DirectCaptureInfo and CmdBufInfo struct to support  |
 * |          | DirectCapture frame generation feature.                                                                |
 * |   794.1  | Add rdWrMask to PerfCounterInfo and PerfCounterId umc structs to allow separate collection of          |
 * |          | read/write UMC counters.                                                                               |
 * |   794.0  | Add flag turboSyncEnabled to PresentSwapChainInfo::flags.                                              |
 * |   793.7  | Add decodeCodecPolicy to VideoInstanceInfo                                                             |
 * |   793.6  | Add new flag gpuVirtualization in pciProperties structure.                                             |
 * |   793.5  | Add new state vertexBufferCount in DynamicGraphicsState                                                |
 * |   793.4  | Add gpuPerformanceCapacity to DeviceProperties                                                         |
 * |   793.3  | Add new state dualSourceBlendEnable in DynamicGraphicsState                                            |
 * |   793.2  | Add SetSdiRemoteBusAddress function to IGpuMemory.                                                     |
 * |   793.1  | Add maxComputeThreadGroupCount[XYZ] to DeviceProperties::gfxipProperties                               |
 * |   793.0  | Add define PAL_BUILD_SUPPORT_DEPTHCLAMPMODE_ZERO_TO_ONE. Most clients don't need                       |
 * |          | DepthClampMode::ZeroToOne. Compiling it out will allow for optimized uCode in some hardware.           |
 * |   792.3  | Add pwsMode in PalPublicSettings struct and remove it from PAL internal settings.                      |
 * |   792.2  | Enable shader float atomic operations for NAVI3.                                                       |
 * |   792.1  | Add binningMode, customBatchBinSize and binningMaxPrimPerBatch in PalPublicSettings struct and remove  |
 * |          | them from PAL internal settings.                                                                       |
 * |   792.0  | Reduce MaxUserDataEntries from 144 to 136 for DXCP.                                                    |
 * |   791.2  | Update shader float atomic operations for NAVI3.                                                       |
 * |   791.1  | Add eRevId (emulation/internal revision ID) to DeviceProperties.                                       |
 * |   791.0  | Add WaitForPagingFence function for IQueue. Client can wait for paging fence by GPU.                   |
 * |   790.1  | Add limitCbFetch256B to PalPublicSettings.                                                             |
 * |   790.0  | Remove maxWorkGraphShaderPayloadAndSharedMemory limit.                                                 |
 * |   789.0  | Deprecate cuEnableMask in DynamicGraphicsShaderInfo. Use maxWavesPerCu for tuning.                     |
 * |   788.1  | Add SetRwxFilePermissions to modify file permissions for writting linux logs.                          |
 * |   788.0  | Add sortTrianglesFirst to PAL::BvhInfo. This is used in both RTIP3.1 and RTIP3.0.                      |
 * |          | Add compressedFormatEn to Pal::BvhInfo. This is a new bvh T# descriptor bit in RTIP3.1.                |
 * |   787.4  | Add hwsEnabled flag in struct GpuEngineProperties perEngine[] to inform clint when HWS is enabled.     |
 * |   787.3  | Added supportFloat32BufferAtomicAdd and supportFloat32ImageAtomicAdd                                   |
 * |   787.2  | Add RsFeatureTypeRtBoost type to RsFeatureType; Add rtBoost struct to RsFeatureInfo;                   |
 * |          | Add rtBoost support.                                                                                   |
 * |   787.1  | Initialize m_memoryProperties.busAddressableMemSize also for PAL_CLIENT_OGL                            |
 * |   787.0  | Include WsiPlatform into debug overlay visual confirm.                                                 |
 * |   786.0  | Define NOMINMAX to undefine the min/max macros defined in Windows.h                                    |
 * |          | Move _CRT_SECURE_NO_WARNINGS and WIN32_LEAN_AND_MEAN from PUBLIC to PRIVATE.                           |
 * |   785.1  | Undefine PAL_WEAK_LINK since it can be replaced with inline.                                           |
 * |          | Undefine PAL_NO_RETURN since it can be replaced with [[noreturn]].                                     |
 * |          | Undefine PAL_NODISCARD since it can be replaced with [[nodiscard]].                                    |
 * |          | This is a minor version bump because no client was using these macros anyway.                          |
 * |   785.0  | Remove timedate based BuildUniqueId and replace with 'BuildId' and 'GetCurrentLibraryBuildId()'. This  |
 * |          | also removes the requirement to always rebuild certain shader-cache related files.                     |
 * |   784.1  | Add GetEnabledCallbackTypes() and SetEnabledCallbackTypes() to IPlatform to selectively and dynamically|
 * |          | enable DeveloperCb callback types. Also add Util::BitfieldSetBit()                                     |
 * |   784.0  | Deprecate Util::AreSameScopedEnum, and Util::OneIsScopedEnumOneIsIntegral                              |
 * |   783.1  | Add maxGsInvocations which reports the maximum of number of GS prim instances.                         |
 * |   783.0  | Deprecate Util::BoolConstant, Util::Negation, Util::Conjunction, and Util::Disjunction                 |
 * |   782.7  | Created new RGD interface ICmdBuffer::CmdInsertExecutionMarker().                                      |
 * |          | Replacement of interface deprecated in 780.0.                                                          |
 * |   782.6  | Remove unused interface CmdCopyImageToPackedPixelImage().                                              |
 * |   782.5  | Added support3dUavZRange to gfxipProperties.                                                           |
 * |   782.4  | Add ClaimGpuMemory() and CheckIfOpenMemory() to Util::BuddyAllocator.                                  |
 * |   782.3  | Add uniqueId to GpuMemoryDesc.                                                                         |
 * |   782.2  | Updated VideoResourceDescriptor to include encode metadata                                             |
 * |   782.1  | Report GWG Scheduler version to clients.                                                               |
 * |   782.0  | Updating RsFeatureInfo to support AntiLag-Next parameters.                                             |
 * |   781.3  | Add ClearColorImageFlags::ClearColorSkipIfSlow to potentially issue fast clears as an optimization.    |
 * |   781.2  | Added instanceGroupSize to GpuBlockPerfProperties to help tools configure more DfMall counters on GPUs |
 * |          | that have finer grained DF counter modules.                                                            |
 * |   781.1  | Add a new enum in ApplicationProfileClient named FreeMux for Free Mux whitelist.                       |
 * |   781.0  | Add useLateAllocGsLimit and lateAllocGsLimit to GraphicsPipelineCreateInfo to set limit per-pipeline.  |
 * |   780.2  | Overloads Vsnprintf and Snprintf for wide character strings.                                           |
 * |          | Move the mode of a file in File::Open into a templated function OpenFileMode to avoid duplicated code. |
 * |          | Convert UTF-8 filename to w_char and call the 'W' version windows function.                            |
 * |   780.1  | Add support for VK_EXT_physical_device_drm to query DRM node properties on linux.                      |
 * |   780.0  | Deprecate old implementation of RGD. Deprecated interfaces: CmdInsertExecutionMarker(),                |
 * |          | CmdBufferBuildFlags::enableExecutionMarkerSupport. Removed: BuildExecutionMarker(), marker_payload.h.  |
 * |   779.1  | Replace the const char* with const StringView<char>& in EncodeAsFilename.                              |
 * |   779.0  | Add EarlyPresent event and add EarlyPresent event pointer to MultiSubmitInfo.                          |
 * |   778.2  | Add enableSqttMarkerEvent to PalPublicSettings.                                                        |
 * |   778.1  | Add isDataCenterBoard flag to DeviceProperties::osProperties::flags for unified VDI/CG driver.         |
 * |   778.0  | Remove interface CmdSetColorWriteMask and CmdSetRasterizerDiscardEnable.                               |
 * |          | Add dynamicState in DynamicGraphicsShaderInfos to support dynamic graphics states in CmdBindPipeline.  |
 * |   777.2  | Add expandHiZRangeForResummarize to PalPublicSettings.                                                 |
 * |   777.1  | Decrease MaxPayloadSize from 256 to 254 to reserved size for NOP packet header.                        |
 * |   777.0  | Move public setting gfx11SampleMaskTrackerWatermark to private setting.                                |
 * |   776.3  | Add RpmViewsBypassMall to PalPublicSettings.                                                           |
 * |   776.2  | Added a flag to CmdClearColorImage to allow clients to force slow clears for debug purposes.           |
 * |   776.1  | Add cpUcodeVersion and pfpUcodeVersion to gfxipProperties.                                             |
 * |   776.0  | Changed Pal::DeviceProperties to report virtualized Shader Engine information instead of physical.     |
 * |   775.0  | Remove return values from MsgPackWriter Pack calls.                                                    |
 * |   774.0  | Add QueryReleaseVersion to Pal::Device for retrieving the ReleaseVersion string                        |
 * |          | Remove unused QueryDriverVersion interface                                                             |
 * |   773.1  | Add new flag supportFreeMux in DeviceProperties::osProperties::flags.                                  |
 * |   773.0  | Add instance id in RequestReleaseVideoBandwidthParams.                                                 |
 * |   772.0  | Add pPageFaultStatus output argument to Pal::IDevice::CheckExecutionState                              |
 * |          | Add supportPageFaultInfo flag to gpuMemoryProperties                                                   |
 * |   771.2  | Change how Amdgpu handles WindowSystem-busy BOs presenting in per cmdBuffer submission BO list. Print  |
 * |          | alerts instead of removing the BO from the list.                                                       |
 * |   771.1  | Add 1.5 to RayTracingIpLevel                                                                           |
 * |   771.0  | Roll up all CmdDispatch* x/y/z group counts into a DispatchDims struct. Extend CmdDispatchOffset so    |
 * |          | that the caller can customize the thread group count seen by shaders that use the thread group count   |
 * |          | metadata. See CmdDispatchOffset's comments for more information.                                       |
 * |   770.1  | Add rbPlusOptimizeDepthOnlyExportRate to pal public settings.                                          |
 * |   770.0  | Add new PipelineStageFlag PipelineStageStreamOut                                                       |
 * |   769.0  | Add new member allow256KBSwizzleModes in ImageCreateFlags. This allows clients to enable 256 KiB       |
   |          | swizzle modes on GFX11                                                                                 |
 * |   768.0  | Add new member clientBlockIfFlipping in SwapChainCreateInfo. This makes swapchain offload block        |
 * |          | if flipping (write primary) responsibility to client.                                                  |
 * |   767.1  | Add new enum values in SpmDataSegmentType.                                                             |
 * |   767.0  | Add new members srcStageMask/dstStageMask in MemBarrier and ImgBarrier. This alllows clients to pass   |
 * |          | per buffer or image transition stageMask and PAL can do potential more optimizations like skip read    |
 * |          | only transitions.                                                                                      |
 * |   766.0  | Changed Pal::GpuMemoryHeapProperties::heapSize to logicalSize.                                         |
 * |          | Changed Pal::GpuMemoryHeapProperties::physicalHeapSize to physicalSize.                                |
 * |          | Add Pal::DeviceProperties::gpuMemoryProperties::barSize.                                               |
 * |          | The philosophy behind these changes is to enforce clients to reexamine which variables they're using.  |
 * |          | In almost all circumstances, they should be using logicalSize over physicalSize.                       |
 * |          | barSize is to provide a clearer way for clients to detect ReBAR enablement independent of heap sizes.  |
 * |   765.0  | Add PlatformCreateInfo::disableDevDriver.                                                              |
 * |   764.2  | PAL now records the detailed shader mask into the RGP api_info chunk when the tracing mode is set to   |
 * |          | full_frame.                                                                                            |
 * |   764.1  | Add pFreeMuxMemory to MultiSubmitInfo.                                                                 |
 * |   764.0  | Add PresentPrivateInfo to PresentDirectInfo.                                                           |
 * |   763.0  | Move zppAreaThreshold from PublicSettings to CmdBufferBuildInfo.                                       |
 * |          | Add contextStatesPerBin and contextStatesPerBin to CmdBufferBuildInfo.                                 |
 * |   762.0  | Add format parameter for the clear color of CmdClearColorImage().                                      |
 * |   761.0  | Add startVaHintFlag in GpuMemoryCreateFlags to set startVaHint in GpuMemoryCreateInfo as baseVirtAddr. |
 * |   760.2  | Add zppAreaThreshold to PalPublicSettings.                                                             |
 * |   760.1  | Add IsCrashAnalysisModeEnabled function to Pal::IPlatform.                                             |
 * |   760.0  | Add disableZpp option to pipeline creation to control ZPP optimization.                                |
 * |   759.0  | Change IPlatform::QueryRawApplicationProfile, IPlatform::EnableSppProfile and                          |
 * |          | gpuUtil::QueryAppContentDistributionId to use wchar_t* as input instead of char* to remove             |
 * |          | unnecessary conversion back and forth.                                                                 |
 * |   758.0  | Add cpu Family and model info to SystemInfo.                                                           |
 * |   757.1  | Made the cache access masks in all ICmdBuffer barrier building functions more permissive. In short,    |
 * |          | it's legal (and encouraged) to include read-only cache flags in the source cache masks. For example,   |
 * |          | CmdReleaseThenAcquire can optimize out cache ops if it knows a prior barrier targeted shader reads.    |
 * |   757.0  | Add disableQueryInternalOps to CmdBufferBuildFlags to control query pools based on internal ops or     |
   |          | client.                                                                                                |
 * |   756.2  | Add supportsClearCopyMsaaDsDst to DeviceProperties to indicate whether each engine supports CmdClear   |
 * |          | and CmdCopy with MSAA depth-stencil destination.                                                       |
 * |   756.1  | Add ThreadLaunch type for GPU work graph nodes.                                                        |
 * |   756.0  | Remove is_scoped param from AmdWorkGraphsOutputCommit.                                                 |
 * |   755.1  | Add supportMsFullRangeRtai to DeviceProperties.                                                        |
 * |   755.0  | Add clientTessDistributionFactors to CmdBufferBuildInfo for easier tessellation distribution factor    |
 * |          | optimization. Adds optimizeTessDistributionFactors to CmdBufferBuildFlags for clients to indicate      |
 * |          | custom factors were provided. Deprecates tess distribution factor public settings added in 749.3.      |
 * |   754.0  | Add an option to control whether the primary node is opened in Pal.                                    |
 * |   753.0  | Rename disableBinningPsKill::True and disableBinningPsKill::False to OverrideMode::Enabled             |
 * |          | and OverrideMode::Disabled.                                                                            |
 * |   752.0  | Split up Pal DeviceProperty supportMeshShader into an explicit supportMeshShader and supportTaskShader.|
 * |   751.0  | Remove BarrierInfo.pSplitBarrierGpuEvent/flags.splitBarrierEarlyPhase/flags.splitBarrierLatePhase.     |
 * |   750.1  | Update PalPublicSettings struct with nggLateAllocGs to allow clients to optimize the value for late    |
 * |          | alloc GS per-app.                                                                                      |
 * |   750.0  | Add sqWgpShaderMask to GpaSession and IPerfExperiment's input structs. Prior to this version,          |
 * |          | sqShaderMask set the shader stage masks for Sq and SqWgp to the same settings.                         |
 * |   749.3  | Update PalPublicSettings struct to accommodate the isoline, triangle, quad, donut, and trapezoid dist  |
 * |          | factors.                                                                                               |
 * |   749.2  | Add support for node shader outputs to share their output record budget with another output.           |
 * |   749.1  | Adds Util::InRange inline function, as well as various MsgPackWriter::Pack* functions that allow for   |
 * |          | result chaining.                                                                                       |
 * |   749.0  | For performance optimization tuning purpose: Add disableBinningPsKill in PalPublicSettings struct and  |
 * |          | remove it from PAL internal settings.                                                                  |
 * |   748.0  | Add secureProcessorType to VideoDecodeFrameInfo.                                                       |
 * |   747.0  | Require all clients to pass patchControlPoints for InputAssemblyStateParams in Gfx12.                  |
 * |   746.1  | Add gl1cSizePerSa, instCacheSizePerCu, scalarCacheSizePerCu, and activePixelPackerMask to              |
 * |          | DeviceProperties.                                                                                      |
 * |   746.0  | Add Storage engine, kmdShareUmdSysMem, and deferCpuVaReservation support.                              |
 * |          | Add AtomicDecrement64.                                                                                 |
 * |          | Bump Pal::GpuMemoryCreateFlags from uint32 to uint64.                                                  |
 * |   745.0  | Add primitiveRestartMatchAllBits to InputAssemblyStateParams.                                          |
 * |   744.2  | Add supportsCmdPresent to queueProperties.                                                             |
 * |   744.1  | Add bitstreamPaddingSizeCbc1Cbcs to VcnIpProperties. For required bitstream padding for CBCS/CBC1.     |
 * |   744.0  | For performance optimization tuning purpose : Add binningPersistentStatesPerBin and                    |
 * |          | binningContextStatesPerBin in PalPublicSettings struct and remove them from PAL internal settings, add |
 * |          | LdsPsGroupSizeOverride in GraphicsPipelineCreateInfo struct.                                           |
 * |   743.1  | Add mallSizeInBytes to GpuChipProperties.                                                              |
 * |   743.0  | Add release point and wait point aliases HwPipePreIndexBuffer/HwPipePostIndexBuffer for index buffer   |
 * |          | fetch, clients should use the two to release or wait index buffer safely.  To avoid confusion, rename  |
 * |          | HwPipePostIndexFetch to HwPipePostPrefetch and public setting forceWaitPointPreColorToPostIndexFetch   |
 * |          | to forceWaitPointPreColorToPostPrefetch.                                                               |
 * |   742.1  | Add wptrGranularity to the SpmTraceLayout struct to track number of bytes in each wptr increment.      |
 * |   742.0  | Add ScaledCopyFlags.DstAsNorm for ScaledCopyImage, so OGLP can treat dstImage as UNORM format when     |
 * |          | there is no need to do degamma correction.                                                             |
 * |   741.0  | Add disableDccRejected field in CmdBufInfo.                                                            |
 * |   740.1  | Add VCN maximum clock info to VideoInstanceInfo.                                                       |
 * |   740.0  | Split CoherShader to CoherShaderRead/CoherShaderWrite, CoherCopy to CoherCopySrc/CoherCopyDst,         |
 * |          | and CoherResolve to CoherResolveSrc/CoherResolveDst for optimal barrier operations.                    |
 * |   739.2  | Add driver store path in DeviceProperties for Windows.                                                 |
 * |   739.1  | Add ResolveImageFlags.DstAsSrgb and ResolveImageFlags.DstAsNorm for ResolveImageCompute, so OGLP can   |
 * |          | do gamma correction as needed.                                                                         |
 * |   739.0  | Add DXGI Stereo support.                                                                               |
 * |   738.0  | Add Util::ShaderFlags param to Util::ICacheLayer::Store. Depends on SCPC_INTERFACE_MAJOR_VERSION 130.  |
 * |   737.2  | Add QueryAllocationInfo to IWorkGraph.                                                                 |
 * |   737.1  | Exposed GPU Work Graphs related limits for node shaders in Pal::DeviceProperties.                      |
 * |   737.0  | Interface changes required for GPU Work Graphs.                                                        |
 * |   736.0  | Add highPrecisionBoxNode, wideSort and hwInstanceNode to Pal::BvhInfo.                                 |
 * |   735.2  | Add numActiveRbs to DeviceProperties::gfxipProperties::shaderCore.                                     |
 * |   735.1  | Add 'SetStaticVmidMode' to acquire/release a static VMID on KMD3                                       |
 * |   735.0  | Split supportFloat32Atomics flag into supportFloat32BufferAtomics and supportFloat32ImageAtomics.      |
 * |   734.3  | Add a flag to CmdBeginBinnablePass args to condition CmdBindTargets optimization.                      |
 * |   734.2  | Report RSync feature support for Streaming SDK project.                                                |
 * |   734.1  | Add functions to create DbgLoggerFile and to get current library name.                                 |
 * |   734.0  | Add clientApiId to PlatformCreateInfo (replaces dxClientId).  All clients are expected to pass this.   |
 * |   733.0  | Add flags for clipMask and cullMask in RasterizerState for register setting use.                       |
 * |   732.0  | Move some file/path-specific constants into palFile.h                                                  |
 * |   731.0  | Make GpuMemSubAllocInfo no longer require a pointer to IGpuMemory* objects.                            |
 * |   730.2  | Add CmdResolveEncoderOutputMetadata and CmdCopyEncoderErrorCode                                        |
 * |   730.1  | Add CmdSaveGraphicsState and CmdRestoreGraphicsState.                                                  |
 * |   730.0  | Add flag 'lowZplanePolyOffsetBits' to use decreased precision for Z_16/Z_24 formats.                   |
 * |   729.4  | Debug log code refactor and creation of a new logger type: DbgLoggerPrint.                             |
 * |   729.3  | Add 'CheckSequential' helper.                                                                          |
 * |   729.2  | Add new function IsTracingEnabled to palplatform                                                       |
 * |   729.1  | Add a few funtions into IDevice Interface for OpenGL GenLock API.                                      |
 * |   729.0  | Add pReset parameter to Device::FlglGetFrameCounter in order to report the counter reset state.        |
 * |   728.0  | Export IPipeline::GetStackSizeInBytes().                                                               |
 * |   727.0  | Remove public setting useAcqRelInterface.                                                              |
 * |   726.0  | Add decodeMode in VideoDecodeFrameInfo to support low latency video decode.                            |
 * |   725.0  | Add freeMemory to ICmdAllocator::Reset, which frees everything the allocator has allocated.            |
 * |   724.0  | Switch to KMD Framelock and remove all now unused UMD Framelock code                                   |
 * |   723.1  | Add settings RPC support. Also adds new field .ps_sample_mask to code object metadata.                 |
 * |   723.0  | Move privateScreen from internal create info into GpuMemoryCreateFlags                                 |
 * |   722.0  | Add videoDecoder flag in ImageUsageFlags.                                                              |
 * |   721.0  | Change depthBiasEnable to frontDepthBiasEnable and backDepthBiasEnable to allow more fine-grained      |
 * |          | control of polygon offset feature (needed for OpenGL).                                                 |
 * |   720.0  | Split supportFloatAtomics flag into 2 flags of 32 and 64 bit types                                     |
 * |   719.1  | Add supportImageViewMinLod to DeviceProperties                                                         |
 * |   719.0  | Remove public setting memMgrPoolAllocationSizeInBytes.                                                 |
 * |   718.0  | Add flipIntervalOverride parameter to support DXXP VSyncControl feature.                               |
 * |   717.0  | Add Util::AssumeAligned. StringBag uses StringView for adding and retrieving strings.                  |
 * |   716.0  | Add a public setting memMgrPoolAllocationSizeInBytes for client to control.                            |
 * |          | Note: default value is 0, which keep to use Pal default value.                                         |
 * |   715.0  | Remove Util::MemoryBarrier and Util::FlushCpuWrites. Clients should use std atomics instead.           |
 * |          | This fixes a build issue on Windows (MemoryBarrier is a winnt.h macro). PAL also shouldn't define      |
 * |          | its own memory barriers now that our coding standards permit use of std atomics.                       |
 * |   714.1  | Add SqWgp as a new GpuBlock enum in palPerfExperiment.h                                                |
 * |   714.0  | Modified Progress to Running in TraceSessionState enum                                                 |
 * |   713.0  | Add a public setting disableInternalVrsImage for client not use vrs feature.                           |
 * |   712.0  | Modified signature of OnConfigUpdated method to parse Json data, based off of DevDriver's              |
 * |          | StructuredValue object                                                                                 |
 * |   711.0  | Add expectedEntries parameter to make the bucket number of MemoryCacheLayer's entry lookup hash table  |
 * |          | can be changed flexibly                                                                                |
 * |   710 0  | Add Multi-media affinity to MultiSubmitInfo.                                                           |
 * |   709.0  | Add topologyIsPolygon to topologyInfo struct. It indicates that triangle primitives are combined to    |
 * |            represent more complex polygons                                                                        |
 * |   708.0  | Add RgpMarkerSubQueueFlags to DrawDispatchInfo and CmdInsertRgpTraceMarker to control which            |
 * |          | sub-queue(s) the marker will be written to.                                                            |
 * |   707.1  | Add BitIter and UnsetLeastBit helpers for iterating through bitmasks                                   |
 * |   707.0  | Add autoTrimMemory flag to CmdAllocatorCreateFlags and allocFreeThreshold to CmdAllocatorCreateInfo.   |
 * |          | This enables and controls the amount of (automatic) memory trimming in CmdAllocator.                   |
 * |          | The added Trim() function is for explicit trimming of CmdAllocator.                                    |
 * |   706.0  | Add public setting to set initial DCC state and ExpandByteTo* helpers                                  |
 * |   705.1  | Add PAL_CONSTEXPR_ASSERT_MSG and PAL_CONSTEXPR_ASSERT.                                                 |
 * |   705.0  | Add DX12 cmdlist handle for batched markers to CmdBuffer create info                                   |
 * |   704.0  | Deprecate Util::EnableIf and Util::UnderlyingType                                                      |
 * |   703.0  | Add nativePolicy to AspDecryptKeys struct needed for native policy enforcement                         |
 * |   702.0  | Add frameLatency to SwapChainCreateInfo.                                                               |
 * |   701.2  | Added SecurityIpProperties::hmpSupport flag indicating whether HMP HWDRM is supported or not.          |
 * |   701.1  | Add debug logging support, phase 2. Has DevDriver logger support to log msgs to a connected tool.      |
 * |   701.0  | Add support for MALL SPM. This includes adding dfSpmTraceBegin and dfSpmTraceEnd to the CmdBufInfo     |
 * |          | struct to begin and end a trace, as well as the CmdCopyDfSpmTraceData function to copy the results     |
 * |          | to an accessible buffer, the dfSpmTrace flag in PerfExperimentDeviceFeatureFlags to tell if a device   |
 * |          | supports DF SPM, the DfSpmTraceMetadataLayout to store the metadata returned, the AddDfSpmTrace        |
 * |          | function to add a DF SPM trace to an experiement, the dfSpmPerfCounters struct to GpaSampleConfig to   |
 * |          | add DF SPM info to a GpaSession, the GpuUtil::GpaSession::CopyDfSpmTraceResults to copy the DF SPM     |
 * |          | result buffer to the GpaSession result buffer and finally GpuUtil::GpaSession::AppendDfSpmTraceData to |
 * |          | add the DF SPM data to an RGP file.                                                                    |
 * |   700.0  | Add threadsPerGroup to ComputePipelineCreateInfo for HSA ABI pipelines, set it to zero for PAL ABI.    |
 * |   699.0  | Remove OSS1 and OSS2 functionality                                                                     |
 * |   698.1  | Util::Vector can now move elements in/out and construct in-place with EmplaceBack.                     |
 * |   698.0  | Add CmdBufInfo::vpBltExecuted to support sending ScenarioInfo to KMD using VCOP                        |
 * |              in GFXIPSubmit when VPBLT is called.                                                                 |
 * |   697.0  | Change BuildCopyData to copy data on PFP for HwPipeTop and on ME for HwPipePostIndexFetch in           |
 * |          | CmdWriteImmediate. Besides, it extends the HwPipeTop behavior in CmdWriteImmediate to                  |
 * |          | HwPipePostIndexFetch and HwPipePreCs correspondingly and changes the HwPipeTop to HwPipePostIndexFetch |
 * |          | in the CmdWriteTimestamp.                                                                              |
 * |   696.0  | Add globalGpuVa to ExternalResourceOpenInfo                                                            |
 * |   695.0  | Add frameIndex field in CmdBufInfo for DirectCapture feature                                           |
 * |   694.2  | Add debug logging support, phase 1. Has logging infrastructure support through DbgLogMgr class.        |
 * |   694.1  | Add QueryCurrentDisplayMode in Pal::IScreen.                                                           |
 * |   694.0  | Add imageMemoryBudget in Pal::ImageCreateInfo to allow client's control of texture allocation size.    |
 * |          | This is only valid for gfx9 and gfx9+. Tests show that 1.5 can reduce texture allocation size a lot.   |
 * |          | However, 0.0 (legacy behavior) is recommended to not cause any performance delta on exisiting apps.    |
 * |   693.1  | Add ICmdBuffer::CmdCopyMemoryByGpuVa.                                                                  |
 * |   693.0  | Add GraphicsPipelineCreateInfo::depthClampMode to be able to set one of three depth clamping modes:    |
 * |          | clamp to viewport bounds, clamp to [0.0, 1.0] interval or clamping disabled.                           |
 * |   692.0  | Add different sample count MSAA resource copy support in GPU UTIL                                      |
 * |   691.2  | Add SwizzledFormat in Pal::SubresLayout for msaa copy image gpu util                                   |
 * |   691.1  | Add VcnInstanceInfo in Pal::VcnIpProperties for codec support per Vcn instance                         |
 * |   691.0  | Add PalPublicSetting::disableExecuteIndirectAceOffload to disable the IndirectCmdGeneration using the  |
 * |          | ACE optimization.                                                                                      |
 * |   690.0  | Add noSignalOnDeviceLost flag in QueueSemaphoreCreateInfo::flags                                       |
 * |   689.0  | Added 12-bit multimedia formats and renamed existing 10-bit X16*_MM_* formats to X16*_MM10_*.          |
 * |   688.0  | Changed IsSetValueAvailable to take an argument (setting name hash) to check on individual settings.   |
 * |          | Added IsSetAllowedInDriverRunningState to check setting modifiability in driver-running state.         |
 * |   687.4  | Add 'HexValue' to JsonWriter                                                                           |
 * |   687.3  | Added Util::IsDebuggerAttached                                                                         |
 * |   687.2  | Added a new interface for arbitrary data capture through PAL traces. Implemented basic data I/O        |
 * |          | functionality in TraceSession class.                                                                   |
 * |   687.1  | Add supportsBorderColorSwizzle flag to gfxipProperties to indicate that the device supports            |
 * |          | image swizzle with custom border color                                                                 |
 * |   687.0  | Add support for CBC stripe information                                                                 |
 * |   686.3  | Put the light shaft optimization inside a feature specific define                                      |
 * |   686.2  | Add RsFeatureTypeProVsr type to RsFeatureType; Add provsr struct to RsFeatureInfo; Add ProVsr support. |
 * |   686.1  | Add light shaft optimizations to GFX11.5 builds.                                                       |
 * |   686.0  | Remove PresentDirectInfo's MgpuSlsInfo. It was only implemented on WDDM1 so it's dead code now.        |
 * |   685.2  | Add support for DXGI swapchain for Vulkan. Add Dxgi to WsiPlatform and DXGI specific flags to          |
 * |          | SwapChainCreateInfo flags.                                                                             |
 * |   685.1  | Add flags union to struct GpuMemoryRequirements.                                                       |
 * |   685.0  | Add 64-bit platform-agnostic Util::File::Status wrapper to _stat64 and stat.                           |
 * |   684.0  | Add preferredPresentMode to SwapChainProperties                                                        |
 * |   683.0  | Remove PAL_BUILD_GFX10 from cmake. Clients must remove references to it and act like it's enabled.     |
 * |   682.0  | Remove PowerXpress support.                                                                            |
 * |   681.4  | Add Util::Result helpers for errno and WinError, plus an error code for 'disk full'.                   |
 * |   681.3  | Add native support for HSA code objects in PAL's compute pipeline logic. Mostly this adds new support  |
 * |          | to existing functions (see palCmdBuffer.h's changes). It also adds ICmdBuffer::CmdSetKernelArguments,  |
 * |          | the supportHsaAbi flag in DeviceProperties, the hsaAbi flag in PipelineInfo, the HsaAbi metadata       |
 * |          | types, IPipeline::GetKernelArgument, and some minor helper ELF and string functions. Support is        |
 * |          | limited to gfx10.3 because it requires certain CP packet support which is very new. There are certain  |
 * |          | HSA features that are not supported today (scratch, device enqueue, gpu printf, etc.).                 |
 * |   681.2  | Add supportFloatAtomics flag to DeviceProperties                                                       |
 * |   681.1  | Add ResourceCorrelation event to PalEvents                                                             |
 * |   681.0  | Add Mesa metadata support for external shared BO                                                       |
 * |   680.0  | Add PalPublicSetting::enableExecuteIndirectPacket to control whether to use the new CP packet or       |
 * |          | Indirect CmdGeneration shaders for the current ExecuteIndirect operation.                              |
 * |   679.0  | For enabling Native CENC added next fields to VideoDecodeFrameInfo struct: pSubsampleBuffer,           |
 * |          | subsampleBufferSize, subsampleBufferOffset and enableNativeCenc.                                       |
 * |   678.1  | Add encodeCodecPolicy struct to support encode queue based on codec fuse settings and instance         |
 * |          | harvesting.                                                                                            |
 * |   678.0  | Remove 12on7 support                                                                                   |
 * |   677.1  | Add a new CacheCoherencyUsageFlags CoherPresent.                                                       |
 * |   677.0  | Add DirectCapture pre-flip access support. Update DirectCaptureInfo and add privPrimary flag in        |
 * |          | GpuMemoryCreateFlags to support DirectCapture pre-flip access and private flip.                        |
 * |   676.0  | Created PalAbi namespace, moved palPipelineAbiMetadata and non-generic methods to new namespace.       |
 * |   675.0  | Remove PAL_INLINE.                                                                                     |
 * |   674.1  | Add a new enum in ApplicationProfileClient named RisWindowed for Ris whitelist.                        |
 * |          | Also add other two enums named DutyCycleScaling and ProBoost to make the ApplicationProfileClient been |
 * |          | aligned with SHARED_AP_AREA.                                                                           |
 * |   674.0  | Device::SetClockMode() method now outputs clk frequencies instead of ratios                            |
 * |   673.1  | Add PAL_CPLUSPLUS for the C++ standard PAL is compiled with.                                           |
 * |   673.0  | Add CmdDispatchDynamic for dynamic compute pipeline launch support                                     |
 * |   672.1  | Add support for TwoDRectList PrimitiveTopology for GfxIp9 and onwards, and add support2DRectList       |
 * |          | to DeviceProperties.                                                                                   |
 * |   672.0  | Retire Pre-Polaris and Pre-Raven NullGpuIds.                                                           |
 * |   671.0  | Removed interface IVideoDecoder::ConfigureVideoDecoderGpuMemory and VP9/AV1 defaults                   |
 * |   670.0  | Add rayTracingExectued to CmdBufInfo so KMD can be notified of ray tracing work.                       |
 * |   669.1  | Add CmdDispatchAce for creating a Dispatch packet from the Universal CmdBuffer on the ACE CmdStream to |
 * |          | support moving ExecuteIndirect's Cmd Generation workload to the ACE.                                   |
 * |   669.0  | Replace boolean PalPublicSettings::useGraphicsFastDepthStencilClear with enum                          |
 * |          | PalPublicSettings::fastDepthStencilClearMode for better readability and additional functionality for   |
 * |          | clients to specify if they want to prefer compute over graphics.                                       |
 * |   668.5  | Add ICmdBuffer::CmdSetRasterizerDiscardEnable().                                                       |
 * |   668.4  | Support monotonic raw clocks for GetPerfCpuTime.                                                       |
 * |   668.3  | Add IDevice::InitializeVideoDecoderGpuMemory interface to initialize Decoder heap context data.        |
 * |   668.2  | Add an option to define RayTracing IP Level.                                                           |
 * |   668.1  | Add legacy barrier and acquire/release interface functions to optimize provided pipeline stages and    |
 * |          | cache access flags using the tracked command buffer stage activity flags.                              |
 * |   668.0  | Add an option to select BVH box sort heuristic.                                                        |
 * |   667.2  | Add numSupportedDecodeCodecs to VcnIpProperties which report the number of supported decode codecs     |
 * |   667.1  | Add maxGsOutputVert and maxGsTotalOutputComponents field to DeviceProperties which report the maximum  |
 * |          | of  vertices output and total components.                                                              |
 * |   667.0  | Add more than one heap preference support to GpuMemory events in place of preferredHeap.               |
 * |   666.0  | Add ComputePipelineCreateInfo::interleaveSize and GraphicsPipelineCreateInfo::taskInterleaveSize. This |
 * |          | allows clients to control how many thread groups are sent to one SE before switching to the next one.  |
 * |   665.3  | Add ICmdBuffer::CmdSetColorWriteMask.                                                                  |
 * |   665.2  | Added supportInt4Dot and supportInt8Dot to DeviceProperties                                            |
 * |   665.1  | Add support for AV1 12-bit decoder.                                                                    |
 * |          | Add interface in IVideoDecoder to initialize Video Decoder HW Context Buffer in PAL                    |
 * |   665.0  | Add option dx10DiamondTestDisable in GraphicsPipelineCreateInfo::rsState                               |
 * |   664.2  | Add a function for clients to check before setting the dualSourceBlendEnable flag used during pipeline |
 * |          | creation.                                                                                              |
 * |   664.1  | Expose flag for SQ_IMAGE_GATHER4_L_O support                                                           |
 * |   664.0  | Refactor AHB(Android hardware buffer), the old design mixed fd and os dependent handle which may leads |
 * |          | to stack corruption and too much redundent logic. This change removes androidGpuMemory.h(.cpp) and     |
 * |          | a series of overrided AHB related functions in androidDevice.cpp.                                      |
 * |   663.0  | Add frame stack size for indirect shaders to MultiSubmitInfo.                                          |
 * |   662.1  | Add Video Encode Bandwidth Managment support                                                           |
 * |   662.0  | Add a public setting disableDebugOverlayVisualConfirm.                                                 |
 * |   661.1  | Add back event-based Acquire/release interface for Vulkan Sync2.                                       |
 * |   661.0  | Remove rootCmdBufferGpuScratchSuballocSize in CmdBufBuildInfo.                                         |
 * |   660.1  | Add the interface and support in PAL to prime cache.                                                   |
 * |   660.0  | Rename Util::GenericAllocatorAuto to Util::GenericAllocatorTracked.                                    |
 * |   659.1  | Add Util::CheckReservedBits<T>() as a helper for bitfields that should be kept in-sync.                |
 * |   659.0  | Revert 658. How PAL determines the values for elf code object header flags need to be updated before   |
 * |          | correct validation can be added.                                                                       |
 * |   658.0  | Add validation for PAL code object ELF header flags (specifically for EF_AMDGPU_FEATURE_XNACK_V4 and   |
 * |          | EF_AMDGPU_FEATURE_SRAMECC_V4 feature masks) when pipelines are initialized from ELF binaries.          |
 * |   657.0  | Add explicit sync flag to GpuMemoryCreateFlags to avoid kernel syncs on shared images.                 |
 * |   656.4  | Add surface address developer callback.                                                                |
 * |   656.3  | Expose flags enableTurboSyncForDwm and enableDwmFrameMetadata to client.                               |
 * |   656.2  | Add StringToValueTypeChecked.                                                                          |
 * |   656.1  | Add DDR5 related info to LocalMemoryType and SqttMemoryType.                                           |
 * |   656.0  | Updated comment for SubresId to reflect that three plane YV12 images data is expected to be in YVU     |
 * |          | order. This interface bump is intended to notify clients that the "official" order has changed, and    |
 * |          | to update any code that relied on the ordering. There are no functional changes.                       |
 * |   655.0  | Extend available external resource type                                                                |
 * |   654.1  | Expose Maximum number of format planes.                                                                |
 * |   654.0  | Adaptive space warp(ASW) support added to existing Motion Estimation interface,                        |
 * |          | to support both H264 and Hevc.                                                                         |
 * |   653.3  | Add VCN3.1 Support and renumber VCN4 enum to make it greater than VCN3.1                               |
 * |   653.2  | Add P208 Format                                                                                        |
 * |   653.1  | Added 10-bit to 8-bit hardware dithering flag support for each decoder                                 |
 * |   653.0  | Remove argument IImage* from SignalNativeFence(), the image is now handled in CmdPostProcessFrame.     |
 * |   652.0  | Add declarative heap selection in GpuMemoryCreateInfo via GpuHeapAccess.                               |
 * |   651.0  | Move CheckCommandStatus and ResetCommandStatus from Device to CheckFeedbackStatus and                  |
 * |          | ResetFeedbackStatus in ISecureProcessor and add new GetFeedbackStatus and GetSessionId functions.      |
 * |          | Remove pAppId from SecureProcessorCmdInfo structure.                                                   |
 * |   650.0  | Removed ConditionVariable::Init()                                                                      |
 * |   649.0  | Removed RWLock::Init()                                                                                 |
 * |   648.0  | Change acquire/release event signaling and waiting to use release token which encodes sync type and    |
 * |          | fence value. And remove the enableGpuEventMultiSlot support.                                           |
 * |   647.0  | Removed Mutex::Init()                                                                                  |
 * |   646.0  | Add an ImageUsage flag to indicate that a surface is a VRS rate image.                                 |
 * |   645.1  | Add File::FastForward & File::RSeek.                                                                   |
 * |   645.0  | Add Extent3d as a reference to ExternalImageOpenInfo for creating image from external handle.          |
 * |          | The old behavior is preserved if any dimension of the extent equals 0. Under such condition this       |
 * |          | reference value would be ignored and just use extents from shared image metadata.                      |
 * |   644.0  | Add separate depth clip support.                                                                       |
 * |   643.0  | Add dxClientId for PlatformCreateInfo if PAL_CLIENT_DX == 1.                                            |
 * |   642.1  | Add supportCtxBufNotInTmz flag in VcnIpProperties to indicate if context buffer is in TMZ or not.      |
 * |   642.0  | Remove ImageAspect from the interface, replace aspect in SubresId with plane index, and add numPlanes  |
 * |          | to SubresRange. Throughout the code base the concept of aspects should be replaced with planes         |
 * |          | (especially in the comments). Update all code paths that use SubresRange to work for the new numPlanes |
 * |          | member (both single plane and multi plane should work). Also, added GetFullSubresourceRange to IImage  |
 * |          | so that clients can more easily set a SubresRange object to cover the image's entire range.            |
 * |   641.2  | Add PAL_HAS_CPP_ATTR and PAL_NODISCARD.                                                                |
 * |   641.1  | Add AV1 video decode support.                                                                          |
 * |   641.0  | Expose system filename length limits in palArchiveFile                                                 |
 * |   640.1  | Add supportPointerFlags flag so clients can communicate this info to build/traversal shaders.          |
 * |   640.0  | Add support for Direct Capture. Update ExternalResourceOpenInfo for opened Direct Capture resource.    |
 * |          | And update CmdBufInfo for appending the direct capture info in submission.                             |
 * |   639.0  | Add an option to use BVH T#.pointer_flags = 1.                                                         |
 * |   638.0  | Remove IDevice::GetValidFormatFeatureFlags because it is no longer used or relevant.                   |
 * |   637.2  | Update Light Shaft optimization for increased lightShaftDrawCall range.                                |
 * |   637.1  | Added Encode Core Queue support                                                                        |
 * |   637.0  | Add Binnable Pass interfaces.                                                                          |
 * |   636.2  | Add supportRayTraversalStack flag so clients can pick a shader based on whether Gpu supports traversal |
 * |          | with HW stack.                                                                                         |
 * |   636.0  | Add stencilOnlyView flag to ImageUsageFlags for cases where stencil-only depth target is possible.     |
 * |   635.1  | Add some new blend equations in AMD_Blend_MinMax extension                                             |
 * |   635.0  | Add emulated mesh/task-shader pipeline stats query for Navi2x which don't have native hardware support.|
 * |   634.2  | Add new field shaderSubType to Pal::ShaderLibStats.                                                    |
 * |   634.1  | Add Util::Rename().                                                                                    |
 * |   634.0  | Update Pal's ELF code objects to AMDGPU V4.                                                            |
 * |   633.2  | Add more clamp type in EXT_texture_mirror_clamp.                                                       |
 * |   633.1  | Add interface to notify clients that the window size is changed possibly                               |
 * |   633.0  | Remove the interfaces introduced with 622.2                                                            |
 * |   632.0  | Add drawId parameter in CmdDraw or CmdDrawIndex.                                                       |
 * |   631.0  | Remove pipeline residency options from Platform and Pipeline. Pipeline residency can only be controlled|
 * |          | via Device::PalPublicSettings::pipelinePreferredHeap.                                                  |
 * |   630.0  | Fix wavesPerSh calculations to correctly match GFX10 hardware expectation.                             |
 * |          | Following this version all clients need to scale their existing values down by numShaderArrays to      |
 * |          | account for this change.                                                                               |
 * |   629.0  | Move ViewportInfo from SCPC GraphicsPipelineCreateInfo to PAL GraphicsPipelineCreateInfo.              |
 * |   628.2  | Add requireFrameEnd flag to DeviceProperties::osProperties.                                            |
 * |   628.1  | Add support flag of dirty tile map. Allow clients to get tile dimensions of dirty tile map from PAL.   |
 * |   628.0  | Added support for compute thread group scheduling using tgScheduleCountPerCu.                          |
 * |   627.0  | Add SEI feature support for HEVC encoder                                                               |
 * |   626.0  | Rename ScaledCopyFlags.srcSrgbAsUnorm to ScaledCopyFlags.dstAsSrgb, Blit/Copy only care dstImage, it's |
 * |          | irrelevent to srcImage.                                                                                |
 * |   625.1  | Add useHp3dForDwm flag into FullScreenFrameMetadataControlFlags as an output field.                    |
 * |   625.0  | Add fullScreenFrameMetadataControlFlags in CmdPostProcessFrameInfo                                     |
 * |   624.2  | Add LPDDR4 related info to LocalMemoryType and SqttMemoryType.                                         |
 * |   624.1  | Add HMP License Read/Write functions to IDevice.                                                       |
 * |   624.0  | Add 2 more QP Map modes for PAL VCN VIdeo Encoder                                                      |
 * |   623.0  | Add seDetailedMask in SQ thread trace configuration of GpaSampleConfig                                 |
 * |          | Add enum value of disabling all shader stages for perf experiment samples in PerfExperimentShaderFlags |
 * |   622.2  | Add intefaces for android swapbuffer performance optimization                                          |
 * |   622.1  | Add gpuEmulatedInHardware flag to DeviceProperties::pciProperties                                      |
 * |   622.0  | Add disablePartialDispatchPreemption for compute pipelines.                                            |
 * |   621.0  | Remove cmdBufForceCpuUpdatePath from CmdBuffer build flags and the associated panel setting. While the |
 * |          | flag is removed as of this version, the functionality has long been deprecated.                        |
 * |   620.0  | Add flag tmzProtected in PresentableImageCreateInfo to indicate this image is tmz protected when client|
 * |          | doesn't create swapchain.                                                                              |
 * |   619.1  | Add Write Combine Support Flag for Video Decode.                                                       |
 * |   619.0  | Remove legacy metadata from the PAL ELF ABI.                                                           |
 * |   618.1  | Add RsFeatureType::RsFeatureTypeBoost, RsFeatureInfo::boost.hotkey, boost.minres, boost.enabled that   |
 * |          |     are filled via IDevice::GetRsFeatureGlobalSettings() for Boost 1.0 and VRS Boost 1.0 (BigSW 6.0).  |
 * |   618.0  | Add CoherSampleRate and LayoutSampleRate for images passed to CmdBindSampleRateImage.                  |
 * |   617.1  | Add interface to import surface flinger buffer object.                                                 |
 * |   617.0  | Add support for EFC conversion in h264 and hevc encoder.                                               |
 * |   616.0  | Add tmzProtected flag in ImageCreateFlags to indicate the image is protected or not.                   |
 * |   615.0  | Add ability to get/set generic sections to pipeline ELF for PipelineAbiProcessor.                      |
 * |   614.0  | Add maxTaMessageSize to SecureProcessorCreateInfo.                                                     |
 * |   613.1  | Add GetRWLockData() for Util::RWLock and Wait() on RWLock for Util::ConditionalVariable.               |
 * |   613.0  | Removes call to DriverControlServer::StartLateDeviceInit, this call is now client responsibility to    |
 * |          | ensure driver state does not advance before devices are ready. The call should be made immediately     |
 * |          | before DevDriverServer::Finalize is called.                                                            |
 * |   612.0  | Add DXGI stereo support for dx11                                                                       |
 * |   611.1  | Add gfx9 swizzle info to Pal::Developer::ImageDataAddrMgrSurfInfo.                                     |
 * |   611.0  | add "Zero-copy" and "Reserve" semantics for ICacheLayer.                                               |
 * |   610.0  | Add compositeAlphaMode to the swapchainProperties.                                                     |
 * |   609.0  | Add alignment requirement to CreateHashContext.                                                        |
 * |   608.1  | Add tmzSupportLevel to indicate which queue supports per-command, per-submit, or per-queue TMZ based on|
 * |          | the queue type.                                                                                        |
 * |   608.0  | Remove 'adjacency' from the iaInfo of the GraphicsPipelineCreateInfo.                                  |
 * |   607.0  | Add support for floating point representation of copy regions for scaled image copy.                   |
 * |   606.0  |  Add resummarizeHiZ flag to enable client to use.                                                      |
 * |   605.1  | Support for PS sort-agnostic Barycentric coordinates on Navi21+.                                       |
 * |   605.0  | Add forceWaitIdleOnRingResize flag in Queue CreateInfo.                                                |
 * |   604.0  | Change maxWavesPerCu (in DynamicComputeShaderInfo & DynamicGraphicsShaderInfo) from uint32 to float so |
 * |          | clients are able to specify less waves (i.e. less number of waves than number of CUs per shader array).|
 * |   603.0  | Add scissor test support for image copy and scaled image copy.                                         |
 * |   602.0  | Add useLossy flag to ImageUsageFlags for lossy image compression on hardware that supports it.         |
 * |   601.0  | Rename P8_Uint to P8_Unorm, clients need the format to be treated as a UNORM format instead of UINT.   |
 * |   600.0  | Add tmzOnly flag in QueueCreateInfo for tmz compute queue.                                             |
 * |   599.0  | Add ResolveImageFlags enum to support Y-inverted resolves.                                             |
 * |   598.0  | Add forcedShadingRate to replace forceSampleRateShading in GraphicsPipelineCreateInfo.                 |
 * |   597.0  | Support Display Dcc.                                                                                   |
 * |   596.0  | Removed the numScratchWavesPerCu PAL public setting.                                                   |
 * |   595.0  | On Linux, make PAL(Vulkan client) read amdVulkanSettings.cfg while PAL(OGLP client) read               |
 * |          | amdOglpSettings.cfg.                                                                                   |
 * |   594.0  | Support for rendering to depth-only or stencil-only component of an depth-stencil format image.        |
 * |   593.1  | Add TurboSync in ApplicationProfileClient enum to allow querying EnhancedSync2.5 AP_AREA from KMD.     |
 * |   593.0  | Support TMZ (trusted memory zone). Add enableTmz in CmdBufferBuildFlags. Add supportsTmz in            |
 * |          | DeviceProperties and GpuMemoryHeapProperties. Add tmzProtected in GpuMemoryCreateFlags and             |
 * |          | SwapChainCreateInfo.                                                                                   |
 * |   592.0  | Support for color masked clear. Add disabledChannelMask in ClearColor to enable client to choose clear |
 * |          | mask during color clear.                                                                               |
 * |          | Note that the client should initialize disabledChannelMask with a certain value.                       |
 * |   591.1  | Add DbgPrintCatMsgFile.                                                                                |
 * |   591.0  | Remove heap perf.                                                                                      |
 * |   590.1  | Added Util::Vector::Resize.                                                                            |
 * |   590.0  | Added IScreen::GetFormatHdrMode(), IDevice::GetFormatHdrMode() (for DX), CasUtil::NotifyHdrMode(), and |
 * |          | CasUtil::IsFormatSupported().  Removed ScreenColorSpace::CsFreeSync2; CsNative should be checked       |
 * |          | instead, along with ScreenColorCapabilities::freeSyncHdrSupported.                                     |
 * |   589.1  | Add support for MM formats used for YUV planes and the P210 YUV format.                                |
 * |   589.0  | Add a new stencilWriteMask for depth/stencil-clear to support clearing specific stencil planes.        |
 * |   588.0  | Replace GpuBlock::Df with GpuBlock::DfMall, as required by our new, more user-friendly MALL interface. |
 * |   587.0  | Add extra UMC-specific threshold params to the perf experiment and GPA session counter info structs.   |
 * |   586.0  | Add view3dAs2dArray flag to ImageCreateFlags enum client can view 3D image as 2D array.                |
 * |   585.0  | Add presentable to ImageCreateFlags and GpuMemoryCreateFlags.                                          |
 * |   584.2  | Added supportFp16Dot2 to DeviceProperties.                                                             |
 * |   584.1  | Add CmdSuspendPredication and predication to InheritedStateFlags                                       |
 * |   584.0  | Break largePageMinSizeForAlignmentInBytes into two fields: largePageMinSizeForVaAlignmentInBytes and   |
 * |          | largePageMinSizeForSizeAlignmentInBytes.                                                               |
 * |   583.0  | Add the swizzledFormat info for CopyRegion                                                             |
 * |   582.2  | Add RemoveFilesOfDir and GetStatusOfDir in Pal Util                                                    |
 * |   582.1  | Added Dynamic DPB Tier 2 Decode support and an Vcn Info capability flag for it.                        |
 * |   582.0  | Add present MSC control for DRI3 on Linux                                                              |
 * |   581.0  | Add a new decoder buffer - pHistogramBuffer.                                                           |
 * |   580.1  | Add jpegDecodeVaOffsetShiftInGb into VcnIpProperties.                                                  |
 * |   580.0  | Add new field stackFrameSizeInBytes to Pal::CommonShaderStats. The clients can use this to query the   |
 * |          | stack frame size for any shader or function by calling IPipeline::GetShaderStats() or                  |
 * |          | IShaderLibrary::GetShaderFunctionStats().                                                              |
 * |          | Add maxFunctionCallDepth parameter to Pal::ComputePipelineCreateInfo to calculate worst case total     |
 * |          | stack frame size if it is not set by application.                                                      |
 * |          | Add IPipeline::SetStackSizeInBytes() to allow application to specify the total stack size needed.      |
 * |   579.2  | Add AtomicWriteRelaxed64 and AtomicReadRelaxed64.                                                      |
 * |   579.1  | Add isHdrEnabled flag to ScreenColorCapabilities.                                                      |
 * |   579.0  | Add support for Light Shaft optimization.                                                              |
 * |   578.0  | Add 'rectangleCount' and 'pRectangles' fields to Pal::PresentSwapChainInfo. Add support for            |
 * |          | VK_KHR_INCREMENTAL_PRESENT for Wayland on Linux.                                                       |
 * |   577.0  | Add a new HwPipePreColorTarget in HwPipePoint enum, and a PAL public setting to expose control of how  |
 * |          | to convert the new pipe point to existing ones.                                                        |
 * |   576.0  | Add a structure for dpbConfig.                                                                         |
 * |   575.0  | Allow clients to specify ApiType in saved RGP files by adding a parameter to GpaSession constructor.   |
 * |   574.0  | Add Task and Mesh shader entries to DynamicGraphicsShaderInfos.                                        |
 * |   573.0  | Change parameter 'pDiscarded' to 'pResults' for IDevice::ReclaimAllocations.                           |
 * |   572.0  | Add gang submission support.                                                                           |
 * |   571.0  | Add a new boolean parameter 'allowDecommit' for IDevice::OfferAllocations.                             |
 * |   570.0  | Add result return to Pal::GpuUtil::GpaSession::BeginSample.                                            |
 * |   569.0  | Add MALL range support for GPU memory allocations.                                                     |
 * |   568.0  | Add 'fenceCount' and 'ppFences' fields for SubmitInfo to support multiple fence objects to be signaled |
 * |          | for one submission.                                                                                    |
 * |   567.0  | Add pipSwapChain flag to Pal::ImageCreateFlags to indicate that image is PIP swap-chain.               |
 * |   566.0  | Add implicitReset in FenceExportInfo to control if fence reset is needed for sync fd exported.         |
 * |   565.0  | Add a new dpbRefBuffers array, and renamed dpbBuffer to dpbCurrBuffer in VideoDecodeFrameInfo for      |
 * |          | for Array of Textures features                                                                         |
 * |   564.0  | Add a new type FmaskOnly for Image MetadataMode, only valid for color msaa Image. If this mode is      |
 * |          | selected, color msaa Image will only have Cmask/Fmask metadata.                                        |
 * |   563.1  | Adds additional MiscInternal types to palEventDefs.h to support DXCP for RMV logging.                  |
 * |   563.0  | Add dirtyTileSize and dtmClearColor to ExternalImageOpenInfo for opening a external image as a dirty   |
 * |          | tile map traced image.                                                                                 |
 * |   562.2  | Add supportEncodeProtectedSession for all multimedia encode engines                                    |
 * |   562.0  | Add fullCopyDstOnly into Pal::ImageCreateFlags for client to hint any copy to this image like,         |
 * |          | ICmdBuffer::CmdCopyImage, CmdCopyMemoryToImage, and CmdScaledCopyImage using this image as a desination|
 * |          | will overwrite the entire image.                                                                       |
 * |   561.0  | Add PresentMode to CmdPostProcessFrameInfo for display on PAL debug overlay.                           |
 * |   560.1  | Add supportSingleChannelMinMaxFilter, and set to 0 for hardware without any min/max filter support.    |
 * |   560.0  | Add capture/replay features to gfxipProperties.flags.                                                  |
 * |   559.0  | Add dtmClearColor to ImageCreateInfo for dirty tile map.                                               |
 * |   558.0  | BufferView SRD MALL control flags.                                                                     |
 * |   557.0  | Add Lineloop and Polygon primitive type for OGL                                                        |
 * |   556.0  | Add support for IShaderLibrary.                                                                        |
 * |   555.1  | Add ClearColorType::Yuv.                                                                               |
 * |   555.0  | Add support for command buffer dumping.                                                                |
 * |   554.0  | Add image resolve support for PRT+                                                                     |
 * |   553.0  | Add Android Window System support.                                                                     |
 * |   552.0  | Add ScaledCopyFlags.srcSrgbAsUnorm flag to treat an sRGB source image as UNORM.                        |
 * |   551.0  | Add support for alphaToOne.                                                                            |
 * |   550.0  | Separate memory predication support (in engineProperties and PredicateType) into 32 and 64-bit.        |
 * |   549.1  | Add Util::AtomicOr and Util::AtomicOr64.                                                               |
 * |   549.0  | Add Mesh & Task shader types.                                                                          |
 * |   548.1  | Add depthClampBasedOnZExport PAL public setting.                                                       |
 * |   548.0  | Remove maxUserDataEntries PAL public setting because it is unnecessary.                                |
 * |   547.1  | Add a few more P2P support flags.                                                                      |
 * |   547.0  | Remove copyFormatsMatch because it's not used in any production paths and is broken in general.        |
 * |   546.2  | Expose MSAA sample size support in DeviceProperties. This is identical in all ASICS                    |
 * |   546.1  | Add QueryRadeonSoftwareVersion to Pal::Device for retrieving the Radeon Software Version.              |
 * |   546.0  | Remove quilting support.                                                                               |
 * |   545.0  | Add supportArbitaryPrtMapUnmap to denote whether kmd fix for arbitary prt map unmap operation is       |
 * |          | available.                                                                                             |
 * |   544.0  | Modify ThreadTraceRegTypeFlags to follow a logical sequence, requesting reg types as comments indicate.|
 * |   543.1  | Add shaderClock features to gfxipProperties.flags.                                                     |
 * |   543.0  | Add GeDist, GeSe, and Df support to IPerfExperiment. Add an extra DF-specific parameter to the perf    |
 * |          | the perf experiment and GPA session counter info structs. Removed the old SQ counter masks.            |
 * |          | Added Util::BitExtract() which is a clean way to shift and mask out a bitfield.                        |
 * |   542.0  | Add device property flag to indicate if IFH mode is on, instead of making it a PAL public setting.     |
 * |   541.1  | Add ICmdBuffer::CmdNop() to embed data directly into the command stream.                               |
 * |   541.0  | Add exportType in GpuMemoryExportInfo to support more export handle types.                             |
 * |   540.0  | Add dwmAlloction flag to GpuMemoryCreateInfo to indicate that gpu memory is DWM allocation.            |
 * |   539.1  | Add RsFeatureInfo::chill.hotkey, chill.minFps, chill.maxFps, delag.enabled, delag.limitFps fields that |
 * |          | are filled via IDevice::GetRsFeatureGlobalSettings() for Chill 3.1 and Delag 2.0 (BigSW 6.0).          |
 * |   539.0  | Add ImageCreateInfo::refreshRate and deprecate stereoRefreshRate as flippable images may also need this|
 * |          | info on latest Windows OS.                                                                             |
 * |   538.0  | Remove support for a Navi10 SCBU chip.                                                                 |
 * |   537.0  | Removed IDevice::DidTurboSyncSettingsChange(), DidChillSettingsChange(), DidDelagSettingsChange(),     |
 * |          | GetChillGlobalEnable(), GetDelagHotKey().  Replaced them with DidRsFeatureSettingsChange(),            |
 * |          | GetRsFeatureGlobalSettings().  New interfaces support RIS 2.0 with back compat for RIS 1.0.            |
 * |   536.0  | Adds interface stubs to support Mesh Shaders.                                                          |
 * |   535.2  | Adds extra developer callbacks used by the PM4 Instrumentor layer.                                     |
 * |   535.1  | Add ifhMode to PublicSettings and also remove ifh from PalSettings to move the control to the clients. |
 * |   535.0  | Adds flag to PlatformCreateInto to indicate that the client supports RGP tracing.  PAL will take over  |
 * |          | responsibilty for enabling tracing in DevDriver with this version.                                     |
 * |   534.0  | Add GetCurSize and GetHashIds functionalities to memoryCache.                                          |
 * |   533.0  | Add support for RGD (Radeon GPU Detective) execution markers.                                          |
 * |   532.1  | Add GenMipmapsInfo and CmdGenerateMipmaps() to automatically and efficiently generate a mip chain.     |
 * |   532.0  | Adds GFXIP property to query support for 64b shader instructions.                                      |
 * |   531.0  | Adds IPlatform and IDevice functions/structures to support RMV event logging.                          |
 * |   530.0  | Removed EngineTypeExclusiveCompute, EngineTypeHighPriorityUniversal, and EngineSubType.  Replaced them |
 * |          | with QueuePriority and DeviceProperties::engineProperties.capabilities[] (per-engine instance).        |
 * |          | Replaced EngineSubType::VrHighPriority with QueueCreateInfo::flags.dispatchTunneling and               |
 * |          | CmdBufferCreateInfo::flags.dispatchTunneling.                                                          |
 * |          | Renamed QueuePriority::Low to Normal and VeryLow to Idle.                                              |
 * |   529.0  | Add CB / DB / SRD MALL control flags.                                                                  |
 * |   528.0  | Add MALL control flags.                                                                                |
 * |   527.0  | VCN2.0 Motion vectors support for textureformat and 16x16 blocksize.                                   |
 * |   526.0  | Deprecate IDevice::ScpcGraphicsPipelineTuningOptions. It is recommended to call                        |
 * |          | Scpc::ICompiler::SetupGraphicsPipelineCreateInfoDefaults instead.                                      |
 * |   525.0  | Add line stippling support via ICmdBuffer::CmdSetLineStippleState" and adding 'enableLineStipple'      |
 * |          | to MsaaStateCreateInfo.                                                                                |
 * |   524.0  | Add OpenGL special change: (a) Add DepthRange in ViewportParams (b) Add clipDistMask and               |
 * |          | forceSampleRateShading in GraphicsPipelineCreateInfo (c) Separate fillMode into frontFillMode          |
 * |          | and backFillmode in TriangleRasterStateParams                                                          |
 * |   523.1  | Add FileAccessNoDiscard to enum FileAccessMode to allow "r+" mode for fopen.                           |
 * |   523.0  | Move disableAlphaToCoverageDither in MsaaStateCreateInfo from the main struct to the flags bit field.  |
 * |   522.1  | Add new function ICmdBuffer::GetUsedSize() to return used size of all chunks in bytes for given type.  |
 * |   522.0  | Remove supportNestedCmdBufStateInheritance, we no longer support nested state inheritance.             |
 * |   521.2  | Add GpuUtil::CasUtil, which does the CAS blit for RIS functionality.  For now, only PAL should use it  |
 * |          | internally rather than clients, but in the future (RIS 2.0), responsibility will be passed to clients. |
 * |   521.1  | Change initialCount in QueueSemaphoreCreateInfo from uint32 to uint64                                  |
 * |   521.0  | Add peerTransferRead flag in struct GpuCompatibilityInfo, and renames peerTransfer to                  |
 * |          | peerTransferWrite.                                                                                     |
 * |   520.0  | Add support for PRT+                                                                                   |
 * |   519.1  | Added support for AtomicIncrement64 to Util library - palMutex.                                        |
 * |   519.0  | Add support for querying dirty tile information from DCC.                                              |
 * |   518.0  | Changes to present paths for all clients: client must now call CmdPostProcessFrame before frameEnd and |
 * |          | the actual present (all of these actions can be part of the same cmd buffer), so that PAL can add      |
 * |          | postprocessing at the appropriate time.  Hard requirement for dev driver overlay, DbgOverlay, and RIS. |
 * |   517.0  | Add ExternalQueueSemaphoreOpenInfo.flags.timeline                                                      |
 * |   516.2  | Add DeviceProperties.gfxipProperties.flags.supportPostDepthCoverage.                                   |
 * |   516.1  | Remove FSR (Flexible scale rasterization) support from PAL.                                            |
 * |   516.0  | Clients no longer need to align GPU memory size/alignment to allocGranularity when allocating memory   |
 * |          | through IDevice::CreateGpuMemory(). PAL owns this now.                                                 |
 * |   515.0  | Add argument IImage* to SignalNativeFence(), the image is mostly a swapchain image for external usage, |
 * |          | used to support postprocess before actual present, ignored when the image is unprovided.               |
 * |   514.0  | Add "disablePipelineUploadToLocalInvis" public device setting.                                         |
 * |   513.0  | Add public settings to control the BigPage TCP address translation coalescing optimization based on    |
 * |          | app-detect.  When set, all IGpuMemory allocations that end up in a system memory heap will be placed   |
 * |          | in a special carve out that can guarantee 64KiB pages.  Unfortunately, this carve out cannot be        |
 * |          | CPU-cached, and therefore this bit can have negative consequences to apps that do heavy reads from     |
 * |          | allocations they wanted to be in the the GartCacheable heap.  It will also cause PAL to report a       |
 * |          | required allocation granularity of 64KiB, which may have performance or conformance implications.      |
 * |   512.0  | Add maxFrameAverageLightLevel to ColorGamut.                                                           |
 * |   511.0  | Added enableBigPage public setting.  Never hooked up, replaced in 513.                                 |
 * |   510.0  | Add ICmdBuffer::CmdSetClipRects() to program the clip rects registers.                                 |
 * |   509.0  | Replace CmdSetHiSCompareState0()/CmdSetHiSCompareState1() with CmdUpdateHiSPretests() and add          |
 * |          | struct HiSPretests to support  HiStencil.                                                              |
 * |   508.0  | Add new interface WaitForSemaphores in pal device for new timeline semaphore spec.                     |
 * |   507.0  | Deprecate "ShaderCacheMode" enum and the shaderCacheMode public setting.                               |
 * |   506.1  | Add PipelineStageAllStages field in PipelineStageFlag enum.                                            |
 * |   506.0  | Add maxContentLightLevel to ColorGamut.                                                                |
 * |   505.0  | Add support for RT IP v1.1, which adds a ray intersection mode that returns triangle barycentrics.     |
 * |   504.0  | Add reason codes to acquire-release barriers, add type enum to Developer::BarrierData, modify          |
 * |          | Developer::BarrierOperations (a) to add bits for pipelined events and gfx10 caches and (b) remove      |
 * |          | waitOnEopTsBottomOfPipe, replacing it with (eopTsBottomOfPipe | waitOnTs).                             |
 * |   503.0  | Add "zeroUnboundDescDebugSrd" public settings.                                                         |
 * |   502.0  | Add preferred heap setting to pipeline createInfo. Remove prefereNonLocalHeap flag.                    |
 * |   501.0  | Replaced "enableRadeonImageSharpening" boolean public setting with "radeonImageSharpening" tri-state   |
 * |          | setting.                                                                                               |
 * |   500.0  | Add "useAcqRelInterface" and "enableGpuEventMultiSlot" public settings.                                |
 * |   499.0  | Add Radeon Image Sharpening support.  Clients may override the "enableRadeonImageSharpening" public    |
 * |          | setting to false.                                                                                      |
 * |   498.2  | Add GetDelagHotKey to allow client get delag user hot key settings                                     |
 * |   498.1  | Add DX12 Motion estimation support for Navi10/VCN2.0                                                   |
 * |   498.0  | Add UserDataMapping::NggCullingDataBuffer to the Pipeline ABI.  This allows the ABI to specify whether |
 * |          | an NGG pipeline accesses the internal constant buffer used to provide register state for culling.  It  |
 * |          | is more performant (on the CPU side) to skip updating this buffer if the pipeline won't use it.        |
 * |          | This ABI change is accessible to clients prior to interface v498.0, but it is useful to LLPC to have a |
 * |          | dummy version bump so they can check whether the ABI changes are accessible at compile-time.           |
 * |   497.1  | Add DidDelagSettingsChange to support DX Delag                                                         |
 * |   497.0  | Add 'UavExportTable' to the pipeline ABI's UserDataMapping and a 'uavExportSingleDraw' flag to         |
 * |          | GraphicsPipelineCreateInfo.cbState to indicate UAV-export-enabled pipelines don't need to maintain     |
 * |          | ordering between draws (eg. single draws or draws to different color targets).                         |
 * |   496.0  | Break out MetadataMode into two axes. One for Metadata. One for TC compat.                             |
 * |   495.0  | Remove support for Compute Pipeline ELF's to encode wave32 mode using the COMPUTE_DISPATCH_INITIATOR   |
 * |          | register.                                                                                              |
 * |   494.0  | Add swizzledFormat into ImageScaledCopyRegion to allow format override for image scaled copy.          |
 * |   493.0  | Add supportOutOfOrderPrimitives to DeviceProperties::gfxipProperties::flags. Remove Max1xMsaaGridSize. |
 * |   492.0  | Add support for variable rate shading.                                                                 |
 * |   491.0  | Changed the meaning of the activeCuMask DeviceProperty on gfx10 and bumped it up to 32-bits per SA.    |
 * |          | It used to be in units of WGPs but is now in units of CUs. This was done for consistency, our other    |
 * |          | "CU" properties report in units of CUs. If a client wants access to an activeWgpMask we can add it.    |
 * |   490.0  | Enable SAO settings for VCN 2.0 HEVC Video Encode                                                      |
 * |   489.0  | Add client specific initial QP for h264/hevc encode.                                                   |
 * |   488.0  | Add preferNonLocalHeap to PipelineCreateInfo. Setting this to true will put the pipeline in Gart       |
 * |          | memory.                                                                                                |
 * |   487.0  | Add kernargSegmentSize and workitemPrivateSegmentSize fields in the DispatchAqlParams since the ISA    |
 * |          | header will contain just 64 bytes instead of original 256.  The change allows the information to be    |
 * |          | passed for dispatching.                                                                                |
 * |   486.0  | Adds perf counter support for new gfxip-10 block Utcl1.                                                |
 * |   485.0  | Adding Picture Type like Frame, Even or Odd for MJPEG Video decode                                     |
 * |   484.0  | Remove the wavefrontSize device property and replace it with nativeWavefrontSize, minWavefrontSize,    |
 * |          | and maxWavefrontSize. PAL's unused WaveSize enum was removed.                                          |
 * |   483.1  | Implemented query pool reset from CPU for StreamoutStatsQueryPool and PipelineStatsQueryPool.          |
 * |   483.0  | Enable TwoPassSearchCenterMapMode for VCN Video Encode                                                 |
 * |   482.0  | Add globalSrcCacheMask and globalDstCacheMask to BarrierInfo to allow clients to specify a set of      |
 * |          | cache operations which apply to all transitions in the barrier.                                        |
 * |   481.0  | Replace ImageCreateFlags::noMetadata with an enum to give clients more control.  Instead of just being |
 * |          | able to enable/disable Image metadata, clients can now give PAL hints which let us decide if the Image |
 * |          | should use TC compatible reads.                                                                        |
 * |   480.0  | Update MotionEstimatorCreateInfo to enable protected DX12 motion Estimation tests                      |
 * |   479.0  | Link gpu memory priorty system to os on linux. Client shall be aware that if they would like to enable |
 * |          | memory priority on linux queue, they should set enableGpuMemoryPriorities to 1 on specific queue.      |
 * |   478.1  | Added largePageMinSizeForAlignmentInBytes to PalPublicSettings and reported largePageSizeInBytes to    |
 * |          | clients in DeviceProperties.                                                                           |
 * |   478.0  | Update CreateImageViewSrds to replace shaderWritable with a layout field describing possible usages    |
 * |          | the image could be in for the duration of the SRD. This is primarily to avoid compressed writes onto   |
 * |          | uncompressed surfaces.                                                                                 |
 * |   477.0  | Updated pipeline ABI metadata note ID to 32 to match HSA code objects.                                 |
 * |   476.0  | Add support for apiPsoHash in layers. Remove palRuntimeHash and replace with apiPsoHash or unique part |
 * |          | of internalPipelineHash as required.                                                                   |
 * |   475.0  | Adds CmdBufferBuildFlags::useCpuPathForTableUpdates to control whether or not CE RAM or the CPU should |
 * |          | be used to update the stream-out, vertex buffer, and user-data-spill tables.                           |
 * |   474.0  | Change IGpuEvent class from IDestroyable interface to IGpuMemoryBindable interface.                    |
 * |          | And add ICmdBuffer::AllocateAndBindGpuMemToEvent()                                                     |
 * |   473.1  | Adds canShareSemaphoreKmtHandle bit to Pal::DeviceProperties::osProperties::flags.  GFX10 MES HWS mode |
 * |          | is currently the only scenario in which this bit is set to 0;  otherwise it is always 1.               |
 * |   473.0  | Adds ICmdBuffer::CmdSetVertexBuffers() to update the vertex buffer SRD table, which replaces @ref      |
 * |          | ICmdBuffer::CmdSetIndirectUserData().  Also adds a vertex buffer count to GraphicsPipelineCreateInfo   |
 * |          | so PAL can manage the number of vertex buffer SRD's to dump from CE RAM to GPU memory before a Draw,   |
 * |          | which replaces ICmdBuffer::CmdSetIndirectUserDataWatermark().                                          |
 * |   472.1  | Add palDevice interface QueryGpuMemoryBudgetInfo to query memory usage and budget info.                |
 * |   472.0  | The query pool slot reset operation after occlusion query's issue_begin causes too many map/unmap      |
 * |          | operations of GPU memory. We could optimize this by passing the mappped gpu address into Reset() so as |
 * |          | to keep the gpu memory locked and avoid unneccesary map/unmap.                                         |
 * |   471.0  | Add new Pal Developer Callback BindPipeline and struct BindPipelineData for supporting RGP             |
 * |          | Instrumentation. Introduce apiPsoHash as part of data passed during CmdBindPipeline call.              |
 * |   470.0  | Remove CalibrateGpuTimestamp, add GetCalibratedTimestamps, add timeDomains to osProperties             |
 * |   469.0  | Size of the indirect user-data table is no longer configurable by the client.  Instead, it is defined  |
 * |          | to be MaxVertexBuffers * sizeof(BufferSrd).  Additionally, the CE RAM used to store this table is now  |
 * |          | managed internally by PAL and is no longer specified by @ref DeviceFinalizeInfo::ceRamSizeUsed.        |
 * |   468.0  | Added an new engine type: EngineTypeVcnUnified which can perform both video encode and decode          |
 * |   467.0  | Add ICmdBuffer::CmdUpdateSqttTokenMask() and add a mode param to GpaSession::UpdateSampleTraceParams   |
 * |   466.0  | Remove the flag turboSyncEnabled from PresentSwapChainInfo::flags.                                     |
 * |   465.3  | Add shaderPrefetchBytes to DeviceProperties::gfxIpProperties::shaderCore.                              |
 * |   465.2  | Add supportsUnmappedPrtPageAccess flag to DeviceProperties::engineProperties::flags.                   |
 * |   465.1  | Add support for CPU clock speed reporting in MHz.                                                      |
 * |   465.0  | Added gl2Uncached to GpuMemoryCreateInfo to indicates the GPU Memory is un-cached on GPU L2 cache. But |
 * |          | the memory still would be cached by other cache hierarchy like L0, RB caches, L1, and L3. This feature |
 * |          | is only supported if the gl2UncachedSupported DeviceProperty flag is set.                              |
 * |   464.1  | Add supportsTrackBusyChunks flag to DeviceProperties::engineProperties::flags.                         |
 * |   464.0  | Add cpuInvisible to GpuMemoryCreateFlags to hint the GpuMemory will never be mapped for CPU access. If |
 * |          | this flag is set, calls to IGpuMemory::Map() on this object will fail.                                 |
 * |   463.3  | Add eccProtectedGprs flag to DeviceProperties::gfxipProperties::shaderCore.                            |
 * |   463.2  | Add Abi::AmdGpuCommentName to unify naming convention between SCPC and LLPC.                           |
 * |   463.1  | Add elementBytes and mipTailCoord in SubresLayout. Also add mipTailCoord in SubResourceInfo.           |
 * |   463.0  | Add disableInternalResidencyOpts to PlatformCreateInfo. Clients need to set this flag to 1 if they     |
 * |          | wish to turn off the residency optimizations for internal GPU memory allocations to save on system     |
 * |          | resources. This flag only affects Windows WDDM 1.x platforms. It is ignored on all other platforms.    |
 * |   462.0  | Removed GfxIpLevel::Count enum value since it wasn't always correct.                                   |
 * |   461.1  | Add DeviceProperties.gfxipProperties.shaderCore.cuMask                                                 |
 * |   461.0  | Add fullResolveDstOnly into Pal::ImageCreateFlags for client to hint any ICmdBuffer::CmdResolveImage   |
 * |          | using this image as a desination will overwrite the entire image (width and height of resolve region is|
 * |          | same as width and height of resolve dst).                                                              |
 * |   460.0  | Replaced 64-bit pipeline "compiler hash" with 128-bit "internal pipeline hash".  Bumped pipeline       |
 * |          | metadata version to 2.0.  PipelineInfo::pipelineHash and ::compilerHash are deprecated.                |
 * |   459.0  | Add JPEG Decode support.                                                                               |
 * |   458.1  | DispatchAqlParams.pAqlPacket->group_segment_size requires to include the static LDS usage in the kernel|
 * |   458.0  | Add timeline semaphore support.                                                                        |
 * |   457.1  | Adds the 'supportNestedCmdBufStateInheritance' flag to @ref DeviceProperties::gfxipProperties::flags.  |
 * |          | This flag indicates whether or not the GFX hardware supports state inheritance for nested command      |
 * |          | buffers.                                                                                               |
 * |   457.0  | Add support for acquire/release-based barrier interface.                                               |
 * |   456.0  | Replace some uses of PsUsesUavs() with PsWritesUavs().                                                 |
 * |   455.1  | Add ICmdBuffer::CmdSetBufferFilledSize() to set the buffer-filled-size explicitly.                     |
 * |   455.0  | Add restricted flags to GpuMemoryFlags and GpuMemoryCreateFlags.                                       |
 * |   454.0  | Add pSlaveDevices to SwapChainCreateInfo for XDMA fullscreen present support.                          |
 * |   453.0  | Add parameters firstInstance and instanceCount to CmdDrawOpaque.                                       |
 * |   452.1  | Add GFXIP10.3 and Navi21Lite enumerations.                                                             |
 * |   452.0  | Add the SPI preference priority support.                                                               |
 * |   451.0  | Modified Thread Trace config to accept a bit-field of token types and register types instead of a mask.|
 * |   450.0  | Add PinnedGpuMemoryCreateInfo::alignment flag for pass the alignment calculated by client              |
 * |   449.0  | Add EngineSubType to CmdBufferCreateInfo to tell if the command buffer is in a VrHighPriority for the  |
 * |          | dispatch tunneling feature.                                                                            |
 * |   448.0  | Add SamplerInfo::disableSingleMipAnisoOverride flag to allow client have control over this Sampler     |
 * |          | optimization when creating Sampler SRDs.                                                               |
 * |   447.0  | Remove the GetConnectorIdFromOutput.                                                                   |
 * |   446.0  | Remove the support for view3dAs2dArray flag.                                                           |
 * |   445.0  | Add compositeAlpha into SwapChainCreateInfo                                                            |
 * |   444.0  | Remove dx9Mipclamping flag from SamplerInfo and keep MIP_POINT_PRECLAMP = 0.                           |
 * |   443.0  | Added more NullGpuIds for null device backend.  All AsicRevisions should be represented now.           |
 * |   442.0  | Add DeviceProperties.gfxipProperties.shaderCore.numAvailableCus and numPhysicalCus.                    |
 * |   441.1  | Added a IPlatform::EnableSppProfile to enable game specific spp profile.                               |
 * |   441.0  | Add a flag notifyOnly in Pal::PresentSwapChainInfo for a "notify-only" present mechanism.              |
 * |   440.1  | Add IScreen::IsImplicitFullscreenOwnershipSafe as lightweight validation for implicit fullscreen mode. |
 * |   440.0  | Adds fields to ComputePipelineCreateInfo to allow clients to request that PAL determines the GPU VA of |
 * |          | symbols referring to shader functions inside the ELF's .text section.  Also adds support to the        |
 * |          | PipelineAbiProcessor class to add and parse generic ELF symbols so PAL can compute shader function GPU |
 * |          | addresses.                                                                                             |
 * |   439.0  | Add a flag notifyOnly in Pal::CmdPresentInfo for a "notify-only" present mechanism.                    |
 * |   438.0  | Modified pipeline binary cache interface to support seperate setting and getting of policies. Added    |
 * |          | option to memory pipeline cache layer for replacing existing entries. Added support for tracking       |
 * |          | binary cache layer and ListDir utility function.                                                       |
 * |   437.0  | Remove support for Gfx9 NGG.                                                                           |
 * |   436.0  | Creates PalPlatformSettings containing layer, debug print & assert settings                            |
 * |   435.0  | Remove SetRandrOutput and add GetRandrOutput. Remove the WsiScreenProperties which is not necessary to |
 * |          | expose these information to client.                                                                    |
 * |   434.1  | Add IDevice::DidTurboSyncSettingsChange for TurboSync 2.0 support.                                     |
 * |   434.0  | Add support for creation of ray-trace (BVH) SRDs.                                                      |
 * |   433.0  | Add support for opening NT sharing objects (Semaphore and GPUMemory) from name.                        |
 * |   432.0  | Updated Code Object to use MsgPack for encoding pipeline ABI metadata, along with other minor changes. |
 * |          | Removed PipelineAbiProcessor::{Add|Get|Has}PipelineMetadataEntr{y|ies}, ::{Add|Get|Has}RegisterEntry.  |
 * |          | Renamed PipelineAbiProcessor::GetAbiVersion to ::GetMetadataVersion.                                   |
 * |   431.0  | Remove the enum EngineTypeHighPriorityGraphics.                                                        |
 * |   430.0  | Add maxNumDedicatedCuPerQueue and dedicatedCuGranularity for VK_AMD_dedicated_compute_units extension. |
 * |   429.0  | Added SEMask field to gpaSession to control which SEs get thread traces.                               |
 * |   428.0  | Back out 427.0 (support for opening NT sharing objects from name) due to regression                    |
 * |   427.1  | Expand the list of AsicRevision to support more gfx10 versions.                                        |
 * |   427.0  | Add support for opening NT sharing objects (Semaphore and GPUMemory) from name.                        |
 * |   426.0  | Expand list of reported GFXIP levels and supported GPU names for gfx10.                                |
 * |   425.0  | Added support for underestimate conservative resterization mode.                                       |
 * |   424.0  | Added support for UMC block performance counters. New GpuBlock::Umcch added.                           |
 * |   423.0  | Adds enableCpuAccess flag to QueryPoolCreateInfo.  Clients must set this if they wish to call either   |
 * |          | IQueryPool::Reset or IQueryPool::GetResults.                                                           |
 * |   422.3  | Bumps RGP file format version to 1.1.                                                                  |
 * |   422.2  | Add IDevice::SetDx12DownlevelRuntimeData() which allows the DX12 client to pass runtime callbacks to   |
 * |          | PAL which are used for D3D12 on Windows 7 support.                                                     |
 * |   422.1  | Add new interface GetCacheFilePath() and GetDebugFilePath() so that client can query and store files   |
 * |          | in standard hierarchical location.                                                                     |
 * |   422.0  | Add trace stalling settings to GpuProfilerSqttConfig, GpaSampleConfig, and ThreadTraceInfo             |
 * |   421.0  | Add ASP Drm support to Video Decoders                                                                  |
 * |   420.0  | Settings refactor changes, moving/changing definitions in public headers                               |
 * |   419.1  | Adds a flag to GPA session to indicate whether the client will provide ETW queue semaphore timing data.|
 * |   419.0  | Add corner sampling support to the ImageViewInfo                                                       |
 * |   418.0  | Add support for GFX10 feature, MSAA coverage sample which adds the ability to export the MSAA cover    |
 * |          | mask to a specified channel within the render target.                                                  |
 * |   417.0  | Add enable1xMsaaSampleLocations flag to MsaaStateCreateInfo for non-MSAA quad sample pattern control   |
 * |   416.0  | Add codec specific and deblocking parameters to video encode (h264 and hevc)                           |
 * |   415.0  | Added direct rendering display support for X window system. supportedSwapChainModes was removed from   |
 * |          | DeviceProperties and replaced with IDevice::GetSupportedSwapChainModes.                                |
 * |          | Changed the input parameters of Util::Event::Init().                                                   |
 * |   414.0  | Add view3dAs2dArray flag to ImageCreateFlags enum so clients can create valid 2D views of 3D images    |
 * |   413.0  | remove VideoEncode and VideoDecode from the PipelineBindPoint enum. They aren't used in PAL any more.  |
 * |   412.1  | Added support for the ShaderDbg library, which instruments a shader to dump out the source and         |
 * |          | destination operands for every instruction to provide a detailed analysis of what happened in the      |
 * |          | shader. In order to build support you must define PAL_BUILD_SHADER_DBG and have a pipeline compiler    |
 * |          | which has performed the shader instrumentation and packaged it into the pipeline ELF binary. Only      |
 * |          | supported on Gfx9+ at the moment (restriction of the library).                                         |
 * |   412.0  | Adds Util::Event::Wait() and removes Util::WaitForEvents().                                            |
 * |   411.0  | For VCN Video Encode, each codec needs to have its own Capability list                                 |
 * |   410.1  | Add LDS allocation granularity to RGP file's ASIC information                                          |
 * |   410.0  | Add QP Map interface for PAL VCN                                                                       |
 * |   409.0  | Remove wave-break-size enumerations as this functionality is controlled by SCPC.                       |
 * |   408.2  | Added IndirectAllocator utility class.                                                                 |
 * |   408.1  | Added UpdateChillStatus() to support DPM Tuning For Chill.                                             |
 * |   408.0  | Remove PAL_SUPPORTED_IL_MAJOR_VERSION macro.  This has been moved to SCPC (in scpcShader.h).  Also,    |
 * |          | the palShader.h header is now deprecated and should never be included by any more clients.             |
 * |   407.1  | Added Util::ArrayLen() to palInlineFuncs.h, which gets the length of an array at compile time.         |
 * |   407.0  | Changes the CmdPresent output struct from DxPresentData to CmdPresentOutput. The new struct explicitly |
 * |          | defines the outputs PAL supports instead of reusing a DX API structure.                                |
 * |   406.0  | Add flag sampleLocsAlwaysKnown into Pal::ImageCreateFlags for client to hint sample pattern is always  |
 * |          | known in client driver for MSAA depth image. If set, decompress MSAA depth image will be deferred from |
 * |          | barrier time to resolve time, and optimally skipped if DB+CB compressed copy approach is selected. Add |
 * |          | sample pattern as parameter of CmdResolveImage() for DB+CB copy or DB decompress before shader resolve.|
 * |          | Sample pattern is needed for CmdResolveImage() if sampleLocsAlwaysKnown was set for MSAA depth image.  |
 * |   405.0  | Added MPEG2 IDCT decoding support                                                                      |
 * |   404.1  | Add new interface CmdDrawOpaque().                                                                     |
 * |   404.0  | Added palDistortionState and related changes for Flexible Scale Rasterization support.                 |
 * |   403.0  | Removed the CmdBufferBuildFlags::useLinearBufferForCeRamDumps flag.  This path is now the only path    |
 * |          | for handling CE RAM dumps in PAL.  For DX12 clients, also adds CmdBuffferBuildInfo::                   |
 * |          | rootCmdBufferGpuScratchMemSuballocSize which lets nested command buffers know how large each GPU mem   |
 * |          | chunk of the caller (root) command buffer will be.  This lets us make more efficient use of memory for |
 * |          | CE RAM dumps in a nested command buffer.                                                               |
 * |   402.0  | Add new field in ScreenMode to provide the physical dimension and preferred mode.                      |
 * |   401.0  | Add video encode interface for getting SPS, PPS, VPS header                                            |
 * |   400.0  | Video encode H264 extend begin info to support constraintSetFlag & PocType                             |
 * |   399.0  | Add NAL type to H265 feedback structure; Add more H265 NAL Type                                        |
 * |   398.1  | Added an QueueSemaphoreExportInfo parameter to IQueueSemaphore::ExportExternalHandle()                 |
 * |          | Add new field in ExternalQueueSemaphoreOpenInfo to support different external handle import.           |
 * |   398.0  | Added an FenceExportInfo parameter to IFence::GetHandle()                                              |
 * |          | Rename IFence::GetHandle() to IFence::ExportExternalHandle to be consistent with QueueSemaphore's name.|
 * |   397.1  | Removes VICE emulation support.                                                                        |
 * |   397.0  | Added an offset parameter to ICmdBuffer::CmdUpdateBusAddressableMemoryMarker()                         |
 * |   396.0  | Video encode H265 include picture type & LTR info in feedback structure                                |
 * |   395.0  | Added a new command allocator memory type, "GpuScratch".  It is invisible-local memory used for CE RAM |
 * |          | dumps and for GPU events.  Also renamed CmdBufferBuildFlags::useEmbeddedDataForCeRamDumps to           |
 * |          | useLinearBufferForCeRamDumps.                                                                          |
 * |   394.1  | Added GFXIP property to indicate timestamps may reset to 0 on GFX idle.                                |
 * |   394.0  | Added support for specifying a view format list for presentable images.                                |
 * |   393.0  | Add new field to mark the pinned host memory from foreign device mapped                                |
 * |   392.0  | Changed OsWindowHandle definition to add Wayland support on Linux                                      |
 * |   391.1  | PAL will now execute workarounds for copies not respecting the minTiledImageMemCopyAlignment engine    |
   |          | property for mem-image and image-mem copies.  These workaround copies are very slow, however, so       |
   |          | respecting the alignment is preferable whenever possible.                                              |
 * |   391.0  | Remove support for passthrough mode via the GraphicsPipelineCreateInfo.                                |
 * |   390.0  | Converted qualified inline utility functions to constexpr (C++11 is now a base requirement).           |
 * |          | Replaced the old VoidPtrInc and VoidPtrDec utility functions with const-correct versions.              |
 * |          | Removed the now redundant ConstexprMax utility function.                                               |
 * |   389.0  | Support PAL VCN for both H264 and H265 codec                                                           |
 * |          | Add codec type for video encode create info. Add fixed bits for h264 video encode slice control mode.  |
 * |   388.0  | Add support for TMZ on compute queues.                                                                 |
 * |   387.2  | Added sqttSeBufferAlignment to PerfExperimentProperties struct                                         |
 * |   387.1  | Add CmdSetHiSCompareState0()/CmdSetHiSCompareState1() to support  HiStencil.                           |
 * |   387.0  | Add pixel shader properties to PipelineInfo in IPipeline.                                              |
 * |   386.0  | Video encode H264 include picture type & LTR in feedback structure for vce engine                      |
 * |   385.0  | Add typed buffer to PresentDirectInfo for DX on HG platform.                                           |
 * |   384.0  | Add new field in DynamicComputeShaderInfo to support LDS size update during binding compute pipeline.  |
 * |   383.0  | Added CHA, CHC, CHCG, GUS, GCR, PH blocks for gfx10.                                                   |
 * |   382.0  | Remove unused Pipeline Metadata.                                                                       |
 * |   381.0  | Rename depthClampEnable flag to depthClampDisable in graphics pipeline create info.                    |
 * |   380.0  | Add interface to provide state inheritance from a previously built command buffer and move enableTmz.  |
 * |   379.0  | Changed IScreen interface for HDR support.                                                             |
 * |   378.0  | Add new field to expose supported import/export semaphore type.                                        |
 * |          | Add new field to ExternalQueueSemaphoreOpenInfo to import more semaphore types                         |
 * |   377.0  | Change PlatformCreateInfo to take in a structure for LogCallbackFunc to also include a client pointer. |
 * |   376.0  | Add video encode H265 VUI parameters configuration support                                             |
 * |   375.0  | Add the build environment variable and preprocessor macro "PAL_ENABLE_INTERNAL_SCPC", which controls   |
 * |          | if PAL will manage an internal instance of SCPC to handle pipeline compilation.  To maintain legacy    |
 * |          | behavior, they should ensure that their makefile defines both the environment variable and macro to 1. |
 * |          | If they want to disable the internal SCPC, they must define both to 0.  Clients built against versions |
 * |          | prior to 370.0 will have the internal SCPC enabled by default unless they explicitly disable it.       |
 * |   374.0  | Add enableDepthClamp flag to graphics pipeline.                                                        |
 * |   373.1  | Added IDevice::GetChillGlobalEnable for Chill support.                                                 |
 * |   373.0  | Streaming performance counter support changes in GpaSession and PerfExperiment.                        |
 * |   372.0  | Add present support for DX11 on PAL.                                                                   |
 * |   371.0  | IQueryPool::GetResults() adds support for persistently mapped query pool buffers                       |
 * |   370.0  | Support TMZ protected allocations and command submission.                                              |
 * |   369.0  | Remove MD5 support.  Use MetroHash instead.                                                            |
 * |   368.1  | Add IDevice::PrimitiveShaderGdsOffset() method so clients can tell the compiler where internal         |
 * |          | counters used for primitive shaders are stored.                                                        |
 * |   368.0  | Add debug print callback to platform creation.                                                         |
 * |   367.0  | Add cross UMD compression metadata support.                                                            |
 * |   366.0  | Replaced the formatChangeSrd and formatChangeTgt image creation flags with a new set of parameters     |
 * |          | that allow specifying an explicit list of allowed view formats at image creation time.                 |
 * |   365.1  | Interface changes required to temporarily support layer settings in core Pal settings struct.          |
 * |   365.0  | Remove old IImage::GetMemoryLayout call.  Use the return by reference version.                         |
 * |   364.0  | Adding support for creating queue with priority.                                                       |
 * |   363.3  | Add CmdPredicateEvent support for UVD/VCN for Vulkan.                                                  |
 * |   363.2  | Add IsCrossAdapter method in IImage to check whether the image is an opened cross-adapter shared image |
 * |          | on MS hybrid graphics system.                                                                          |
 * |   363.1  | Add DetermineHwStereoRenderingSupported() method in IDevice for client to check whether hardware       |
 * |          | accelerated stereo rendering can be enabled for given graphic pipeline.                                |
 * |   363.0  | Remove auto-generated Public settings                                                                  |
 * |   362.0  | Remove support for the old ELF PAL Metadata Note Type of 9.  It has been replaced by 12.               |
 * |   361.0  | Added Ge, Gl1a, Gl1c, Gl1cg, Gl2a, Gl2c perf counter support.                                          |
 * |   360.2  | Fixed a misspelling and added a supported flag for Anisotropic Lod Compensation.                       |
 * |   360.1  | Add developer mode support for HDR formats in overlay and logs.  Add LogCategory::Display.             |
 * |   360.0  | Added support for RGP barrier reasons.                                                                 |
 * |   359.0  | Add Anisotropic Lod Compensation support.                                                              |
 * |   358.0  | Add requestShadowDescriptorVaRange to PlatformCreateInfo to override memoryProperties's                |
 * |          | shadowDescVaSupport flag. Now clients need to set this flag to 1 if need to use this feature.          |
 * |   357.0  | Added "enabledPerformanceData" option for shader creation and added IPipeline::GetPerformanceData.     |
 * |   356.0  | Remove ResolveMode::Decompress from PAL. This should be handled client side.                           |
 * |   355.0  | GpaSession adds Query type that supports pipeline stats query.                                         |
 * |   354.1  | Add implementation in GpaSession timed operation that injects Present timed event marker for RGP use.  |
 * |   354.0  | Add fragment count to CmdClearBound* calls to support EQAA.                                            |
 * |   353.0  | Deprecate support for old assert / debug print macros and functions.                                   |
 * |   352.1  | Add new hashing Util MetroHash; a very fast non cryptographic 64-bit and 128-bit hasher.               |
 * |   352.0  | Add PAL_BUILD_SCPC to interface. Add PrimShaderCbLayout and related structs to the Pipeline ABI.       |
 * |   351.0  | Support MGpu Sls, add MgpuSlsInfo to PresentDirectInfo.                                                |
 * |   350.0  | Add externalOpened to QueueSemaphoreCreateInfo                                                         |
 * |   349.0  | Add srcAlpha flag for CmdScaledCopyImage. This is used for crossfire logo alpha blend effect.          |
 * |   348.1  | Add support for keyed mutex. For D3d11 resource created by misc flag D3D11_RESOURCE_MISC_SHARED_KEYEDMU|
 * |          | TEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE, a keyed mutex and a synchronization object will be bound on |
 * |          | the resource. The keyed mutex and the synchronization object could be got from D3DKMT_OPENRESOURCEFROMN|
 * |          | THANDLE in client driver. The keyed mutex could be used to perform CPU sync and the synchronization    |
 * |          | object could be used to perform GPU sync through graphics API driver. Two pal queue interface functions|
 * |          | have been added: IQueue::KeyedMutexAcquireSync and IQueue::KeyedMutexAcquireSync, which will be used to|
 * |          | perform CPU/GPU upon the shared GPU memory object.                                                     |
 * |   348.0  | Add support for sharing Fence.                                                                         |
 * |   347.0  | Remove PipelineRegisterFlags enum from palPipelineAbi.h because IA_MULTI_VGT_PARAM will be controlled  |
 * |          | by the PAL component again rather than by pipeline binaries.  PAL will allow pipeline binaries to      |
 * |          | optionally specify that register value with the understanding that PAL may override some fields.       |
 * |   346.1  | Add new IScreen interfaces for HDR support: GetFormats(), GetColorSpace(), SetColorSpace().            |
 * |   346.0  | Remove gpuVirtLoadAddr from Set/Get PipelineCode/Data/ReadOnlyData in the PipelineAbiProcessor.        |
 * |   345.0  | Replace inputs to ICmdBuffer::CmdBindPipeline with PipelineBindParams to support dynamic wave and CU   |
 * |          | enable limits.                                                                                         |
 * |   344.0  | PAL's pipeline hash doesn't match the hashes from the new pipeline compiler components.  The "hash"    |
 * |          | field in PipelineInfo has been renamed pipelineHash and a compilerHash has been added.                 |
 * |   343.0  | Add a name string parameter to PipelineAbiProcessor::Finalize(), so that clients can store a human-    |
 * |          | readable name in each pipeline binary.                                                                 |
 * |   342.0  | Add more device in NullGpuId.                                                                          |
 * |   341.1  | Add ExportKmtHandle() for IQueueSemaphore object. This is used for mantle to share semaphore with      |
 * |          | DX11 driver.                                                                                           |
 * |   341.0  | Add support for setting generic SCOptions and SCOptionsMask settings.                                  |
 * |   340.0  | Add bitstreamBufferSize in VideoDecodeFrameInfo structure for all video decoder.                       |
 * |   339.0  | Deprecate CmdStoreMsaaQuadSamplePattern, CmdLoadMsaaQuadSamplePattern and                              |
 * |          | InitMsaaQuadSamplePatternGpuMemory. Also, remove the supportDepthStencilSamplePatternMetadata flag.    |
 * |   338.0  | Add view instancing descriptor in GraphicsPipelineCreateInfo. Add CmdSetViewInstanceMask in ICmdBuffer |
 * |          | for client to set view instance mask.                                                                  |
 * |   337.1  | Relax command allocator requirement at CreateCmdBuffer. Clients can now specify a null allocator at    |
 * |          | CreateCmdBuffer, but must have a valid allocator specified via ICmdBuffer::Reset before calling        |
 * |          | ICmdBuffer::Begin.                                                                                     |
 * |   337.0  | Update TurboSyncControl interface according to TurboSync 2.0 KMD interface change, for mGPU support.   |
 * |          | Move TurboSyncControl method from IDevice to IPlatform as it now spans over multiple devices.          |
 * |   336.0  | Renamed ScratchSize PipelineMetadataTypes to ScratchByteSize to be clear that they are not in dwords.  |
 * |   335.0  | Support 128-bit shader hashes by adding and renaming entries in PipelineMetadataType.                  |
 * |   334.0  | Update Pal Interface to support new SC per Shader VGPR Minimization Strategy for bulk code motion.     |
 * |   333.0  | Fix GetShaderDisassembly and modify Result codes.                                                      |
 * |   332.0  | Add additional implicit primitive shader control flags.                                                |
 * |   331.0  | Add Property Id support which allows gfx9 hardware to write an Id to MRT7 without invoking             |
 * |          | the pixel shader. At this time keep this feature from being released in OpenSource.                    |
 * |   330.0  | Video encode include timestamps in feedback structure for vce/uvd engines                              |
 * |   329.0  | Clients of the ElfProcessor<> utility class must call Init() before using the object to generate an    |
 * |          | ELF binary. Calling LoadFromBuffer() does not require a call to Init().                                |
 * |   328.2  | Add MM DRM functionality for DX9 support.                                                              |
 * |   328.1  | Exposes previously internal LocalMemoryType enum through the DeviceProperties struct.                  |
 * |   328.0  | Add VideoDecoderHeap implementation and GetVideoDecoderGpuMemorySize() function for clients.           |
 * |          | Add videoReferenceOnly field in ImageCreateFlags to indicate referenceOnly allocation.                 |
 * |   327.0  | Adds SubmitOptMode to QueueCreateInfo which enables PAL to more aggressively optimize submissions.     |
 * |          | For example, by default PAL may upload some non-exclusive-submit command buffers to local memory.      |
 * |          | Clients which always use exclusive-submit command buffers should prefer Disabled over Default.         |
 * |   326.0  | Add some more device in NullGpuId.                                                                     |
 * |   325.1  | Add GetActive10BitPackedPixelMode() interface to return  the supported 10-bit/packed-pixel mode and    |
 * |          | RequestKmdReinterpretAs10Bit() to inform the KMD that present dst allocation must be reinterpreted as  |
 * |          | 10-bit.                                                                                                |
 * |   325.0  | Add window mode present on Windows RS3 for Vulkan and Mantle                                           |
 * |   324.0  | Add userDataShaderUsage flags to Indirect Params                                                       |
 * |   323.0  | Add peerWritable flag to PresentableImageCreateInfo                                                    |
 * |   322.0  | Set matchStencilTileCfg to 1 if there could be stencil resolve for resolveSrc depth-stencil. And set   |
 * |          | decompressOnZPlanes to 1 for D16S8 format, which is essential for tc read. Client driver shall be aware|
 * |          | that if there could be stencil resolve, noStencilShaderRead should be set 0 for the resolveSrc depth-  |
 * |          | stencil image to avoid corruption on Gfx8 Asics, since shader read based resolve might be performed.   |
 * |   321.5  | Eliminate the requirement of having major interface version >= 314 before exposing descTableVaStart    |
 * |          | and shadowDescTableVaStart in DeviceProperties::gpuMemoryProperties.                                   |
 * |   321.4  | Add gpuEmulatedInSoftware flag to DeviceProperties::pciProperties.                                     |
 * |   321.3  | Add offchipTessBufferSize and tessFactorBufSizePerSe to DeviceProperties::gfxipProperties.             |
 * |   321.2  | Fix typo in interface; the preciseAnsio flag is now called preciseAniso.                               |
 * |   321.1  | This version was accidentally skipped.  Oops!                                                          |
 * |   321.0  | This version was accidentally skipped.  Oops!                                                          |
 * |   320.3  | Add CmdCopyImageToPackedPixelImage interface which convert rendered contents from application          |
 * |          | primaries into packed pixel on the scratch surface.                                                    |
 * |   320.2  | Add physicalHeapSize to GpuMemoryHeapProperties. When HBCC(High Bandwidth Cache Controller is enabled, |
 * |          | certain heaps may be virtualized and the logical size(heapSize) will exceed the physical size.         |
 * |   320.1  | Add support to do full range stall when ppImage is null for rangeCheckedTargetWait barrier.            |
 * |   320.0  | Add p2pCopyToInvisibleHeapIllegal engine property.  Clients must avoid performing P2P copies from      |
 * |          | engines that set this flag.                                                                            |
 * |   319.1  | Add UnregisterTimedQueue method to GpaSession to better support clients that allow destroying and      |
 * |          | recreating command queue's with the same handle.                                                       |
 * |   319.0  | Add enableUmdFpsCap and umdFpsCapFrameRate in DeviceProperties::osProperties                           |
 * |   318.2  | Add ldsSizePerThreadGroup, gsVgtTableDepth, and gsPrimBufferDepth, and primitive shader info to the    |
 * |          | shaderCore part of DeviceProperties.                                                                   |
 * |   318.1  | Add two flags in osProperties to report if KMD supports Creator Who Game(CWG) driver and if KMD is in  |
 * |          | gaming mode.                                                                                           |
 * |   318.0  | GpaSession now uses the platform's allocator callbacks instead of GenericAllocatorAuto (jemalloc). The |
 * |          | GpaSession constructor now takes an IPlatform argument. IPlatform is now a fully qualified allocator   |
 * |          | type.                                                                                                  |
 * |   317.2  | Add CmdWriteImmediate interface which allows the client to write out 32 or 64bit data to a given       |
 * |          | gpu address.                                                                                           |
 * |   317.1  | Add IShaderCache::Merge to support vkMergePipelineCaches.                                              |
 * |   317.0  | Added new Error Results: ErrorUnsupportedPipelineElfAbiVersion and ErrorInvalidPipelineElf used during |
 * |          | Pipeline creation. Added gpusize to Util namespace.  Modified Pipeline ABI Notes.                      |
 * |   316.0  | Add expectedEntries field to ShaderCacheCreateInfo to allow the clients to control the HashMap size.   |
 * |   315.1  | Add Reset function to IShaderCache to allow the client to reset the shader cache and free the memory   |
 * |          | that the cache has allocated for shaders.                                                              |
 * |   315.0  | Add new engine types -- HighPriority "universal" and "graphics".                                       |
 * |   314.0  | Add descTableVaStart and shadowDescTableVaStart in DeviceProperties::gpuMemoryProperties.              |
 * |   313.0  | Switch to open source-able ASIC ID header. Clients who referenced AsicRevision::Ellesmere, ::Baffin,   |
 * |          | or ::Lexa need to change those to ::Polaris10, ::Polaris11, or ::Polaris12, respectively. Some ASICs   |
 * |          | in AsicRevision and NullGpuId enums are gated by PAL_CLOSED_SOURCE; clients who reference these ASICs  |
 * |          | must compile their sources with PAL_CLOSED_SOURCE=1.                                                   |
 * |   312.0  | Add peerWritable flag bit in Pal::GpuMemoryCreateFlags to notify whether a memory can be open as peer  |
 * |          | memory and be writable.                                                                                |
 * |   311.0  | Add predication support for video decode. Modify CmdSetPredication interface for physical engines.     |
 * |   310.0  | Add support for overriding global PBB setting per pipeline.                                            |
 * |   309.0  | Add clientInternal pipeline flag for internal pipelines created by client drivers.                     |
 * |   308.0  | Add PAL_SUPPORTED_IL_MAJOR_VERSION. All clients should modify their AMDIL converters to safely only    |
 * |          | generate code up to PAL's supported major version.                                                     |
 * |   307.1  | Add function MemoryBarrier to issue full memory barrier.                                               |
 * |   307.0  | Add U8V8_Snorm_L8W8_Unorm and U10V10W10_Snorm_A2_Unorm for DX9 mixed signed unsigned format support.   |
 * |   306.0  | Add resolveDst usage flag bit in Pal::ImageUsageFlag to suggest setting nonSplit flag for depth-only   |
 * |          | resolveDst surface on Gfx7/Gfx8. Previously PAL assumed that all single sample images were a potential |
 * |          | target of a resolve, so clients should default to setting resolveDst to 1 for such images to maintain  |
 * |          | compatible behavior.                                                                                   |
 * |   305.0  | Adds support for creating pipelines using the Pipeline ELF ABI.  Skip PAL shader creation through SC   |
 * |          | and instead set the pPipelineBinary field in GraphicsPipelineCreateInfo to a precompiled pipeline ELF. |
 * |   304.0  | Gives the client full control over which SC optimization options are enabled for each pipeline.        |
 * |          | Clients who previously were setting PipelineCreateFlags::enableFastCompile should instead set the      |
 * |          | disableOptimization[C1|C2|C3|C4] to 1.                                                                 |
 * |   303.0  | Remove legacy UVD/VCE path. Add PAL_BUILD_VIDEO flag to video decode/encode queue.                     |
 * |   302.0  | Expose csTgPerCu to clients. This will allow clients to selectively tune this value for specific       |
 * |          | compute shaders for performance tuning.                                                                |
 * |          | @ref ShaderOptimizationStrategy::csTgperCu                                                             |
 * |   301.1  | Add support for virtual display.                                                                       |
 * |   301.0  | Add pixelShaderInvokeMask to MsaaStateCreateInfo. This new flag is only supported when the             |
 * |          | @ref DeviceProperties::gfxipProperties::flags::supportPsInvokeMask support flag is set.                |
 * |   300.0  | Add a flag pageMigrationEnabled to gpuMemoryProperties structure to report PageMigrationSupport to UMD |
 * |          | Removed the migrationSupport flag in that same structure; clients can assume all PAL supported         |
 * |          | platforms support whole allocation migration between submissions.                                      |
 * |   299.0  | Add switchWinding flag to PipelineShaderInfo for Vulkan driver to configure tessellation vertex order. |
 * |   298.0  | Add 64-bit support to settings, via the "HEX64_STR" encoding.                                          |
 * |   297.0  | Add video encode H264 VUI parameters configuration support                                             |
 * |   296.0  | Add some interface changes for Vulkan TurboSync 2.0 support                                            |
 * |   295.0  | Support video encode high-frequency (per frame) rate control for both codecs H264 & H265.              |
 * |   294.0  | Add use non-ieee fp flag to shader optimization flags.                                                 |
 * |   293.0  | Add shader optimization flag for the SC option to enable XNack support.                                |
 * |   292.0  | Add new tilingPreference into imageCreateInfo. It specifies preferred tiling organization for an image.|
 * |   291.0  | Add SC option for aggressive-hoist.                                                                    |
 * |   290.0  | Adding Nt handle caps to osProperties and Adding ntHandle to GpuMemoryCreateFlags,                     |
 * |          | QueueSemaphoreCreateInfo as well as ExternalQueueSemaphoreOpenInfo                                     |
 * |   289.0  | Add a setting to PipelineShaderInfo for controlling the wave break size.                               |
 * |   288.0  | Add windowedPriorBlit flag to QueueCreateInfo to inform KMD no need to blt the surface in DdiPresent.  |
 * |   287.0  | Add per-shader-stage control of the waveSize                                                           |
 * |   286.0  | Remove externally visible helper functions from palGpaSession.h.                                       |
 * |   285.0  | Query codec specific capabilities from PAL; Modify the interfaces of VceIpProperties & UvdIpProperties |
 * |          | to align with VK defined interface for both codecs H264 & H265                                         |
 * |   284.0  | Change cubeMap flag in SamplerInfo to seamlessCubeMapFiltering. Correct the DISABLE_CUBE_WRAP logic.   |
 * |   283.0  | Track memory loads as part of the graphics push/pop state. This requires change to the                 |
 * |          | CmdLoadMsaaQuadSamplePattern api.                                                                      |
 * |   282.3  | Add supportBankPipeSwizzle flag in UvdIpProperties and VcnIpProperties.                                |
 * |   282.2  | Add BigSoftwareReleaseInfo to DeviceProperties.                                                        |
 * |   282.1  | Add new functions to get cpu type and cpu core complex info                                            |
 * |   282.0  | Allow the client to tell Pal if CmdLoadMsaaQuadSamplePattern needs to be used before a depth           |
 * |          | decompress.                                                                                            |
 * |   281.0  | Add property for double rate half precision instructions and rename related properties.                |
 * |   280.0  | Allow loading of sample locations vs immediate mode. This is done by introducing the SamplePattern     |
 * |          | struct, which contains MsaaQuadSamplePattern for immediate mode and a memory address and offset        |
 * |          | if loading of the sample pattern is desired.                                                           |
 * |   279.0  | Replaces the unused prefetchSrds CmdBufferBuildFlags with a new prefetchCommands flag, which pulls     |
 * |          | command data into the GPU cache to improve front-end performance.                                      |
 * |   278.1  | Add new functions CmdWaitBusAddressableMemoryMarker and CmdUpdateBusAddressableMemoryMarker to wait    |
 * |          | and write a marker value to external physical/bus addressable memory respectively. This change also    |
 * |          | stores the markerVA as directly returned by KMD                                                        |
 * |   278.0  | Adds a new shader optimization flag that tells the shader compiler to remove null parameter exports if |
 * |          | possible.                                                                                              |
 * |   277.0  | Add resolveSrc usage flag bit in Pal::ImageUsageFlag so that Vulkan client driver can set it  when     |
 * |          | msaa image setting Transfer_Src bit.                                                                   |
 * |   276.0  | Add to each public PAL object the ability for the client to assign and retrieve a pointer to arbitray  |
 * |          | client data. This version also removes the CmdBufferCreateInfo::pClientData. This functionality        |
 * |          | has been replaced by the Get/SetClientData.                                                            |
 * |   275.0  | 128-bit shader hash support                                                                            |
 * |   274.0  | Add CmdStoreMsaaQuadSamplePattern and CmdLoadMsaaQuadSamplePattern interface to allow the client to    |
 * |          | store custom sample patterns into depth image memory to load at a later time.                          |
 * |   273.0  | Added query frame QP statistics feedback in video encode for both H264 & H265.                         |
 * |   272.0  | Let client report SDI External Physical Memory that needs to be initialized in SubmitInfo by exposing  |
 * |          | SDI External Physical Memory flag from GpuMemoryFlags to GpuMemoryDesc and adding an array of SDI      |
 * |          | memory to SubmitInfo. Remove UpdateMemorySdi() call from Pal interface. Enable SDI External Physical   |
 * |          | Memory for PAL WDDM1.                                                                                  |
 * |   271.0  | Added InitBusAddressableGpuMemory() to IDevice to handle residency for remote device and support       |
 * |          | Bus Addressable Memory on WDDM1.                                                                       |
 * |   270.0  | Split RGP instrumentation version into instrumentation spec version and api version.                   |
 * |   269.1  | Moved early phase barrier metadata initialization BLTs into late phase for GFX6, GFX9, and SDMA.       |
 * |          | Clients may now safely remove two-stage barrier code.                                                  |
 * |   269.0  | Switched IGpuEvent from IGpuMemoryBindable to internally managed GPU memory, added GpuEventCreateInfo  |
 * |          | and IDevice::CreateGpuEvent is now non-const.                                                          |
 * |   268.2  | Added SC option unsafe-convert-to-F16                                                                  |
 * |   268.1  | Adds max on-chip VRAM size to device GPU memory properties.                                            |
 * |   268.0  | Modified external timed queue event functions in GpaSession to accept multiple timestamps.             |
 * |   267.0  | Added a new postFrameTimerSubmission flag to FullScreenFrameMetadataControlFlags.                      |
 * |   266.0  | Made obtaining shader disassembly size in IPipeline::GetShaderStats optional for performance reasons.  |
 * |   265.0  | Modify video encode H264 interfaces to align with H265 and added the corresponding implementation      |
 * |   264.0  | Added implicitPrimitiveShaderControl to GraphicsPipelineCreateInfo to ask the shader compiler to       |
 * |          | convert the hardware vertex shader into a primitive shader that performs various culling and           |
 * |          | compaction within the shader, rather than the fixed-function hardware. Support for the controls can be |
 * |          | found in DeviceProperties::gfxipProperties::flags::supportImplicitPrimitiveShader.                     |
 * |   263.1  | Added supportRgpTraces gfxip device property flag.                                                     |
 * |   263.0  | Add gfxStepping in DeviceProperties and expose revision in all clients.                                |
 * |   262.0  | Updated the ThreadTraceSeLayout struct defined in the IPerfExperiment interface to add information     |
 * |          | about which compute unit the thread trace was collected on.                                            |
 * |   261.4  | Add Lexa, Bermuda, Godavari, Spooky, Maui, Grenada and Bristol enumerations to the                     |
 * |          | AsicRevison list.                                                                                      |
 * |   261.3  | Add Vega10 and Raven enumerations to the AsicRevison list.                                             |
 * |   261.2  | Add ICmdBuffer::CmdUpdateMemorySdi() to initialize PTE for SDI memory with private data                |
 * |   261.1  | Added MSAA constants MaxGridSize, SubPixelBits and SubPixelGridSize in palMsaaState.h.                 |
 * |   261.0  | Updated video encode H265 interface to accomplish UVD-HEVC encoding and added the corresponding        |
 * |          | implementation to operate the basic functionalities of HEVC codec on vega10.                           |
 * |   260.0  | Change the meaning of "revisionId" in DeviceProperties.  It used to be one of the enumerations that    |
 * |          | differentiated between spins within a certain GPU type (TAHITI_A0, TAHITI_A1, etc.).  It is now one    |
 * |          | of the PRID_* constant revision ID values.                                                             |
 * |   259.1  | Added RegisterPipeline to GpaSession. Added shaderOperations flags to IPipeline::ShaderStats.          |
 * |   259.0  | Added attachedScreenCount to DeviceProperties                                                          |
 * |   258.0  | Adds internalTexOptLevel to DeviceFinalizeInfo to override the public setting.                         |
 * |   257.0  | Adds Reset to IQueryPool to improve the performance when we can use CPU to reset the query pool.       |
 * |   256.1  | Adds support for two SC options (useMoreD16 and useUnsafeMAD_MIX).                                     |
 * |   256.0  | Adds ExternalGpuMemoryOpenInfo structure for OpenExternalSharedGpuMemory to support typed buffer       |
 * |   255.1  | Adds support for external timed queue semaphore operations in GpaSession.                              |
 * |   255.0  | A breaking ImageCreateInfo behavior change for DX clients. If the image is flippable, vidPnSourceId    |
 * |          | must always be set to a valid ID or to the new InvalidVidPnSourceId constant.                          |
 * |   254.0  | Adds private data pointer to create pipeline info structs to support RS2 shader caching                |
 * |   253.0  | Adds ChNumFmt::X10Y10Z10W2Bias_Unorm.                                                                  |
 * |   252.0  | Adding the parameter const IGpuMemory& gpuMemory to Device::GetXdmaInfo().                             |
 * |   251.0  | Added Atc, AtcL2, McVmL2, Ea, Rpb, and Rmi GPU block perf counter support. Removed UtcL2 enum; it is   |
 * |          | replaced by McVmL2.                                                                                    |
 * |   250.0  | Added support for rgp queue timing in GpaSession via new Timed* functions.                             |
 * |   249.1  | Added IDevice::DidChillSettingsChange for Chill support.                                               |
 * |   249.0  | Remove supportedModes from SwapChainProperties and move it to DeviceProperties                         |
 * |   248.1  | Adding ExportExternalHandle() for IQueueSemaphore object. This is only for linux build so far.         |
 * |   248.0  | Added late-alloc-vs info in GraphicsPipelineCreateInfo to expose control of late-alloc-vs              |
 * |   247.0  | Added vidPnSourceId in struct ScreenProperties, also expose interface GetFlipStatus to Vulkan.         |
 * |   246.1  | Added Result::ErrorGpuPageFaultDetected                                                                |
 * |   246.0  | Adds support for SVM fine grain system                                                                 |
 * |   245.0  | Added ImageTexOptLevel to ImageViewInfo so that client driver can adjust the texture optimization level|
 * |          | dynamically.                                                                                           |
 * |   244.0  | Enables clients to set custom quad sample locations independent of MsaaState.                          |
 * |          | Removed quadSamplePattern member from MsaaStateCreateInfo structure and added a new interface          |
 * |          | CmdSetQuadSamplePattern. Clients must set correct sample positions before all calls to clear functions |
 * |          | involving images.                                                                                      |
 * |   243.0  | Removed duplicate numPhysicalVGRPs/SGPRs in ShaderStats as it can be obtained from DeviceProperties.   |
 * |   242.0  | Update ValidateMemoryImageRegion to verify the linear memory alignment.                                |
 * |   241.0  | Changed GpaSession constructor to take RGP instrumentation version additionally.                       |
 * |   240.0  | GpaSession adds a timing mode to collects timestamps. GpaSampleConfig is modified to add the new mode. |
 * |   239.0  | Modify debug vm id / sqtt marker behavior to be based off of IsDevDriverProfilingEnabled() instead of  |
 * |          | IsDeveloperModeEnabled(). This means that clients must now enable dev driver traces before any pal     |
 * |          | devices are finalized.                                                                                 |
 * |   238.0  | Add new "ResolveMode" enumeration for CmdImageResolve().                                               |
 * |   237.1  | Added helper function for filling SqttFileChunkAsicInfo required for RGP files.                        |
 * |   237.0  | Add a DrawDispatch developer callback type.  This callback is invoked on every draw or dispatch        |
 * |          | that is written into a command buffer.  It is mainly used to aid clients in generating instrumentation |
 * |          | marker data for SQ thread tracing.                                                                     |
 * |   236.0  | PerfExperimentProperties::blocks is now a flat array indexed by GpuBlock instead of a list of blocks   |
 * |          | which must be searched.  The "Unknown" GpuBlock was removed and the Utcl2 block was added.             |
 * |   235.0  | Added FormatFeatureFlags::FormatFeatureImageFilterMinMax to indicate which formats support             |
 * |          | TexFilterMode::Min and TexFilterMode::Max.                                                             |
 * |   234.0  | Adds Get/StoreValue callbacks to IShaderCache create info to support RS2 shader caching feature.       |
 * |   233.0  | Moved SQTT related flag sqttBadScPackerId from DeviceProperties to PerfExperimentProperties.           |
 * |   232.0  | IQueue::RemapVirtualMemoryPages() now takes a fence argument.                                          |
 * |   231.1  | Added Pal::CmdBufferCreateInfo::pClientData and Pal::ICmdBuffer::GetClientData().  This lets clients   |
 * |          | clients associate arbitrary data with a PAL command buffer.  Is is mainly useful in dealing with PAL   |
 * |          | developer callbacks where PAL calls back with an ICmdBuffer*.                                          |
 * |   231.0  | Add support for Persistent CE RAM.  When creating a universal Queue, the client can request that PAL   |
 * |          | preserves the contents of CE RAM across submissions.                                                   |
 * |   230.3  | Added IDevice::TurboSyncControl Escape and GpuMemoryFlags::turboSyncSurface for TurboSync support.     |
 * |   230.2  | Add a NullGpuId::All enum value.  When PlatformCreateInfo::createNullDevice = 1, the null device       |
 * |          | platform will be initialized with a separate device for each supported null device rather than a       |
 * |          | specific single device.                                                                                |
 * |   230.1  | Changed Util::Snprintf to return the length of the formatted string instead of void.                   |
 * |   230.0  | Removes shader cache max size.                                                                         |
 * |   229.2  | Add "supportPerChannelMinMaxFilter" to DeviceProperties to indicate which GPU's support min/max filter |
 * |          | operations on a per-channel basis.  Requirement for DX12 compliance                                    |
 * |   229.1  | Add some helper functions to check whether the image/buffer copy is supported by the specific engine.  |
 * |   229.0  | Interface change to add Bus Addressable Memory support                                                 |
 * |   228.1  | Added distributed tessellation mode flags to DeviceProperties. Enabled assigning default tessellation  |
 * |          | distribution modes depending on gfxip.                                                                 |
 * |   228.0  | Removed disableInternalShaderCache flag.                                                               |
 * |   227.2  | Added a IPlatform::QueryRawApplicationProfile to get client specific profile.                          |
 * |   227.1  | Added a new supportMinPrecisionFetch flag to DeviceProperties::flags.                                  |
 * |   227.0  | Refactors ISwapChain::AcquireNextImage, splits IQueue::Present into two functions, and changes the     |
 * |          | device and platform properties PAL reports for presents.                                               |
 * |   226.0  | Added a new field swizzleOffset to SubresLayout to support parameterized swizzle. Also added a new     |
 * |          | field swizzleEqTransitionPlane into ImageMemoryLayout.                                                 |
 * |   225.0  | Added support for allowing the platform to automatic determine GPU memory priorities. This feature is  |
 * |          | currently only supported under DX12 on RS2 and higher, and relies on the runtime managing this for us. |
 * |   224.0  | Added maxSqttSeBufferSize to PerfExperimentProperties struct                                           |
 * |   223.0  | Added MGPU support for SVM allocations.                                                                |
 * |   222.0  | Modified GpaSession's GpaSampleConfig structure to contain ShaderMask info, so client can use          |
 * |          | GpaSession to selectively choose which shader stages will be profiled.                                 |
 * |   221.0  | Modified BarrierData callback to provide more detailed cache info. The old BarrierData::caches is      |
 * |          | deprecated. Added new developer CallbackTypes BarrierBegin and BarrierEnd (for RGP barrier annotations)|
 * |   220.1  | Added support for developer driver. The server object can be accessed via GetDevDriverServer() if      |
 * |          | developer mode was enabled on the system during driver startup.                                        |
 * |   220.0  | Changing device callback pointer types to be void so win files can use the correct structure based on  |
 * |          | ddi version                                                                                            |
 * |   219.0  | Reverted shader cache changes added in version 215                                                     |
 * |   218.0  | Removed ICompoundState from PAL                                                                        |
 * |   217.0  | Replaced the capabilities structure in DeviceProperties with an EngineSubType enumeration.             |
 * |   216.0  | Add DX12 Decode query support. Added decode tile output support. Used query pool for feedback buffer.  |
 * |   215.1  | Added a new flag supportVirtualMemoryRemap to engineProperties indicating whether the engine supports  |
 * |          | virtual memory remapping or not.                                                                       |
 * |   215.0  | Removed disableInternalShaderCache as it is no longer necessary/supported with latest shader cache     |
 * |          | changes.                                                                                               |
 * |   214.1  | Added ForwardAllocator which wraps an AllocCallbacks struct with the Allocator concept.                |
 * |   214.0  | Removed maxScratchWavesPerCu from engine properties. Moved numScratchWavesPerCu                        |
 * |          | to the public PAL settings.                                                                            |
 * |   213.2  | Added paddedExtent to SubresLayout.                                                                    |
 * |   213.1  | Added support for dynamic start/stop of per-draw/dispatch GPU profiler logging (for OpenShimInterface).|
 * |   213.0  | Added support of AQBS stereo mode for dx9 .                                                            |
 * |   212.2  | Added IPlatform::GetPrimaryLayout to get the layout of the primary surface.                            |
 * |   212.1  | Added IDevice::GetValidFormatFeatureFlags.                                                             |
 * |   212.0  | Added perfMip field to samplerInfo structure, only works when preciseaniso is not required.            |
 * |   211.0  | Added DoppDesktopInfo and DoppRef structures to handle Display Output Post-Processing (DOPP) desktop   |
 * |          | texture. Added the DoppDesktopInfo field to ExternalResourceOpenInfo.                                  |
 * |          | Added 'doppRefCount' and 'pDoppRefs' fields for SubmitInfo to handle DOPP desktop texture.             |
 * |   210.0  | Added tilingOptMode in ImageCreateInfo for client to select tiling optimization target.                |
 * |   209.0  | Added support for real time queues                                                                     |
 * |   208.0  | Interface change to add SVM(Shared Virtual Memory) support.                                            |
 * |   207.1  | Added Util::CollapseResults to help combine multiple Results into a single Result.                     |
 * |   207.0  | Added an EngineType field to CmdBufferCreateInfo.                                                      |
 * |   206.0  | Interface change to add support for firstShaderWritableMip in PAL. Now, clients can tell PAL which is  |
 * |          | the first mip among the whole mip chain with usage shader writable. PAL may choose to enable DCC on    |
 * |          | those mips which are not shader writable.                                                              |
 * |   205.0  | Separated the concepts of queues and engines. Add a new enum EngineType which will help map queues to  |
 * |          | the engines that support them.                                                                         |
 * |   204.1  | Added support for Flgl functionality.                                                                  |
 * |   204.0  | Interface change to use the new DispatchAqlParams structure for CmdDispatchAql().                      |
 * |   203.1  | Added Util::AtomicExchange64 to palMutex.h                                                             |
 * |   203.0  | Added UVD IP property structure. Added new UVD version support (6, 6.2, 6.3, 7).                       |
 * |          | Added InitializeGpuChipProperties for UVD IP.                                                          |
 * |   202.0  | Interface change to remove support for nativeResolve optimization from PAL. Now, pal handles it,       |
 * |          | without  any explicit hint from clients.                                                               |
 * |   201.0  | Interface change to add auxiliary buffer for UVD decode. Add statusReportNum for feedback status.      |
 * |          | Added new codec support HEVC, HEVC10bit and VC1.                                                       |
 * |   200.0  | Added the flag initUndefZero to PipelineShaderInfo to initialize undefined IL registers to zero.       |
 * |   199.0  | Added IPrivateScreen::EnableAudio() and a new 'hasAudio' field for PrivateScreenProperties.            |
 * |   198.1  | Replaced the PAL default allocator with jemalloc, a much more efficient open sourced allocator.        |
 * |   198.0  | Included UVD interface header from its official location.                                              |
 * |   197.0  | Added 'occlusionQuerySamples' to MsaaStateCreateInfo.  Clients should program this when they add       |
 * |          | support for EQAA.                                                                                      |
 * |   196.0  | Added flags parameter to CmdClearColorImage() and CmdClearDepthStencil() to support a new "auto sync"  |
 * |          | mode for clears.                                                                                       |
 * |   195.0  | Added the 'disableBusyChunkTracking' to CmdAllocatorCreateFlags.  This allows clients to enable auto   |
 * |          | memory reuse without also enabling busy chunk tracking anymore.  If the new flag is unset, legacy      |
 * |          | behavior will be preserved.                                                                            |
 * |   194.1  | Add property flag for primitive ordered PS support                                                     |
 * |   194.0  | Add support for creating a null device for off-line shader compilation                                 |
 * |   193.0  | Packed the CmdScaledCopyImage's parameters into the struct ScaledCopyInfo.  Also added ScaledCopyFlags |
 * |          | to support dest color key copy.                                                                        |
 * |   192.1  | Added IPipeline::GetShaderStats to retrieve pre and post-compilation shader stats.                     |
 * |   192.0  | Added UVD Carizzo support.  Added patching list for UVD engine. Added UVD no-op register padding.      |
 * |          | Added internal CmdAllocateEmbeddedData to return GpuMemory pointer and offset.                         |
 * |          | Enable UVD linux support.                                                                              |
 * |   191.3  | Added CmdVirtualQueueHandshake(), CmdVirtualQueueDispatcherStart(), CmdVirtualQueueDispatcherEnd()     |
 * |          | to ICmdBuffer class and GetDispatchKernelSource() to Idevice for device enqueue support in OpenCL2.0.  |
 * |   191.2  | Added IDevice::SetClockMode to set the device engine and memory clocks to pre-defined modes.           |
 * |   191.1  | Added preferredHeap and some flags to GpuMemoryDesc.                                                   |
 * |   191.0  | Moved pMemAllocator from CmdBufferCreateInfo into CmdBufferBuildInfo to support additional clients.    |
 * |          | Added Util::VirtualLinearAllocatorWithNode and made VirtualLinearAllocator's destructor virtual.       |
 * |   190.0  | Added X10Y10Z10W2 for Snorm, Sscaled, and Sint, and also added indications that certain hardware might |
 * |          | not support 2-bit signed values.  Removed the Snorm override in ImageViewInfo.                         |
 * |   189.0  | Added flag useAnisoThreshold and anisoThreshold for use in IDevice::CreateSamplerSrds                  |
 * |   188.1  | Added nullSrd structs to initialize srds when the descriptor is NULL instead of a memset to 0          |
 * |   188.0  | Added force32BitVaSpace flag into PlatformCreateInfo.  That allows to force 32 bit VA space            |
 * |          | for the flat address instruction with 32 bit ISA                                                       |
 * |   187.0  | Added LDS spill size SC shader optimization support.                                                   |
 * |   186.1  | Overloads GetExecutableName for wide character strings and modifies return type to Pal::Result.        |
 * |   186.0  | Adds actualSwitchInfo into CustomPowerProfile as output.                                               |
 * |   185.0  | Added flag forceAnisoMaxThresh for use in IDevice::CreateSamplerSrds                                   |
 * |   184.0  | Added enableFastCompile flag to PipelineCreateFlags.                                                   |
 * |   183.2  | Adds Util support for AtomicExchange, thread-local data (ThreadLocalKey), and JsonWriter.              |
 * |   183.1  | Added gpuConnectedViaThunderbolt flag to DeviceProperties.                                             |
 * |   183.0  | Added disableInternalShaderCache flag into PlatformCreateInfo.                                         |
 * |   182.0  | Added disableGpuTimeout flag into PlatformCreateInfo. It can disable TDR feature (Windows only)        |
 * |   181.0  | Removed IQueue::GetPresentSupport and added supportedPresentModes to the per-engine DeviceProperties.  |
 * |          | Replaced the PresentModeFlags enum with SwapChainMode and SwapChainModeSupport.  SwapChainProperties   |
 * |          | now reports a mask of supported SwapChainModes for each PresentMode.                                   |
 * |   180.0  | Add a flag forceSnorm in ImageViewInfo to handle the special case of DX9 accessing D3DFMT_A2W10V10U10. |
 * |   179.0  | Refactor ICmdBuffer::CmdClear* so that clear colors are always specified in RGBA order.  Also changed, |
 * |          | are Formats::PackRawClearColor and Formats::ConvertColor, and added Formats::SwizzleColor.             |
 * |   178.0  | Modifies IDevice::AddGpuMemoryReferences to take a list of GpuMemoryRef structs.                       |
 * |   177.0  | Added support to new frame metadata 'TimerNodeSubmissionMode'                                          |
 * |   176.0  | Added UVD H264 decoding support. Added video decode pipeline support.                                  |
 * |   175.0  | Added support to CmdPresent to use permanent typed buffers as the present source or destination.       |
 * |   174.0  | IGpuMemory objects can be permanently cast as typed buffers at creation.                               |
 * |   173.1  | Changed the parameters in QueryAllocationInfo() for Iqueue and Ipipeline.                              |
 * |   173.0  | Added external memory allocator to CmdBufferCreateInfo.                                                |
 * |   172.2  | Clarifies the behavior of CmdDraw* and CmdDispatch* functions with zero indices/instances/threads.     |
 * |   172.1  | Added QueryAllocationInfo() to IPipeline/IQueue. Added GpuMemSubAllocInfo to palGpuMemory.h            |
 * |   172.0  | Added CustomPowerProfile to IDevice::SetPowerProfile().                                                |
 * |   171.0  | Merge ChFmt and NumFmt into one ChNumFormat enum.                                                      |
 * |   170.0  | Gave access to AsicRevision for DX12 clients and made HashBase group size a non-template parameter     |
 * |          | to allow clients to specify their own sizes - initially needed by DX12's Shader Hive.                  |
 * |   169.3  | Added AddShadersToCache() to IPipeline to support adding pipeline shaders to a cache after creation.   |
 * |   169.2  | Report PCI location information for the device in DeviceProperties                                     |
 * |   169.1  | Added maxCusPerShaderArray to DeviceProperties for reporting count of physical CUs (before harvesting).|
 * |   169.0  | Added support for clients to read settings from the global scope (controlled by CCC)                   |
 * |   168.0  | Added support for IShaderCache objects to support API level PSO Library features.                      |
 * |   167.0  | Replaced PAL's implicit written primaries tracking with an explicit "blockIfFlipping" list in Submit.  |
 * |   166.0  | Changed CmdClearImageView(), so it takes 2d Rects to clear image view instead of 3d boxes.             |
 * |   165.0  | Added queue semaphore max counts. Extra signals will be dropped by the OS.                             |
 * |   164.0  | Add new parameter in ICmdBuffer::CmdScaledCopyImage to support copy with color key.                    |
 * |   163.0  | Added per-shader VGPR limit alteration ability.                                                        |
 * |   162.0  | Added privateApertureBase and sharedApertureBase to gpuMemoryProperties. It's used for generic address |
 * |          | space implementation in OCL2.0. Added vramBusBitWidth to gpuMemoryProperties                           |
 * |   161.1  | Added QueryAllocationInfo() which returns a list of GPU memory allocation used by the allocator.       |
 * |   161.0  | Changed PAL policy to accept NumFmt::Ds only for images with separate depth/stencil planes.            |
 * |   160.0  | Added flags in DeviceFinalizeInfo to indicate desired shared memory types to initialize. Added         |
 * |          | fullscreen frame metadata caps flags.                                                                  |
 * |   159.1  | Added PresentModeFifoRelaxed to PresentModeFlags                                                       |
 * |   159.0  | Added channels mapping to BufferViewInfo.                                                              |
 * |   158.0  | Added video encode feedback query pools.                                                               |
 * |   157.1  | Added minLinearMemCopyAlignment to DeviceProperties.                                                   |
 * |   157.0  | Added additional information to DeviceProperties to support RGP; added a new ICmdBuffer interface,     |
 * |          | CmdInsertRgpTraceMarker, to support RGP; added a new API version argument specified to PAL when        |
 * |          | creating a platform for use by RGP.                                                                    |
 * |   156.0  | Add new parameter in ICmdBuffer::CmdScaledCopyImage to support rotated copies                          |
 * |   155.0  | Add support for Color-Target Views of YUV Images.  Clients must now specify an Image aspect which the  |
 * |          | view is associated with.                                                                               |
 * |   154.0  | Added BarrierType::SyncCpDma to indicate when a CmdBarrier waited for CP DMA blts to finish.           |
 * |   153.0  | Removed requireUploadToGpuMem flag and UploadToGpuMemory() from IGpuMemoryBindable interface.          |
 * |   152.1  | Added pipeline support for shader disassembly.                                                         |
 * |   152.0  | Added initial UVD (Unified Video Decoder) support.                                                     |
 * |   151.1  | Adding DeterminePresentationSupported interface for API layer to query whether the presentation is     |
 * |          | supported on platform with certain configuration.                                                      |
 * |   151.0  | Modifies pipelines to manage their own GPU memory.                                                     |
 * |   150.0  | Add a flag to ImageViewInfo to indicate that a view should see internal padding of the image. Add      |
 * |          | ICmdBuffer::CmdCopyTiledImageToMemory() and ICmdBuffer::CmdCopyMemoryToTiledImage()                    |
 * |   149.0  | Added new format P8 to support 8 bpp primary surface mode.                                             |
 * |   148.0  | Add BlendZeroMode bit in SamplerInfo flag to support PRT.                                              |
 * |   147.0  | Added TriangleRasterStateParams::flags.depthBiasEnable to control whether depth bias (polygon offset)  |
 * |          | is applied to triangle-based primitives.  To match previous interface behavior, set this flag always   |
 * |          | to 1.                                                                                                  |
 * |   146.0  | Add z offset and z range when creating an unordered access view of a 3d texture resource in            |
 * |          | ImageViewInfo.                                                                                         |
 * |   145.0  | Add initial support for YUV formats: AYUV, UYVY, VYUY, YUY2, YVY2, YV12, NV11, NV12, NV21, P016 and    |
 * |          | P010.                                                                                                  |
 * |   144.1  | Added support for graphics shader throttling using maxWavesPerCu.                                      |
 * |   144.0  | Added new generic GDS management support API and new IDevice::AllocateGds interface for fine grained   |
 * |          | GDS allocation on a per-engine basis.                                                                  |
 * |   143.0  | For a windows allocation sharing hDxRtResource with an existing allocation, do not call DeallocateCb.  |
 * |          | GpuMemoryCreateInfo::isChildAllocation is added to do the job.                                         |
 * |   142.1  | Add Util::MkDirRecursively() and change prototype of Utill::FileMapping::GetHandle() on Linux          |
 * |   142.0  | Change timeout of Pal::Device::WaitForFences from float to be uint64.                                  |
 * |          | the timeout will be nano-second based but not second.                                                  |
 * |   141.1  | Adds ICmdBuffer::CmdXdmaWaitFlipPending which stalls until there are no XDMA flips pending.            |
 * |   141.0  | Allow client to use previously reserved GPU VA for creating a memory object.                           |
 * |   140.0  | Add a flag to image-create to allow Z and stencil aspects to be initialized separately.  In essence    |
 * |          | this really disables stencil compression.                                                              |
 * |   139.0  | Consolidate primitive restart related fields into new CmdSetInputAssemblyState interface function and  |
 * |          | deprecate CmdSetPrimitiveTopology.                                                                     |
 * |   138.0  | Added CmdBufInfoList parameter to Pal::IQueue::Submit. Clients using KMD-calculated-time Frame Pacing  |
 * |          | can provide additional CmdBufInfo for KMD along with the command buffers during submit.                |
 * |   137.0  | Add a new flag absoluteDepthBias to handle DX9 depth bias                                              |
 * |   136.0  | Added maxWavesPerCu to PipelineShaderInfo, allowing clients to throttle shader work.                   |
 * |   135.0  | Move "depthAsZ24" flag from the ImageViewInfo structure to ImageUsageFlags of ImageCreateInfo structure|
 * |   134.0  | Added fixedTileSwizzle and tileSwizzle to ImageCreateInfo to support explicit tile swizzle. Changed    |
 * |          | the SubresLayout values added in version 126.0 to use an Extent3d.                                     |
 * |   133.0  | Added perpLineEndCapsEnable to GraphicsPipelineCreateInfo::rState to control line rasterization.       |
 * |   132.2  | Updated CmdResolveImage specification for integer NumFmt image resolve to simply copy first sample.    |
 * |   132.1  | Added VirtualLinearAllocator::Remaining, which returns the free space remaining in the allocator.      |
 * |   132.0  | Added PinnedGpuMemoryCreateInfo, realMemAllocGranularity, and virtualMemAllocGranularity.  Removed     |
 * |          | GpuMemoryHeapProperties::pageSize.  Note that the allocation granularities may be as low as 4KB on     |
 * |          | some platforms so clients may want to align certain real allocations to the fragmentSize manually.     |
 * |          | See the IGpuMemory documentation for an explanation of the new GPU memory allocation rules.  Clients   |
 * |          | are also required to remove all references to the enable4kPageSize PAL setting.                        |
 * |   131.0  | Added a new flag repetitiveResolve to ImageCreateFlags for a possible optimization for CmdResolveImage.|
 * |   130.0  | Added globalGpuVa to GpuMemoryCreateInfo to indicates the GPU virtual addresses are visible to all     |
 * |          | devices. This feature is only supported if the globalGpuVaSupport DeviceProperty flag is set.          |
 * |   129.0  | Added xdmaBuffer flag to GpuMemoryCreateInfo in PAL. Clients can now create XDMA cache buffer.         |
 * |   128.0  | Changed the structure of VirtualMemoryRemapRange and VirtualMemoryCopyPageMappingsRange to use BYTE    |
 * |          | unit instead of PAGE unit.                                                                             |
 * |   127.0  | Added dx9Mipclamping field in SamplerInfo structure for client to control mip clamping style.          |
 * |          | Added vFaceIsFloat flag in PipelineShaderInfo for client to control face register type in pixel shader.|
 * |   126.0  | Added tileSwizzle, blockWidth, blockHeight, and blockSlices to SubresLayout to support DX12            |
 * |          | parameterized swizzle.                                                                                 |
 * |   125.0  | Added maxAsyncComputeThreadGroupSize field to DeviceProperties which reports what the maximum number   |
 * |          | of threads-per-group in a compute shader is safe to use while also using the async compute queues. To  |
 * |          | maintain backwards-compatibility, non-DX12 clients should compute min(maxAsyncComputeThreadGroupSize,  |
 * |          | maxAsyncComputeThreadGroupSize) when reporting the device's maximum supported thread-group size.       |
 * |   124.1  | Added Result::OutOfSpec as a success with warning.                                                     |
 * |   124.0  | Removed the HwPipePoint for CmdSetMarker and added enableFullPipelineMarkers to CmdBufferBuildFlags    |
 * |          | the future needs of high performance marker data.                                                      |
 * |   123.0  | Added descrVirtAddr to GpuMemoryCreateInfo and shadowFmask to PipelineShaderInfo to support FMask      |
 * |          | shadow descriptors.                                                                                    |
 * |   122.0  | Clients must now include palSpecifiedScDefs before building SC as described in the doxygen "Build"     |
 * |          | page.                                                                                                  |
 * |   121.0  | Added VCE (Video Compression Engine) queue.                                                            |
 * |   120.0  | Modified ShaderCreateInfo in PAL. Client's can now choose optimal VGPR minimization strategy per shader|
 * |          | For this to work minimizeVgprs flag must be set to true.                                               |
 * |   119.0  | Added priority bias control to SetPriority(). This allows the OS to prioritize allocations within      |
 * |          | the same base priority level.                                                                          |
 * |   118.0  | Add AQL compute queue support: ICmdBuffer::CmdDispatchAql() and QueueCreateInfo.aqlQueue.              |
 * |          | New DeviceProperties.shaderCore structure describes the computational power of the device.             |
 * |   117.0  | Added topologyAllowsPrimitiveRestart in structure PrimitiveTopologyParams, so that clients can control |
 * |          | whether to allow the reset index feature being enabled per primitive topology. Clients always set      |
 * |          | topologyAllowsPrimitiveRestart to true in order to maintain consistent behavior with older interface   |
 * |          | versions.                                                                                              |
 * |   116.0  | Modified GpuMemoryCreateInfo in PAL. Client's can now bias priority within Same Base Level and hence   |
 * |          | they can Hint OS to treat allocations within same Level with different priority.                       |
 * |   115.0  | Renamed PrtFeaturePerLayerMipTail to PrtFeaturePerSliceMipTail.                                        |
 * |          | Renamed prtMipTailTileCountPerSlice to prtMipTailTileCount.                                            |
 * |   114.0  | Added members in structure ViewportParams to control guard band settings.                              |
 * |          | Client should set the clip guardband ratios to FLT_MAX, and the discard guardband ratios to 1.0f       |
 * |          | for backwards compatibility with earlier interface versions.                                           |
 * |   113.0  | Add HwPipePoint to ICmdBuffer::CmdSetMarker.                                                           |
 * |   112.0  | Added support to force out-of-order primitive rasterization for a graphics pipeline.                   |
 * |   111.0  | Added several formats which are used by DX9.                                                           |
 * |   110.0  | IDevice::CreateCmdBuffer is no longer const.                                                           |
 * |   109.0  | Renamed Md5::Compact to Md5::Compact64 and added an Md5::Compact32.                                    |
 * |   108.1  | Added m_initialized to Mutex to solve deleting uninitialized OS object issue.                          |
 * |   108.0  | Added fine grained PRT feature flags, replacing existing PRT support level enum.                       |
 * |   107.0  | Changed the types of the TexFilter back to uint32 because GCC (rightfully) issues a warning.           |
 * |          | The relevant enums have been switched to non-scoped enums as an alternative way to remove the casts.   |
 * |   106.0  | Added on-disk file backing for the shader cache along with related util classes.                       |
 * |   105.0  | Added the ability for clients to install developer callbacks which can be installed into an IPlatform. |
 * |   104.0  | Modified AddGpuMemoryReferences implementation to support returning Result::NotReady. This will only   |
 * |          | happen when a client needs to wait on a paging fence that they passed in.                              |
 * |   103.0  | Changed the types of the TexFilter bitfield so that the clients don't have to cast enums to use it.    |
 * |   102.0  | Added pointSizeMin/pointSizeMax in PointLineRasterStateParams structure to control the min/max point   |
 * |          | size rasterization state. Remove the PA_SU_POINT_MINMAX register programming from preamble command     |
 * |          | stream, all clients now need to explicitly set the min/max point size to maintain previous behavior.   |
 * |          | Added rasterizeLastLinePixel flag in GraphicsPipelineCreateInfo::rsState structure to control whether  |
 * |          | to draw last pixel in a line or not, clients can leave this flag as 0 if they do not use it.           |
 * |   101.1  | Added IPrivateScreen::GetHdcpStatus().                                                                 |
 * |   101.0  | Added truncCoord bit to handle DX9 trunc texture coordinate when mip and mag filter are point          |
 * |          | Added cubeMap bit to handle disableCubeWrap bit                                                        |
 * |   100.1  | Added new error codes: from ErrorPrivateScreenInvalidFormat to ErrorPrivateScreenInvalidScaling.       |
 * |   100.0  | Added pPsTexWrapping and numPsTexWrappingEntries in PipelineShaderInfo structure                       |
 * |    99.0  | Add wsiPlatform field to SwapChainCreateInfo.                                                          |
 * |          | Add wsiPlatform parameter to IDevice::GetSwapChainInfo to indicate the wsi platform                    |
 * |    98.0  | Added pDstImage in PresentInfo structure to support DX9 blit present.                                  |
 * |          | Added vidPnSourceId in ImageCreateInfo structure to record primary surface vidPn source Id             |
 * |    97.0  | Adds the minimizeVgprs flag to ShaderCreateFlags. This flag indicates the client would like PAL to     |
 * |          | compile the shader in a way that minimizes the number of VGPRs used.                                   |
 * |    96.0  | Adds the prefSwizzleEqs flag to ImageCreateFlags. This flag instructs PAL/AddrLib to return an invalid |
 * |          | swizzle equation if a chosen tile mode does not support swizzle equations. This would allow AddrLib to |
 * |          | avoid downgrading tile mode.                                                                           |
 * |    95.0  | Adds the hiZNeverInvalid flag to ImageUsageFlags. Setting it allows clients to give a hint to PAL that |
 * |          | the Image never needs a resummarization blit because Hi-Z metadata will never become out-of-sync with  |
 * |          | the Image's pixel data.                                                                                |
 * |    94.0  | Added a new flag exclusiveCompute to DeviceFinalizeInfo                                                |
 * |    93.0  | Added usrClipPlaneMask in GraphicsPipelineCreateInfo to control user defined clip plane setting.       |
 * |          | Added ICmdBuffer::CmdSetUserClipPlanes function for client to set user defined clip planes.            |
 * |    92.1  | Added a new abstract method to ISwapChain                                                              |
 * |    92.0  | Refactored TexFilter so that individual fields can be set orthogonally                                 |
 * |    91.0  | Add noStencilShaderRead flag to ImageUsageFlags.                                                       |
 * |    90.1  | Add fragmentSize to DeviceProperties.                                                                  |
 * |    90.0  | Added ErrorFenceNeverSubmitted, which replaced ErrorUnavailable returned in IDevice::WaitForFence and  |
 * |          | IFence::GetStatus to ease the mapping to different APIs' error code.                                   |
 * |    89.0  | Changed IntervalTree::GetNull() from a static class function to a const member function.  Only Mantle  |
 * |          | used this function, and they have made adjustments on their side to prevent breaking overlays.         |
 * |    88.0  | Added client hash communication to PAL.  Clients not providing their own shader hashes will use PAL's  |
 * |          | internal hash computation.                                                                             |
 * |    87.0  | Add doNotWait flag to IQueue::RemapVirtualMemoryPages() and IQueue::CopyVirtualMemoryPageMappings().   |
 * |          | It allows clients to update page mappings without waiting for previous rendering to finish.            |
 * |    86.0  | Fixed client major version define mismatches.                                                          |
 * |    85.0  | Added new enum ResourceMappingNodeType::InlineSrvConst for Vulkan.                                     |
 * |    84.0  | Added flags to AddGpuMemoryReferences. It is expected that all clients will want to set some of these  |
 * |          | flags (for example clients that don't support allocation trimming should set GpuMemoryRefCantTrim).    |
 * |    83.2  | Added IDevice::QueryDisplayConnectors() method.                                                        |
 * |    83.1  | Added Clear() method to Util::IntervalTree().                                                          |
 * |    83.0  | Add disallowNestedLaunchViaIb2 flag to CmdBufferBuildFlags. This flag should be cleared to preserve    |
 * |          | legacy behavior.                                                                                       |
 * |    82.0  | Support for conservative rasterization.                                                                |
 * |    81.0  | Add blendSourceAlphaToColor to GraphicsPipelineCreateInfo.                                             |
 * |    80.1  | Added minTimestampAlignment to DeviceProperties.                                                       |
 * |    80.0  | Added shadeMode and dx9PixCenter in pipeline create structure, add outputWinCoord in shader create     |
 * |          | structure to indicate whether vertex shader output window coordinates.                                 |
 * |    79.0  | Added stride parameter to QueryPool::GetResults.                                                       |
 * |    78.1  | Added FormatFeatureFormatConversionSrc and FormatFeatureFormatConversionDst to indicate whether or not |
 * |          | images are supported as source and destination images in format conversion image copies.               |
 * |    78.0  | Added ChFmt::G4R4.  This may break any client lookup tables that index by ChFmt.                       |
 * |    77.1  | Added Formats::LinearToGamma() and Formats::GammaToLinear() utility functions.                         |
 * |    77.0  | Added FormatFeatureImageFilterLinear to indicate whether a format supports linear filtering.           |
 * |    76.0  | ImageScaledCopyRegion is modified such that srcExtent and dstExtent are now of type SignedExtent3d.    |
 * |    75.0  | Add new ChFmt entries for 16-bit formats.  This will shift ChFmt opcode values and break any client    |
 * |          | lookup tables.                                                                                         |
 * |    74.1  | Added perSubresInit to ImageCreateFlags and clarified the barrier requirements for it.                 |
 * |    74.0  | Adds IPrivateScreen::SetEventAfterVsync, IQueue::DelayAfterVsync and updates PowerProfile enum as well |
 * |          | as a new implementation of IDevice::SetPowerProfile.                                                   |
 * |    73.0  | Removes customSamplePatternEnable flag from MsaaStateCreateInfo and renames customSamplePattern to     |
 * |          | quadSamplePattern. Clients need to always provide quadSamplePattern after integrating this change.     |
 * |    72.0  | Adds ICmdBuffer::Reset flag that indicates whether data chunks should be retained or returned.         |
 * |    71.0  | Removed Mantle specific parameter "advancedMsaa" from MsaaStateCreateInfo.                             |
 * |          | Renamed colorTargetSamples to shaderExportMaskSamples.                                                 |
 * |    70.0  | Added parameter PresentInfo::imageIndex used in combination with Swapchain objects to index and signal |
 * |          | fences and semaphores after completion of Presentation events                                          |
 * |    69.0  | Added split barrier support.  This allows a single barrier call to be split into separate client calls |
 * |          | where PAL will perform an "early" and "late" transition.  This can reduce pipeline bubbles when the    |
 * |          | app/client can insert unrelated GPU work between the two phases.                                       |
 * |    68.0  | Added "numSamples" and "samplePatternIdx" fields to the rsState of GraphicsPipelineCreateInfo;         |
 * |          | Added a "samplePatternIdx" field to ImageViewInfo structure;                                           |
 * |          | Added IDevice::SetSamplePatternPalette().                                                              |
 * |    67.0  | Added a TexQuilt enum to the ImageViewType enumeration list as well as added a                         |
 * |          | "quiltWidthInSlices" field to the ImageViewInfo structure.                                             |
 * |    66.0  | Added a "depthAsZ24" flag to the ImageViewInfo structure.                                              |
 * |    65.0  | Added a "filterMode" field to the SamplerInfo structure.  Needed for DX12.                             |
 * |    64.1  | Implements IDevice::SetPowerProfile()                                                                  |
 * |    64.0  | Adding initiallySignaled parameter to IDevice::CreateFence interface.                                  |
 * |    63.1  | Added IDevice::SetPowerProfile().                                                                      |
 * |    63.0  | Adds the "useEmbeddedDataForCeRamDumps" flag to CmdBufferBuildInfo.  If set, command buffers will dump |
 * |          | internal CE RAM tables to embedded data. If clear, command buffers will dump to a ring buffer. Legacy  |
 * |          | behavior is to set this flag to 0.                                                                     |
 * |    62.4  | Added Vector class to Util. Added overloaded Erase function that takes a IntrusiveListNode argument.   |
 * |    62.3  | Makes it clearer that the FormatPropertiesTable is indexed by "is-linear" and "is-non-linear".         |
 * |    62.2  | Added ResourceMappingNodeType::Count.                                                                  |
 * |    62.1  | Removes ErrorInvalidCompressedImageSize which is no longer needed, that check is now in Mantle only    |
 * |    62.0  | Replace numStaticSamplers, pStaticSamplerIds and pStaticSamplerSrds with numDescriptorRangeValues and  |
 * |          | pDescriptorRangeValues in PipelineShaderInfo to match Vulkan's requirement                             |
 * |    61.0  | Add "exposedSamples" field to MsaaStateCreateInfo to allow restricting coverage mask exposed to pixel  |
 * |          | shaders. Must be programmed same as "coverageSamples" to maintain backward compatibility.              |
 * |    60.2  | Add ICmdBuffer::CmdClearBoundColorTargets, and ICmdBuffer::CmdClearBoundDepthStencilTargets functions  |
 * |    60.1  | Convert AllowNonIeeeOperations to a public setting.                                                    |
 * |    60.0  | Renamed the formatChange Image flag to formatChangeSrd and added the formatChangeTgt Image flag.       |
 * |    59.0  | Added support for setting Stencil reference values, masks and stencil op individually or all at once.  |
 * |          | Flags must be set in StencilRefMaskParams to indicate which of the values are being updated.           |
 * |    58.0  | Updates BarrierTransition for latest Vulkan spec and updates documentation to clarify cache bitmasks.  |
 * |    57.2  | Added ICmdBuffer::CmdSetMarker for DX clients and allows clients to specify ICmdAllocator's sizes for  |
 * |          | marker data.                                                                                           |
 * |    57.1  | Added Util::RWLock and Util::RWLockAuto.                                                               |
 * |    57.0  | Modifies the Cmd*CopyImage region structs to allow copying of multiple slices in a single region. Also |
 * |          | modifies CmdResolve image interface to match other blit functions in treatmet of slices.               |
 * |    56.0  | Adds the 'autoMemoryReuse' flag to CmdAllocatorCreateFlags. If this flag is zero, a command allocator  |
 * |          | will not attempt to recycle command memory until ICmdAllocator::Reset() is called. This flag must be   |
 * |          | set to '1' to preserve legacy behavior!                                                                |
 * |    55.0  | Added "unnormalizedCoords" to SamplerInfo to force use of unnormalized texture coordinates for         |
 * |          | sampler SRDs.  This should be set to 0 to maintain previous behavior.                                  |
 * |    54.1  | Wait for queue idle returns an error if device is lost or reset. Added IDevice::CheckExecutionState.   |
 * |    54.0  | Allow clients to specify ICmdAllocator's chunk and suballocation size for embedded data.               |
 * |    53.0  | Added "flippable" flags for GpuMemory and Image objects; if set they can be flipped to the screen.     |
 * |          | Added the "interprocess" flag for GpuMemory objects that must be visible to other processes. Renamed   |
 * |          | the TooManyPresentableImages result to TooManyFlippableAllocations and added it to CreateGpuMemory.    |
 * |          | Added ErrorTooManyPresentableImages for CreatePresentableImage calls with full swap chains.            |
 * |    52.0  | Exposed StencilOp value in PalStencilRefMaskParams for clients. Default behavior == 1                  |
 * |    51.0  | Created StencilRefMaskParams from the previously separate Ref and Mask structures                      |
 * |    50.0  | Remove pScreen and have hWindow/pSwapChain as separate variables outside of union in Pal::PresentInfo. |
 * |          | Return signaled semaphore in ISwapChain::AcquireNextImage.                                             |
 * |    49.0  | Added patchControlPoints to the PrimitiveTopology parameters for DX builds.                            |
 * |    48.0  | Removed ICmdBuffer::CmdBindViewportState and ICmdBuffer::CmdBindScissorState and replaced them with    |
 * |          | ICmdBuffer::CmdSetViewports and ICmdBuffer::CmdSetScissorRects.                                        |
 * |    47.0  | Expose limitations for optimally tiled image to memory (and vice versa) on each engine.                |
 * |    46.1  | Added vsyncGpuTime to PrivateScreenPresentStats.                                                       |
 * |    46.0  | Removed the limitImageAlignmentTo64Kb and imagesNeedSwizzleEquations public settings, replacing them   |
 * |          | with ImageCreateInfo::maxBaseAlign and ImageCreateFlags::needSwizzleEqs. Removed the redundant         |
 * |          | ImageMemoryLayout::tiling. Added Pal::LinearSwizzleEqIndex. Added IDevice::GetLinearImageAlignments,   |
 * |          | ImageCreateInfo::rowPitch and ImageCreateInfo::depthPitch to support client-defined linear pitches.    |
 * |    45.0  | Exposed limitations of optimally tiled copies on each engine.                                          |
 * |    44.0  | Added numStaticSamplers, pStaticSamplerIds and pStaticSamplerSrds in PipelineShaderInfo to support     |
 * |          | static sampler for Vulkan.                                                                             |
 * |    43.1  | Changed the function return type of IPrivateScreen::GetDisplayMode() from void to Result.              |
 * |    43.0  | Added PRT packed mip info and renamed sparseImage variables to PRT for consistency.                    |
 * |    42.0  | Refactored SamplerInfo to include a flags struct and added the preciseAnsio flag.                      |
 * |    41.1  | Added CmdBufferBuildFlags::optimizeExclusiveSubmit.                                                    |
 * |    41.0  | Adds support for opening shared external resources on DX builds. This required changes to the existing |
 * |          | shared external interface functions and the addition of IDevice::DetermineExternalSharedResourceType.  |
 * |    40.5  | Report support for minimum precision shader instructions in Pal::DeviceProperties.                     |
 * |    40.4  | Added emulated private screen support and opening external shared resource from NT handle.             |
 * |    40.3  | Exposed PrivateScreenCreateInfo and added IPrivateScreen::BindOwner().                                 |
 * |    40.2  | Modifies IImage so ImageCreateInfo can be queried without a virtual call, changes GetImageCreateInfo   |
 * |          | to return a const reference instead of a pointer.                                                      |
 * |    40.1  | Added SetExecutionPriority interface to IQueue to set the priority level of queue.                     |
 * |    40.0  | Two new ImageCreateFlags for optimizations: nativeResolve and copyFormatsMatch. The former must be set |
 * |          | to retain previous behavior. The CopySrgbToUnorm flag was removed. ImageLayout arguments were added to |
 * |          | CmdClearImageView and CmdResolveImage.                                                                 |
 * |    39.2  | Added ICmdBuffer::CmdSaveComputeState and ICmdBuffer::CmdRestoreComputeState.                          |
 * |    39.1  | Added Rewind() method to Util::File.                                                                   |
 * |    39.0  | Refine Vulkan WSI SDK0.2.0 interface. Fix a minor bug in Pal::PlatformProperties.                      |
 * |    38.0  | Added ImageResolveRegion::format, which must be set to UndefinedFormat to get the previous behavior.   |
 * |    37.2  | Adds GetConnectorProperties, GetDisplayMode, SetGammaRamp, SetPowerMode, SetDisplayMode and            |
 * |          | SetColorMatrix interfaces to IPrivateScreen.                                                           |
 * |    37.1  | Added ImageCreateFlags::noMetadata, which disables all metadata allocations for the given image.       |
 * |    37.0  | CmdCopyMemoryToImage, CmdCopyImageToMemory, and CmdCopyTypedBuffer now take pitches in units of bytes. |
 * |          | Adds Util::RoundUpQuotient and fixes Util::CompressedTexelsToBlocks for ASTC formats.                  |
 * |    36.2  | Added Util::MkDir().                                                                                   |
 * |    36.1  | Added IShader::UsesPushConstants().                                                                    |
 * |    36.0  | Remove the registerWindowRequired from PlatformProperties.                                             |
 * |    35.3  | Adds PAL interface for Vulkan WSI of SDK0.2.0                                                          |
 * |    35.2  | Added the CopyRawSwizzle copy flag and changed the default copy behavior to not do raw swizzles.       |
 * |    35.1  | Added GetInfo() method to IPipeline and GetPipelineInfo() to ICompoundState.                           |
 * |    35.0  | Added flags for ICmdBuffer::CmdCopyImage and added ICmdBuffer::CmdCopyTypedBuffer.                     |
 * |    34.0  | Dynamic display enumeration: IPlatform::GetScreens enumerates with the OS and returns new screen       |
 * |          | objects in caller-allocated memory.                                                                    |
 * |    33.0  | Adds support for swizzle equations, which specify how to interpret a mapped, optimally-tiled image.    |
 * |    32.0  | Changed device property supportsOcclusionPredication to supportsQueryPredication to cover both         |
 * |          | Occlusion and Streamout query                                                                          |
 * |    31.0  | Adds support for PRT images, including copying of page tile mappings from one virtual GPU memory       |
 * |          | object to another.                                                                                     |
 * |    30.2  | Added IsSectionPresent() method to ElfReadContext.                                                     |
 * |    30.1  | Added ICmdBuffer::CmdClearColorBuffer.                                                                 |
 * |    30.0  | ColorTargetViewCreateInfo has been reworked; it can create color target image views and buffer views.  |
 * |    29.2  | Added IDevice::Cleanup, a function to explicitly clean up a device object for reuse or destruction.    |
 * |    29.1  | Added ICmdBuffer::DispatchOffset().                                                                    |
 * |    29.0  | Added the countGpuAddr parameter to ICmdBuffer::DrawIndirectMulti() and DrawIndexedIndirectMulti(). It |
 * |          | allows clients to issue indirect draws where the number of draws is stored in GPU memory. Passing zero |
 * |          | matches the old behavior.                                                                              |
 * |    28.0  | Replaced IDevice::CreateBufferViewSrds() with two variants: CreateTypedBufferViewSrds() and            |
 * |          | CreateUntypedBufferViewSrds(). This allow clients to avoid branches in some scenarios.                 |
 * |    27.1  | Changes the type of QueryPoolCreateInfo::enabledStats to uint32 and adds QueryPipelineStatsAll.        |
 * |    27.0  | Changed the meaning of ResourceMappingNode::inlineConst::slot when the sm5_1ResourceBinding flag is set|
 * |          | for a pipeline: In DX12, inline constants must be an entire constant buffer, so slot refers to which   |
 * |          | buffer in an indexable range the inline constant maps. The old meaning of which vec4 in the buffer is  |
 * |          | mapped only applies to non- SM5.1 pipelines (i.e., Mantle).                                            |
 * |    26.0  | Replaced PipelineShaderInfo::userDataNodes with pUserDataNodes because the pipeline creation info is   |
 * |          | too large if we support as many user-data entries as DX12 needs.                                       |
 * |    25.0  | Reworks the change made in version 23.0: clients must pass in a ClearColor struct instead of a raw     |
 * |          | clear color. This reduces the number of clear functions without limiting Pal to raw clears. Adds       |
 * |          | ICmdBuffer::CmdClearBufferView and ICmdBuffer::CmdClearImageView to support DX12 UAV clears.           |
 * |    24.0  | Added/Modified Predication interface:CmdSetPredication that covers both enable and disable for both    |
 * |          | memory and QueryPool based predication.                                                                |
 * |    23.0  | Removes ICmdBuffer's non-raw color clear function and renamed its raw color clear function. Clients    |
 * |          | must convert their clear colors to raw clear colors using Formats::ConvertColor.                       |
 * |    22.1  | Added support for clients to change the high watermark for indirect user-data tables while recording a |
 * |          | command buffer.  To maintain old behavior, clients can just never call CmdSetIndirectUserDataWatermark.|
 * |    22.0  | Added support for nested command buffers.  Clients should leave all flags in CmdBufferCreateInfo at 0  |
 * |          | maintain old behavior.                                                                                 |
 * |    21.0  | Modified ICmdBuffer::CmdBindIndexData() to take an address and an index count instead of an IGpuMemory.|
 * |          | Modified IGpuMemory to store a GpuMemoryDesc that can be accessed non-virtually.                       |
 * |    20.1  | Added IImage::GetMemoryLayout to support DX12's CheckResourceAllocationInfo DDI.                       |
 * |    20.0  | Added the ICmdAllocator object. ICmdBuffers must be associated with an ICmdAllocator at creation.      |
 * |          | It is possible to switch to a different ICmdAllocator when calling ICmdBuffer::Reset(). UpdateMemory   |
 * |          | no longer has a size limit and maxEmbeddedDataDwords was replaced by ICmdBuffer::GetEmbeddedDataLimit. |
 * |          | IntrusiveList lost its NumElements() function and gained IsEmpty() and PushBackList().                 |
 * |    19.3  | Added "supportsRegMemAccess" flag to DeviceProperties structure to support RegMemAccess caps.          |
 * |    19.2  | Add stream output support. Clients should ensure soState.numStreamOutEntries in PipelineCreateInfo is  |
 * |          | 0 to maintain old behavior.                                                                            |
 * |    19.1  | Added "vaStart" and "vaEnd" fields to DeviceProperties structure to support DX12 VA range caps.        |
 * |    19.0  | Added "origin" field to ViewportInfo structure to support configurable viewport origin.                |
 * |    18.0  | Finished adding optional validation to the various IDevice::Get*Size functions.                        |
 * |    17.0  | Removes residency from IQueue and adds an optional IQueue pointer to IDevice residency functions.      |
 * |          | Also makes device residency methods work on WDDM1.                                                     |
 * |    16.1  | Added ICmdBuffer::CmdAllocateEmbeddedData() and DeviceProperties::maxEmbeddedDataDwords.               |
 * |    16.0  | Modified BufferViewInfo to take a GPU virtual address instead of a GpuMemory pointer and an offset.    |
 * |    15.0  | Added requireUploadToGpuMem to GetGpuMemoryRequirements, denotes whether UploadToGpuMem is required.   |
 * |    14.1  | Introduced the concept of Windows KMT and DX builds. Added DX-only structure members and functions.    |
 * |    14.0  | Fixed some issues with the previous version. Moved mgpuIqMatch to Sampler SRD creation.                |
 * |    13.0  | Added the ability to perform multi-slice resolves to ICmdBuffer::CmdResolveImage.                      |
 * |    12.0  | Merged IDevice and IPhysicalGpu. IPhysicalGpu has been removed and all functionality moved to IDevice. |
 * |    11.0  | Add trap handler support.  Clients should ensure trapPresent and debugMode flags in PipelineShaderInfo |
 * |          | are 0 to maintain old behavior.                                                                        |
 * |    10.0  | Adds QueueCreateInfo and moves pImage from the internal GpuMemory create info to the interface info.   |
 * |     9.0  | Moves IDevice::GetMaxAtomicCounters() to IPhysicalGpu. Makes GDS allocations part of IPhysicalGpu.     |
 * |     8.1  | Updates the format properties table to report shader atomic capabilities.                              |
 * |     8.0  | Adds parameters to ICmdBuffer::CmdDumpCeRam to allow PAL to instruct the CP to help handle ring        |
 * |          | buffers managed by the constant engine.                                                                |
 * |     7.0  | Moves IGpuMemory creation to IPhysicalGpu as the first part to merging IPhysicalGpu and IDevice.       |
 * |     6.0  | Various interface changes to support Vulkan and DX12 query pool requirements.                          |
 * |     5.0  | Added ICmdBuffer::CmdSetGlobalScissor and updated the compound state object to support it.             |
 * |     4.0  | Adds framebuffer (color and depth targets) to the ICompoundState object.                               |
 * |     3.0  | Add support for spilling user-data entries to memory. PhysicalGpuProperties::maxUserDataEntries is the |
 * |          | same for all shader types now, so the array has been changed to a single uint32.                       |
 * |     2.0  | IQueryPool::GetResults has been reworked to support Vulkan.                                            |
 * |     1.0  | Initial version.  Only enough functionality to support the Mantle driver.                              |
 ***********************************************************************************************************************
 */
#endif // PAL_CLOSED_SOURCE
