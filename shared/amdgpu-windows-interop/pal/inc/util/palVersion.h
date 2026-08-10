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
#define PAL_INTERFACE_MAJOR_VERSION 998

/// Minimum major interface version. This is the minimum interface version PAL supports in order to support backward
/// compatibility. When it is equal to PAL_INTERFACE_MAJOR_VERSION, only the latest interface version is supported.
///
/// @ingroup LibInit
#define PAL_MINIMUM_INTERFACE_MAJOR_VERSION 948

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

// Static asserts to ensure clients define PAL_CLIENT_INTERFACE_MAJOR_VERSION and that it falls in the supported range.
#ifndef PAL_CLIENT_INTERFACE_MAJOR_VERSION
    static_assert(false, "The client must link against 'palUtil' or 'pal' in CMake!");
#else
    static_assert((PAL_CLIENT_INTERFACE_MAJOR_VERSION >= PAL_MINIMUM_INTERFACE_MAJOR_VERSION) &&
                  (PAL_CLIENT_INTERFACE_MAJOR_VERSION <= PAL_INTERFACE_MAJOR_VERSION),
                  "The specified PAL_CLIENT_INTERFACE_MAJOR_VERSION is not supported.");
#endif

///# Version history details are closed source (descriptions may contain internal details)
///# @page VersionHistory
///# %Version History
///# ---------------
///#
///# | %Version | Change Description                                                                                   |
///# | -------- | ---------------------------------------------------------------------------------------------------- |
///# |   998.0  | Move from DoNotWait bool to RemapFlags bitmap for virtual memory (re)mapping operations.             |
///# |   997.0  | Move enum operators from palTypeTraits.h to palOperators.h                                           |
///# |   996.0  | Add new Work Graphs SRL interfaces in support of WGIP3, update OutputAllocate, OutputCommit with     |
///# |          | new signatures passing additional arguments, and extend WorkGraphScheduler version enum.             |
///# |   995.0  | Modify the layout of ApiCompositeDataValue. Shrink the number of bits used for rasterization stream  |
///# |          | from 3 to 2.                                                                                         |
///# |   994.0  | Change VPE shaper LUT interface to use GPU virtual addresses instead of IImage pointers.             |
///# |   993.0  | Change Pipeline SemanticInfo semantic field to uint32.                                               |
///# |   992.0  | Remove cross-process communication buffer for optimal sharing.                                       |
///# |   991.0  | Add new Engine/Queue Type in Brighton and flag for external Command Building.                        |
///# |   990.0  | Deletes the unused palHashLiteralString.h. Remove all includes of this file.                         |
///# |   989.0  | Removes the GfxIpLevel enum and the standalone DeviceProperties gfxStepping field. They are redundant|
///# |          | against the IpTriple gfxTriple field. Clients should replace GfxIpLevel comparisons with constant    |
///# |          | IpLevel structs e.g. `if (props.gfxTriple >= IpLevel(11, 0))` checks for "is gfx11.0 or greater".    |
///# |          | Note that we've also removed the implicit uint32 conversion operators from IpTriple and IpLevel, use |
///# |          | the constexpr "Bits()" function instead to implement explicit uint32 casts in switch statements.     |
///# |   988.0  | Split AsicRevision AlphaTrion2 to A0/B0, and added AtLite3 A0/B0. Removed AsicRevisions              |
///# |          | AlphaTrionX / AlphaTrion1 when building AtLite3                                                      |
///# |   987.0  | Replace NullGpuId with AsicRevision for null device; use IPlatform::CreateNullDevice(AsicRevision).  |
///# |          | Replace createNullDevice flag with useNullBackend. Add NullGpuInfoTable indexed by AsicRevision.     |
///# |          | Replace GfxIpLevel gfxIpLevel with IpTriple gfxTriple in GpuInfo. Move IpTriple and IpLevel from     |
///# |          | palDevice.h to palLib.h since they are now part of the public null-device API.                       |
///# |   986.0  | Add TileOptMode::BlockBased                                                                          |
///# |   985.0  | Add "available_threads_per_wg" field to pipeline metadata.                                           |
///# |   984.0  | Remove/split chipProps.gfxip.max3dDispatchInterleaveProduct to track GFX and ACE separately          |
///# |   983.0  | Add unique export names for mesh and vertex Work Graphs SRL functions. Remove the old overloads.     |
///# |   982.0  | Remove aggregate initialization for IpTriple. Use IpLevel when you wish to express a certain         |
///# |          | group of HW features. IpTriple should only be used when you have a specific stepping value.          |
///# |          | Note that many IpTriple <> IpLevel operators exist, e.g. "if (gfxTriple >= IpLevel(12, 0))".         |
///# |   981.0  | Add IFence::Reset() to be able to be used in cases where only 1 fence needs a reset.                 |
///# |   980.0  | Add ResolveMode::SampleZero to be used for existing depth and stencil resolves. ResolveMode::Average |
///# |          | can now be used for depth that considers all samples similar to the unchanged behavior for color.    |
///# |   979.0  | Remove PAL_BUILD_STRIX_HALO, PAL_BUILD_HAWK_POINT1, PAL_BUILD_HAWK_POINT2, PAL_BUILD_KRACKAN1, and   |
///# |          | PAL_BUILD_KRACKAN2. Clients should use PAL_BUILD_GFX9 instead.                                       |
///# |   978.0  | Remove createInfo from GetGraphicsPipelineSize and GetComputePipelineSize.                           |
///# |   977.0  | Unify Pal::ShaderType with the Pipeline ABI shader stage enum; keep legacy spellings for old clients.|
///# |   976.0  | Removes old Work Graphs SRL interface for mesh & vertex shaders. Also removes support for cross-group|
///# |          | sharing for Mesh nodes, which was never actually part of the WG API.                                 |
///# |   975.0  | Note: This interface change was backed out. Upgrading to this version number changes nothing in PAL. |
///# |   974.0  | Replace IGpuMemory with gpuva for SetHipTrapHandler                                                  |
///# |   973.0  | Remove a bunch of unused ICmdBuffer functions: CmdIf, CmdElse, CmdEndIf, CmdWhile, CmdEndWhile,      |
///# |          | CmdMemoryAtomic, CmdCopyRegisterToMemory, CmdWaitRegisterValue, plus some supporting enums and flags.|
///# |   972.0  | Remove MaxPayloadSize and allow arbitrary length strings and buffers in CmdCommentString and CmdNop. |
///# |          | Also, CmdCommentString uses StringView<char> now so the strings don't need a NULL-terminator!        |
///# |          | Also also, StringView can call Strlen at compile time and it now has DropFront and DropBack          |
///# |   971.0  | Change PipelineCreateFlags::reverseWorkgroupOrder to an enum in ComputePipelineCreateInfo.           |
///# |   970.0  | Add "vgpr_count_max" and "vgpr_count_rts" fields to ELF metadata.                                    |
///# |   969.0  | Add GpuVaHintFlags for ICmdBuffer::CmdCopyMemoryByGpuVa to indicate if memory is TMZ Protected.      |
///# |          | Also add ICmdBuffer::CmdCopyMemoryToImageByGpuVa and ICmdBuffer::CmdCopyImageToMemoryByGpuVa.        |
///# |   968.0  | Deprecate Util::RemoveCvref. Use std::remove_cvref instead.                                          |
///# |   967.0  | Deprecate CoherClear, plase use CoherCopyDst instead.                                                |
///# |   966.0  | Adds a new Work Graphs SRL interface for mesh & vertex shaders which more closely parallels the one  |
///# |          | used for compute nodes. This will eventually replace the existing mesh & vertex shader SRL interface.|
///# |   965.0  | Remove arguments from CmdRestoreComputeState, rename ComputeStateAll, and add ComputeStateTreatAsBlt.|
///# |   964.0  | Remove memAlign from VpeTonemapParams and adds a VpeIpCapabilities sub-struct into VpeIpProperties   |
///# |   963.0  | Deprecate image creation flag fullResolveDstOnly.                                                    |
///# |   962.0  | move frameId from DX PrivatePresentInfo to PresentDirectInfo                                         |
///# |   961.0  | Deprecate EQAA from Image create and ClearRTV/DSV. Fragments and samples no longer can be different. |
///# |          | Also change the max supported image sample value from dynamic to statically specified.               |
///# |   960.0  | Deprecate image creation flag fullCopyDstOnly.                                                       |
///# |   959.0  | Deprecate LateAllocVs from GfxPipeline.                                                              |
///# |   958.0  | Rename Pal::AsicRevision::AlphaTrion1 to Pal::AsicRevision::AlphaTrionX. Remove AlphaTrion1 & Navi52 |
///# |   957.0  | Deprecate Image repetitiveResolve flag.                                                              |
///# |   956.0  | Deprecate some public settings related to GFX10                                                      |
///# |   955.0  | Rename CopyControlFlags to CopyImageControlFlags; add copy control flag CopyMemroyToImageControlFlags|
///# |          | for CmdCopyMemoryToImage(); allow clear and copy to initialize metadata via newly defined flags.     |
///# |   954.0  | Add a DispatchAqlFeedback* argument to CmdDispatchAql to return information about the dispatch.      |
///# |   953.0  | Change interface of GetFullSubresourceRange() to make it easier to call.                             |
///# |   952.0  | Remove imageVaLocked from PAL's color targets and depth/stencil targets. We have long required that  |
///# |          | imageVaLocked = 1 so no existing clients should be rebinding image memory and using old view objects.|
///# |   951.0  | Add max thread group count fields for mesh shaders.                                                  |
///# |   950.0  | Remove DepthClampMode::ZeroToOne.                                                                    |
///# |   949.0  | Adds compression option to ITraceSource                                                              |
///# |   948.0  | Removes unsupported XDMA, HW Composition, and MGPU SLS interfaces.                                   |
