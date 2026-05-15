/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palH264.h
 * @brief PAL utility collection H264 namespace declarations.
 ***********************************************************************************************************************
 */

#pragma once

#include "pal.h"
#include "palUtil.h"
#include "palDevice.h"

namespace Util
{

#if PAL_BUILD_VIDEO
class File;

/// Namespace containing functions that provide support for H.264/MPEG4 AVC codec related information.
namespace H264
{

/// Structure representing H.264/MPEG4 AVC codec level constraints.
struct LevelCaps
{
    uint32 maxMacroBlocksPerSec;            ///< Maximum decoding speed in macro blocks per second.
    uint32 maxMacroBlocksPerFrame;          ///< Maximum frame size in terms of macro blocks.
    uint32 maxDpbMacroBlocks;               ///< Maximum size of DPB (decoded picture buffer) in terms of macro blocks.
};

/// Gets macro block size.
extern Pal::Extent2d GetMacroBlockSize();

/// Gets capabilities of H.264/MPEG4 AVC codec level.
///
/// @param [in] level Codec level (encoded as 10 times the value of the fractional codec level value).
///
/// @returns Capabilities of the specified codec level.
extern LevelCaps GetLevelCaps(uint32 level);
} // H264
#endif

} // Util
