/*
************************************************************************************************************************
*
*  Copyright (C) 2024-2025 Advanced Micro Devices, Inc.  All rights reserved.
*  SPDX-License-Identifier: MIT
*
***********************************************************************************************************************/

/**
************************************************************************************************************************
* @file  gfx13addrlib.cpp
* @brief Contain the implementation for the Gfx13Lib class.
************************************************************************************************************************
*/

#include "amdgpu_asic_addr.h"

#include "gfx13addrlib.h"
#include "addrswizzler.h"
#include "gfx13AT1MetaSwizzlePattern.h"
#include "gfx13AT_LITE3MetaSwizzlePattern.h"
#include "gfx13AT2MetaSwizzlePattern.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace rocr {
namespace Addr
{
/**
************************************************************************************************************************
*   Gfx13HwlInit
*
*   @brief
*       Creates an Gfx1Lib object.
*
*   @return
*       Returns an Gfx1Lib object pointer.
************************************************************************************************************************
*/
Addr::Lib* Gfx13HwlInit(
    const Client* pClient)
{
    return V3::Gfx13Lib::CreateObj(pClient);
}

namespace V3
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//                               Static Const Member
////////////////////////////////////////////////////////////////////////////////////////////////////
const SwizzleModeFlags Gfx13Lib::SwizzleModeTable[ADDR3_MAX_TYPE] =
{//Linear 2d   3d  256B  4KB  64KB  256KB  Reserved
    {{1,   0,   0,    0,   0,    0,     0,    0}}, // ADDR3_LINEAR
    {{0,   1,   0,    1,   0,    0,     0,    0}}, // ADDR3_256B_2D
    {{0,   1,   0,    0,   1,    0,     0,    0}}, // ADDR3_4KB_2D
    {{0,   1,   0,    0,   0,    1,     0,    0}}, // ADDR3_64KB_2D
    {{0,   1,   0,    0,   0,    0,     1,    0}}, // ADDR3_256KB_2D
    {{0,   0,   1,    0,   1,    0,     0,    0}}, // ADDR3_4KB_3D
    {{0,   0,   1,    0,   0,    1,     0,    0}}, // ADDR3_64KB_3D
    {{0,   0,   1,    0,   0,    0,     1,    0}}, // ADDR3_256KB_3D
    {{0,   1,   0,    0,   0,    1,     0,    0}}, // ADDR3_64KB_2D_Z
    {{0,   1,   0,    0,   0,    0,     1,    0}}, // ADDR3_256KB_2D_Z
};

/**
************************************************************************************************************************
*   Gfx13Lib::Gfx1Lib
*
*   @brief
*       Constructor
*
************************************************************************************************************************
*/
Gfx13Lib::Gfx13Lib(
    const Client* pClient)
    :
    Lib(pClient),
    m_bitEight2dXor(0)
{
    memcpy(m_swizzleModeTable, SwizzleModeTable, sizeof(SwizzleModeTable));
}

/**
************************************************************************************************************************
*   Gfx13Lib::~Gfx1Lib
*
*   @brief
*       Destructor
************************************************************************************************************************
*/
Gfx13Lib::~Gfx13Lib()
{
}

/**
************************************************************************************************************************
*   Gfx13Lib::HasBit8Xor
*
*   @brief
*       Returns true if a bit8 xor should be applied to an image in this swizzle mode.
*
************************************************************************************************************************
*/
BOOL_32 Gfx13Lib::HasBit8Xor(
    Addr3SwizzleMode  swMode
    ) const
{
    // If this is a 4kB block or larger (i.e., bits 8 and 11 exist...) and the BIT8_2D_XOR bit is set, then bit8
    // of the equation also includes bit 11.  Register spec specifically states that this bit only applies to 2D
    // swizzle modes as well.
    return ((m_bitEight2dXor != 0) &&
            Is2dSwizzle(swMode)    &&
            (IsBlock256b(swMode) == FALSE));
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetNumSupportedMsaaRates
*
*   @brief
*       Returns the number of supported MSAA rates for the specified swizzle mode.
*
************************************************************************************************************************
*/
UINT_32 Gfx13Lib::GetNumSupportedMsaaRates(
    Addr3SwizzleMode  swMode
    ) const
{
    // MSAA is only supported for 64kB and 256kB 2D swizzle modes.
    const UINT_32  numMsaaRates = (Is2dSwizzle(swMode) && (GetBlockSizeLog2(swMode) >= 16)) ? MaxNumMsaaRates : 1;

    return numMsaaRates;
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetBlockSizeIndex
*
*   @brief
*       Returns a 0..N index representing the block sizes in their size order.  .  Linear swizzles
*       are not supported.
*
*   @return
*       256B is zero, 4KB is 1, 64KB is 2, 256KB is 3.
*
************************************************************************************************************************
*/
UINT_32 Gfx13Lib::GetBlockSizeIndex(
    Addr3SwizzleMode  swMode    ///< [in] swizzle mode
    ) const
{
    // Table to convert Log2 of block sizes (256B, 4kB, 64KB, 256KB) into an 0..3 index.
    static constexpr  UINT_32  BlockSizeLog2[] = { 8, 12, 16, 18 };

    const UINT_32  blockSizeLog2 = GetBlockSizeLog2(swMode);

    BOOL_32  found = FALSE;
    UINT_32  idx = 0;
    while ((idx < (sizeof(BlockSizeLog2) / sizeof(UINT_32))) && (found == FALSE))
    {
        if (BlockSizeLog2[idx] == blockSizeLog2)
        {
            found = TRUE;
        }
        else
        {
            idx++;
        }
    }

    return idx;
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetSwizzleTypeIndex
*
*   @brief
*       Returns a 0..N index representing the swizzle mode in order (i.e., 0 = 2D, 1 = 2D_Z, 2 = 3D)
*
************************************************************************************************************************
*/
UINT_32 Gfx13Lib::GetSwizzleTypeIndex(
    Addr3SwizzleMode  swMode    ///< [in] swizzle mode
    ) const
{
    UINT_32  idx = 0;

    if (Is2dSwizzle(swMode))
    {
        switch (swMode)
        {
        case Addr3SwizzleMode::ADDR3_64KB_2D_Z:
        case Addr3SwizzleMode::ADDR3_256KB_2D_Z:
            idx = 1;
            break;
        default:
            break;
        }
    }
    else if (Is3dSwizzle(swMode))
    {
        idx = 2;
    }

    return idx;
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetEquation
*
*   @brief
*       Returns a pointer to the equation matching the supplied parameters.
*
*   @return
*       Returns a pointer to the equation that matches the supplied parameters.
************************************************************************************************************************
*/
const ADDR3_EQUATION* Gfx13Lib::GetEquation(
    UINT_32                elemLog2,  ///< [in] element bytes log2
    Addr3SwizzleMode       swMode,    ///< [in] swizzle mode
    UINT_32                msaaIdx,   ///< [in] Log2 of number of samples
    BOOL_32                isMeta)
    const
{
    const ADDR3_EQUATION*  pEquation      = nullptr;
    const UINT_32          blockSizeIdx   = GetBlockSizeIndex(swMode);
    const UINT_32          swizzleTypeIdx = GetSwizzleTypeIndex(swMode);

    if (isMeta)
    {
        const ADDR3_EQUATION*** pppLevel1 = nullptr;

        if (ASICREV_IS_AT1(m_chipRevision))
        {
            pppLevel1 = GFX13_SW_AT1;
        }
        else if (ASICREV_IS_GFX1300(m_chipRevision))
        {
            pppLevel1 = GFX13_SW_AT2;
        }
        else if (ASICREV_IS_AT_LITE3(m_chipRevision))
        {
            pppLevel1 = GFX13_SW_AT_LITE3;
        }

        if (pppLevel1 != nullptr)
        {
            // Meta equations aren't affected by elemLog2 or MSAA, so there's fewer levels
            // of indirection.
            const ADDR3_EQUATION** ppLevel2 = pppLevel1[blockSizeIdx];

            pEquation = ppLevel2[swizzleTypeIdx];
        }
    }
    else
    {
        const ADDR3_EQUATION****  ppppLevel1 = GFX13_SW_IMAGE[blockSizeIdx];
        const ADDR3_EQUATION***   pppLevel2  = ppppLevel1[swizzleTypeIdx];
        const ADDR3_EQUATION**    ppLevel3   = pppLevel2[msaaIdx];

        pEquation = ppLevel3[elemLog2];
    }

    return pEquation;
}

/**
************************************************************************************************************************
*   Gfx13Lib::ConvertToLegacyEquation
*
*   @brief
*       Converts an ADDR3 equation into its legacy form.
*
*   @return
*       N/A
************************************************************************************************************************
*/
VOID Gfx13Lib::ConvertToLegacyEquation(
    const ADDR3_EQUATION*  pEquation,
    ADDR_EQUATION*         pLegacyEquation
    ) const
{
    memset(pLegacyEquation, 0, sizeof(ADDR_EQUATION));

    pLegacyEquation->numBitComponents = 0;
    for (UINT_32  bitIdx = 0; bitIdx < pEquation->numValidBits; bitIdx++)
    {
        const auto*  pBit = &pEquation->bits[bitIdx];

        pLegacyEquation->numBitComponents = Max(pBit->numTerms, pLegacyEquation->numBitComponents);

        for (UINT_32  termIdx = 0; termIdx < pBit->numTerms; termIdx++)
        {
            const auto* pTerm = &pBit->term[termIdx];

            // Legacy equations can only support XOR terms.  The last term in any bit will have
            // an operator of "none".
            ADDR_ASSERT((pTerm->fields.op == ADDR3_EQ_OPERATOR::_NONE) ||
                        (pTerm->fields.op == ADDR3_EQ_OPERATOR::_XOR));

            pLegacyEquation->comps[termIdx][bitIdx].channel = pTerm->fields.channel;
            pLegacyEquation->comps[termIdx][bitIdx].index   = pTerm->fields.ordinal;
            pLegacyEquation->comps[termIdx][bitIdx].valid   = 1;
        }
    }
}

/**
************************************************************************************************************************
*   Gfx13Lib::InitEquationTable
*
*   @brief
*       Initialize Equation table.
*
*   @return
*       N/A
************************************************************************************************************************
*/
VOID Gfx13Lib::InitEquationTable()
{
    memset(m_pEquationTable, 0, sizeof(m_pEquationTable));

    for (UINT_32 swModeIdx = 0; swModeIdx < ADDR3_MAX_TYPE; swModeIdx++)
    {
        const Addr3SwizzleMode swMode = static_cast<Addr3SwizzleMode>(swModeIdx);

        // Skip linear equation (data table is not useful for 2D/3D images-- only contains x-coordinate bits)
        if (IsValidSwMode(swMode) && (IsLinear(swMode) == false))
        {
            const UINT_32 maxMsaa = GetNumSupportedMsaaRates(swMode);

            for (UINT_32 msaaIdx = 0; msaaIdx < maxMsaa; msaaIdx++)
            {
                for (UINT_32 elemLog2 = 0; elemLog2 < MaxElementBytesLog2; elemLog2++)
                {
                    UINT_32                equationIndex = ADDR_INVALID_EQUATION_INDEX;
                    const ADDR3_EQUATION*  pEquation     = GetEquation(elemLog2, swMode, msaaIdx, FALSE);

                    if (pEquation != NULL)
                    {
                        // Still need a legacy equation for reporting to PAL.
                        ConvertToLegacyEquation(pEquation, &m_legacyEquationTable[m_numEquations]);

                        equationIndex = m_numEquations;
                        ADDR_ASSERT(equationIndex < Gfx13NumImageEquations);

                        m_pEquationTable[equationIndex] = pEquation;
                        m_numEquations++;
                    }

                    SetEquationTableEntry(swMode, msaaIdx, elemLog2, equationIndex);
                } // end loop through bpp sizes
            } // end loop through MSAA rates
        } // end check for valid modes
    } // end loop through swizzle modes
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlGetEquationIndex
*
*   @brief
*       Return equationIndex by surface info input
*
*   @return
*       A valid equationIndex for non-linear swizzle mode, or ADDR_INVALID_EQUATION_INDEX for linear swizzle mode
************************************************************************************************************************
*/
UINT_32 Gfx13Lib::HwlGetEquationIndex(
    const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn    ///< [in] input structure
    ) const
{
    return GetEquationTableEntry(pIn->swizzleMode, Log2(pIn->numSamples), Log2(pIn->bpp >> 3));
}

/**
************************************************************************************************************************
*   Gfx13Lib::InitBlockDimensionTable
*
*   @brief
*       Initialize block dimension table for all swizzle modes + msaa samples + bpp bundles.
*
*   @return
*       N/A
************************************************************************************************************************
*/
VOID Gfx13Lib::InitBlockDimensionTable()
{
    memset(m_blockDimensionTable, 0, sizeof(m_blockDimensionTable));

    ADDR3_COMPUTE_SURFACE_INFO_INPUT surfaceInfo {};

    addr_params params = {};

    for (UINT_32 swModeIdx = 0; swModeIdx < ADDR3_MAX_TYPE; swModeIdx++)
    {
        const Addr3SwizzleMode swMode = static_cast<Addr3SwizzleMode>(swModeIdx);

        if (IsValidSwMode(swMode))
        {
            surfaceInfo.swizzleMode = swMode;
            const UINT_32 maxMsaa   = GetNumSupportedMsaaRates(swMode);

            for (UINT_32 msaaIdx = 0; msaaIdx < maxMsaa; msaaIdx++)
            {
                surfaceInfo.numSamples = (1u << msaaIdx);
                for (UINT_32 elementBytesLog2 = 0; elementBytesLog2 < MaxElementBytesLog2; elementBytesLog2++)
                {
                    surfaceInfo.bpp = (1u << (elementBytesLog2 + 3));

                    ConvertSurfInfoToAddrParams(&surfaceInfo, &params, FALSE);
                    ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT input{ &surfaceInfo, &params };

                    ComputeBlockDimensionForSurf(&input, &m_blockDimensionTable[swModeIdx][msaaIdx][elementBytesLog2]);
                } // end loop through bpp sizes
            } // end loop through MSAA rates
        } // end check for a valid swizzle mode
    } // end loop through swizzle modes
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetMipOrigin
*
*   @brief
*       Internal function to calculate origins of the mip levels
*
*   @return
*       None
************************************************************************************************************************
*/
VOID Gfx13Lib::GetMipOrigin(
     const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,        ///< [in] input structure
     const ADDR_EXTENT3D&                           mipExtentFirstInTail,
     ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*             pOut        ///< [out] output structure
     ) const
{
    const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pSurfInfo = pIn->pSurfInfo;
    const BOOL_32        is3d             = (pSurfInfo->resourceType == ADDR_RSRC_TEX_3D);
    const UINT_32        bytesPerPixel    = pSurfInfo->bpp >> 3;
    const UINT_32        elementBytesLog2 = Log2(bytesPerPixel);
    const UINT_32        samplesLog2      = Log2(pSurfInfo->numSamples);

    // Calculate the width/height/depth for the given microblock, because the mip offset calculation
    // is in units of microblocks but we want it in elements.
    ADDR_EXTENT3D        microBlockExtent = HwlGetMicroBlockSize(pIn->pvAddrParams);
    const ADDR_EXTENT3D  tailMaxDim       = GetMipTailDim(pIn, pOut->blockExtent);
    const UINT_32        blockSizeLog2    = GetBlockSizeLog2(pSurfInfo->swizzleMode);

    UINT_32 pitch  = tailMaxDim.width;
    UINT_32 height = tailMaxDim.height;
    UINT_32 depth  = (is3d ? PowTwoAlign(mipExtentFirstInTail.depth, microBlockExtent.depth) : 1);

    const UINT_32 tailMaxDepth   = (is3d ? (depth / microBlockExtent.depth) : 1);

    for (UINT_32 i = pOut->firstMipIdInTail; i < pSurfInfo->numMipLevels; i++)
    {
        const INT_32  mipInTail = HwlCalcMipInTail(pIn, pOut, i);
        const UINT_32 mipOffset = HwlCalcMipOffset(pIn, mipInTail);
        ADDR3_COORD  coord  = {};

        pOut->pMipInfo[i].offset           = mipOffset * tailMaxDepth;
        pOut->pMipInfo[i].mipTailOffset    = mipOffset;
        pOut->pMipInfo[i].macroBlockOffset = 0;

        HwlGetMipOrigin(pIn->pvAddrParams, mipInTail, &coord);

        pOut->pMipInfo[i].mipTailCoordX = static_cast<UINT_32>(coord.x);
        pOut->pMipInfo[i].mipTailCoordY = static_cast<UINT_32>(coord.y);
        pOut->pMipInfo[i].mipTailCoordZ = static_cast<UINT_32>(coord.z);

        if (IsLinear(pSurfInfo->swizzleMode))
        {
            pitch = Max(pitch >> 1, 1u);
        }
        else
        {
            pOut->pMipInfo[i].pitch  = PowTwoAlign(pitch,  microBlockExtent.width);
            pOut->pMipInfo[i].height = PowTwoAlign(height, microBlockExtent.height);
            pOut->pMipInfo[i].depth  = PowTwoAlign(depth,  microBlockExtent.depth);
            pitch  = Max(pitch >> 1,  1u);
            height = Max(height >> 1, 1u);
            depth  = Max(depth >> 1,  1u);
        }
    }
}

/**
************************************************************************************************************************
*   Gfx13Lib::ConvertSurfInfoToAddrParams
*
*   @brief
*       Combines various SWAL image properties from the ADDR3_COMPUTE_SURFACE_INFO_INPUT struct
*       into the HWAL addr_params struct.
*       A fullUpdate will overwrite all member variables of the addr_params struct.
*
*   @return
*       None
************************************************************************************************************************
*/
VOID Gfx13Lib::ConvertSurfInfoToAddrParams(
    const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn,
    addr_params*                            pOut,
    BOOL_32                                 fullUpdate
    ) const
{
    if (fullUpdate)
    {
        pOut->Set_Width(pIn->width);
        pOut->Set_Height(pIn->height);
        pOut->mip_chain.Init(*pOut);
        pOut->maxmip = GetMaxMipNumber(pIn->numMipLevels);
    }

    if (IsLinear(pIn->swizzleMode))
    {
        pOut->sw = SW_L;
    }
    else if (Is2dSwizzle(pIn->swizzleMode))
    {
        if (IsZSwizzle(pIn->swizzleMode))
        {
            pOut->sw = SW_Z_2D;
        }
        else
        {
            pOut->sw = SW_D_2D;
        }
    }
    else if (Is3dSwizzle(pIn->swizzleMode))
    {
        pOut->sw = SW_S_3D;
    }

    pOut->bpp_log2              = Log2(pIn->bpp >> 3);
    pOut->num_samples_log2      = Log2(pIn->numSamples);
    pOut->slice_block_size_log2 = GetBlockSizeLog2(pIn->swizzleMode);
    pOut->pitch_block_size_log2 = GetBlockSizeLog2(pIn->swizzleMode, TRUE);
    pOut->bit8_2d_xor           = m_bitEight2dXor;
    pOut->chip_engine           = ADDR_ASIC_ID_GFX_ENGINE_GFX13;
    pOut->num_pipes_log2        = NumPipesLog2;
    pOut->pipe_interleave_log2  = PipeInterleaveLog2;
}

/**
************************************************************************************************************************
*   Gfx13Lib::ConvertHtileInfoToAddrParams
*
*   @brief
*       Populates a HWAL addr_params struct based on some HTile properties, the chip config and the swizzle mode
*
*   @return
*       None
************************************************************************************************************************
*/
VOID Gfx13Lib::ConvertHtileInfoToAddrParams(
    const ADDR3_COMPUTE_HTILE_INFO_INPUT* pIn,
    addr_params*                          pOut
    ) const
{

    pOut->num_pipes_log2        = NumPipesLog2;
    pOut->pipe_interleave_log2  = PipeInterleaveLog2;

    pOut->sw                    = SW_Z_2D; // Only Z modes are allowed for D/S surfaces
    pOut->pitch_block_size_log2 = GetBlockSizeLog2(pIn->swizzleMode, TRUE);
    pOut->slice_block_size_log2 = GetBlockSizeLog2(pIn->swizzleMode, FALSE);
    pOut->pipe_aligned          = TRUE;
    pOut->surf_type             = SURF_DEPTH;
    pOut->pipe_dist             = PIPE_DIST_16X16; // True for RB+ parts
    pOut->num_se                = GetNumSe(m_chipRevision);
    pOut->bit8_2d_xor           = m_bitEight2dXor;
    pOut->maxmip                = GetMaxMipNumber(pIn->numMipLevels);
    pOut->chip_engine           = ADDR_ASIC_ID_GFX_ENGINE_GFX13;
    pOut->bpp_log2              = 2; // hTile is always 32bpp

    pOut->Set_Width(pIn->unalignedDims.width);
    pOut->Set_Height(pIn->unalignedDims.height);
    pOut->mip_chain.Init(*pOut);
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetNumSe
*
*   @brief
*       Get se number per ASIC version.
*
*   @return
*       Number of shader engines.
************************************************************************************************************************
*/

UINT_32 Gfx13Lib::GetNumSe(
    UINT_32 chipRevision
    ) const
{
    UINT_32 numSe = 0;

    if (ASICREV_IS_AT1(chipRevision))
    {
        numSe = 6; // AT1
    }
    else if (ASICREV_IS_GFX1300(chipRevision))
    {
        numSe = 4; // AT2
    }
    else if (ASICREV_IS_AT_LITE3(chipRevision))
    {
        numSe = 4; // Magnus
    }
    else
    {
        ADDR_ASSERT_ALWAYS();
    }

    return numSe;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlGetMipOrigin
*
*   @brief
*       Determines the (X,Y,Z) coordinates within a swizzle block as to where a given mip level can be found.
*
*   @return
*       None.
************************************************************************************************************************
*/
VOID Gfx13Lib::HwlGetMipOrigin(
    void*        pvAddrParams,
    UINT_32      mipInTail,
    ADDR3_COORD* pCoord
    ) const
{
    addr_params* pAddrParams = reinterpret_cast<addr_params*>(pvAddrParams);

    ADDR_ASSERT(pAddrParams != NULL);
    getMipOrigin(*pAddrParams, mipInTail, &pCoord->x, &pCoord->y, &pCoord->z);
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlGetMicroBlockSize
*
*   @brief
*       Determines the dimensions of a 256B microblock
*
*   @return
*       Returns the pixel dimensions of a micro block.
************************************************************************************************************************
*/
ADDR_EXTENT3D Gfx13Lib::HwlGetMicroBlockSize(
    void*        pvAddrParams
    ) const
{
    addr_params* pAddrParams = reinterpret_cast<addr_params*>(pvAddrParams);
    ADDR_ASSERT(pAddrParams != NULL);
    int32 widthLog2 = 0;
    int32 heightLog2 = 0;
    int32 depthLog2 = 0;

    getMicroBlockSize(*pAddrParams, &widthLog2, &heightLog2, &depthLog2);
    return {1u << widthLog2, 1u << heightLog2, 1u << depthLog2};
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlGetXyzBlockIndices
*
*   @brief
*       Determines the number of swizzle blocks that precede the coordinates specified via "pIn".
*
*   @return
*       None.
************************************************************************************************************************
*/
VOID Gfx13Lib::HwlGetXyzBlockIndices(
    const ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
    addr_params*                                     pAddrParams,
    const ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*         pComputeSurfOut,
    INT_32                                           mipInTail,
    UINT_32*                                         pYxMacroBlockIndex,
    UINT_32*                                         pZmacroBlockIndex
    ) const
{
    const UINT_32  bytesPerPixel = pIn->bpp >> 3;

    int64  yx_macro_block_index;
    int64  z_macro_block_index;

    ADDR_ASSERT(pAddrParams != NULL);

    getXYZblockIndexes(
        *pAddrParams,
        pIn->x, pIn->y, pIn->slice,
        mipInTail,
        pComputeSurfOut->pMipInfo[pIn->mipId].pitch,
        pComputeSurfOut->sliceSize / bytesPerPixel,
        &z_macro_block_index,
        &yx_macro_block_index);

    *pYxMacroBlockIndex = static_cast<UINT_32>(yx_macro_block_index);
    *pZmacroBlockIndex  = static_cast<UINT_32>(z_macro_block_index);
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlGetXyzOffsets
*
*   @brief
*       Determines the (x,y,z) coordinates within a swizzle block that correspond to the coordinates specified in pIn.
*
*   @return
*       None.
************************************************************************************************************************
*/
VOID Gfx13Lib::HwlGetXyzOffsets(
    const ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
    addr_params*                                     pAddrParams,
    INT_32                                           mipInTail,
    ADDR3_COORD*                                     pCoord
    ) const
{
    ADDR3_COORD  mipOrig;

    ADDR_ASSERT(pAddrParams != NULL);

    getXYZoffsets(*pAddrParams,
                  pIn->x, pIn->y, pIn->slice,
                  mipInTail,
                  // Outputs
                  &pCoord->x, &pCoord->y, &pCoord->z,
                  &mipOrig.x, &mipOrig.y, &mipOrig.z);
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetMipOffset
*
*   @brief
*       Populates the mipInfo structure of the supplied pOut structure.
*
*   @return
*       None.
************************************************************************************************************************
*/
VOID Gfx13Lib::GetMipOffset(
     const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,    ///< [in] input structure
     ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*             pOut    ///< [out] output structure
     ) const
{
    const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pSurfInfo = pIn->pSurfInfo;
    const UINT_32        bytesPerPixel    = pSurfInfo->bpp >> 3;
    const UINT_32        elementBytesLog2 = Log2(bytesPerPixel);
    const UINT_32        blockSizeLog2    = GetBlockSizeLog2(pSurfInfo->swizzleMode);
    const UINT_32        blockSize        = 1 << blockSizeLog2;
    const ADDR_EXTENT3D  tailMaxDim       = GetMipTailDim(pIn, pOut->blockExtent);;
    const ADDR_EXTENT3D  mip0Dims         = GetBaseMipExtents(pSurfInfo);
    const UINT_32        maxMipsInTail    = getNumMipsInTail(*reinterpret_cast<addr_params*>(pIn->pvAddrParams));
    const bool           isLinear         = IsLinear(pSurfInfo->swizzleMode);

    UINT_32 firstMipInTail    = pSurfInfo->numMipLevels;
    UINT_64 mipChainSliceSize = 0;
    UINT_64 mipChainSliceSizeDense = 0;
    UINT_64 mipSize[MaxMipLevels];
    UINT_64 mipSliceSize[MaxMipLevels];

    const BOOL_32 useCustomPitch    = UseCustomPitch(pSurfInfo);
    for (UINT_32 mipIdx = 0; mipIdx < pSurfInfo->numMipLevels; mipIdx++)
    {
        const ADDR_EXTENT3D  mipExtents = GetMipExtent(mip0Dims, mipIdx);

        if (Lib::SupportsMipTail(pSurfInfo->swizzleMode) &&
            // The hardware treats a single mip as being outside the miptail, so we should too
            (pSurfInfo->numMipLevels > 1)                &&
            IsInMipTail(tailMaxDim, mipExtents, maxMipsInTail, pSurfInfo->numMipLevels - mipIdx))
        {
            firstMipInTail          = mipIdx;
            mipChainSliceSize      += blockSize / pOut->blockExtent.depth;
            mipChainSliceSizeDense += blockSize / pOut->blockExtent.depth;
            break;
        }
        else
        {
            UINT_32 pitchImgData   = 0u;
            UINT_32 pitchSliceSize = 0u;
            if (isLinear)
            {
                // The slice size of a linear image is calculated as if the "pitch" is 256 byte aligned.
                // However, the rendering pitch is aligned to 128 bytes, and that is what needs to be reported
                // to our clients in the normal 'pitch' field.
                // Note this is NOT the same as the total size of the image being aligned to 256 bytes!
                pitchImgData   = (useCustomPitch ? pOut->pitch : PowTwoAlign(mipExtents.width, 128u / bytesPerPixel));
                pitchSliceSize = PowTwoAlign(pitchImgData, blockSize / bytesPerPixel);
            }
            else
            {
                pitchImgData   = PowTwoAlign(mipExtents.width, pOut->blockExtent.width);
                pitchSliceSize = pitchImgData;
            }

            UINT_32 height = UseCustomHeight(pSurfInfo)
                                        ? pOut->height
                                        : PowTwoAlign(mipExtents.height, pOut->blockExtent.height);
            const UINT_32 depth  = PowTwoAlign(mipExtents.depth, pOut->blockExtent.depth);

            if (isLinear && pSurfInfo->flags.denseSliceExact && ((pitchImgData % blockSize) != 0))
            {
                // If we want size to exactly equal (data)pitch * height, make sure that value is 256B aligned.
                // Essentially, if the pitch is less aligned, ensure the height is padded so total alignment is 256B.
                ADDR_ASSERT((blockSize % 128) == 0);
                height = PowTwoAlign(height, blockSize / 128u);
            }

            // The original "blockExtent" calculation does subtraction of logs (i.e., division) to get the
            // sizes.  We aligned our pitch and height to those sizes, which means we need to multiply the various
            // factors back together to get back to the slice size.
            UINT_64 sizeExceptPitch = static_cast<UINT_64>(height) * pSurfInfo->numSamples * (pSurfInfo->bpp >> 3);
            UINT_64 sliceSize       = static_cast<UINT_64>(pitchSliceSize) * sizeExceptPitch;
            UINT_64 sliceDataSize   = PowTwoAlign(static_cast<UINT_64>(pitchImgData) * sizeExceptPitch,
                                                  static_cast<UINT_64>(blockSize));

            if ((mipIdx == 0) && CanTrimLinearPadding(pSurfInfo))
            {
                // When this is the last linear subresource of the whole image (as laid out in memory), then we don't
                // need to worry about the real slice size and can reduce it to the end of the image data (or some
                // inflated value to meet a custom depth pitch)
                pitchSliceSize = pitchImgData;
                if (UseCustomHeight(pSurfInfo))
                {
                    sliceSize = pSurfInfo->sliceAlign;
                }
                else
                {
                    sliceSize = sliceDataSize;
                }
            }

            mipSize[mipIdx]         = sliceSize * depth;
            mipSliceSize[mipIdx]    = sliceSize * pOut->blockExtent.depth;
            mipChainSliceSize      += sliceSize;
            mipChainSliceSizeDense += (mipIdx == 0) ? sliceDataSize : sliceSize;

            if (pOut->pMipInfo != NULL)
            {
                pOut->pMipInfo[mipIdx].pitch         = pitchImgData;
                pOut->pMipInfo[mipIdx].pitchForSlice = pitchSliceSize;
                pOut->pMipInfo[mipIdx].height        = height;
                pOut->pMipInfo[mipIdx].depth         = depth;
            }
        }
    }

    pOut->sliceSize            = mipChainSliceSize;
    pOut->sliceSizeDensePacked = mipChainSliceSizeDense;

    pOut->surfSize         = mipChainSliceSize * pOut->numSlices;
    pOut->mipChainInTail   = (firstMipInTail == 0) ? TRUE : FALSE;
    pOut->firstMipIdInTail = firstMipInTail;

    if (pOut->pMipInfo != NULL)
    {
        if (isLinear)
        {
            // 1. Linear swizzle mode doesn't have miptails.
            // 2. The organization of linear 3D mipmap resource is same as GFX11, we should use mip slice size to
            // caculate mip offset.
            ADDR_ASSERT(firstMipInTail == pSurfInfo->numMipLevels);

            UINT_64 sliceSize = 0;

            for (INT_32 i = static_cast<INT_32>(pSurfInfo->numMipLevels) - 1; i >= 0; i--)
            {
                pOut->pMipInfo[i].offset           = sliceSize;
                pOut->pMipInfo[i].macroBlockOffset = sliceSize;
                pOut->pMipInfo[i].mipTailOffset    = 0;

                sliceSize += mipSliceSize[i];
            }
        }
        else
        {
            UINT_64 offset         = 0;
            UINT_64 macroBlkOffset = 0;

            // Even though "firstMipInTail" is zero-based while "numMipLevels" is one-based, from definition of
            // _ADDR3_COMPUTE_SURFACE_INFO_OUTPUT struct,
            // UINT_32             firstMipIdInTail;     ///< The id of first mip in tail, if there is no mip
            //                                           ///  in tail, it will be set to number of mip levels
            // See initialization:
            //              UINT_32       firstMipInTail    = pIn->numMipLevels
            // It is possible that they are equal if
            //      1. a single mip level image that's larger than the largest mip that would fit in the mip tail if
            //         the mip tail existed
            //      2. 256B_2D and linear images which don't have miptails from HWAL functionality
            //
            // We can use firstMipInTail != pIn->numMipLevels to check it has mip in tails and do mipInfo assignment.
            if (firstMipInTail != pSurfInfo->numMipLevels)
            {
                // Determine the application dimensions of the first mip level that resides in the tail.
                // This is distinct from "tailMaxDim" which is the maximum size of a mip level that will fit in the
                // tail.
                ADDR_EXTENT3D mipExtentFirstInTail = GetMipExtent(mip0Dims, firstMipInTail);

                // For a 2D image, "alignedDepth" is always "1".
                // For a 3D image, this is effectively the number of application slices associated with the first mip
                //                 in the tail (up-aligned to HW requirements).
                const UINT_32 alignedDepth = PowTwoAlign(mipExtentFirstInTail.depth, pOut->blockExtent.depth);

                // "hwSlices" is the number of HW blocks required to represent the first mip level in the tail.
                const UINT_32 hwSlices = alignedDepth / pOut->blockExtent.depth;

                // Note that for 3D images that utilize a 2D swizzle mode, there really can be multiple
                // HW slices that encompass the mip tail; i.e., hwSlices is not necessarily one.
                // For example, you could have a single mip level 8x8x32 image with a 4KB_2D swizzle mode
                // The 8x8 region fits into a 4KB block (so it's "in the tail"), but because we have a 2D
                // swizzle mode (where each slice is its own block, so blockExtent.depth == 1), hwSlices
                // will now be equivalent to the number of application slices, or 32.

                // Mip tails are stored in "reverse" order -- i.e., the mip-tail itself is stored first, so the
                // first mip level outside the tail has an offset that's the dimension of the tail itself, or one
                // swizzle block in size.
                offset         = blockSize * hwSlices;
                macroBlkOffset = blockSize;

                // And determine the per-mip information for everything inside the mip tail.
                GetMipOrigin(pIn, mipExtentFirstInTail, pOut);
            }

            // Again, because mip-levels are stored backwards (smallest first), we start determining mip-level
            // offsets from the smallest to the largest.
            // Note that firstMipInTail == 0 immediately terminates the loop, so there is no need to check for this
            // case.
            for (INT_32 i = firstMipInTail - 1; i >= 0; i--)
            {
                pOut->pMipInfo[i].offset           = offset;
                pOut->pMipInfo[i].macroBlockOffset = macroBlkOffset;
                pOut->pMipInfo[i].mipTailOffset    = 0;

                offset         += mipSize[i];
                macroBlkOffset += mipSliceSize[i];
            }
        }
    }
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlComputeSurfaceInfo
*
*   @brief
*       Internal function to calculate alignment for a surface
*
*   @return
*       ADDR_OK if successful.
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlComputeSurfaceInfo(
     const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn,    ///< [in] input structure
     ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*      pOut    ///< [out] output structure
     ) const
{
    // Check that only 2D swizzle mode supports MSAA
    const UINT_32 samplesLog2 = Is2dSwizzle(pIn->swizzleMode) ? Log2(pIn->numSamples) : 0;

    // The block dimension width/height/depth is determined only by swizzle mode, MSAA samples and bpp
    pOut->blockExtent = GetBlockDimensionTableEntry(pIn->swizzleMode, samplesLog2, Log2(pIn->bpp >> 3));

    ADDR_E_RETURNCODE  returnCode = ApplyCustomizedPitchHeight(pIn, pOut);

    if (returnCode == ADDR_OK)
    {
        pOut->numSlices = PowTwoAlign(pIn->numSlices, pOut->blockExtent.depth);
        pOut->baseAlign = 1 << GetBlockSizeLog2(pIn->swizzleMode);
        addr_params params = {};
        ConvertSurfInfoToAddrParams(pIn, &params);
        ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT input{ pIn, &params };
        GetMipOffset(&input, pOut);

        SanityCheckSurfSize(&input, pOut);

        // Slices must be exact multiples of the block sizes.  However:
        // - with 3D images, one block will contain multiple slices, so that needs to be taken into account.
        //
        // Note that with linear images that have only one slice, we can always guarantee pOut->sliceSize is 256B
        // alignment so there is no need to worry about it.
        ADDR_ASSERT(((pOut->sliceSize * pOut->blockExtent.depth) % GetBlockSize(pIn->swizzleMode)) == 0);
    }

    return returnCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetBaseMipExtents
*
*   @brief
*       Return the size of the base mip level in a nice cozy little structure.
*
*   @return
*       Return the size, in pixels, of the base mip level.
*
************************************************************************************************************************
*/
ADDR_EXTENT3D Gfx13Lib::GetBaseMipExtents(
    const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn
    ) const
{
    return { pIn->width,
             pIn->height,
             (IsTex3d(pIn->resourceType) ? pIn->numSlices : 1) }; // slices is depth for 3d
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlCalcMipInTail
*
*   @brief
*       Internal function to calculate the "mipInTail" parameter.
*
*   @return
*       The magic "mipInTail" parameter.
************************************************************************************************************************
*/
INT_32 Gfx13Lib::HwlCalcMipInTail(
    const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
    const ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*       pOut,
    UINT_32                                        mipLevel
    ) const
{
    const INT_32  firstMipIdInTail = static_cast<INT_32>(pOut->firstMipIdInTail);

    INT_32  mipInTail = 0;

    addr_params* pAddrParams = reinterpret_cast<addr_params*>(pIn->pvAddrParams);

    return calc_mip_in_tail(*pAddrParams, mipLevel, firstMipIdInTail);
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlCalcMipOffset
*
*   @brief
*       Determines the offset of the mip-tail in bytes.
*
*   @return
*       The byte offset of the mip tail.
************************************************************************************************************************
*/
UINT_32 Gfx13Lib::HwlCalcMipOffset(
    const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
    UINT_32                                        mipInTail
    ) const
{
    addr_params* pAddrParams = reinterpret_cast<addr_params*>(pIn->pvAddrParams);

    return calc_byte_offset(*pAddrParams, mipInTail);
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlComputeSurfaceAddrFromCoordLinear
*
*   @brief
*       Internal function to calculate address from coord for linear swizzle surface
*
*   @return
*       ADDR_E_RETURNCODE
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlComputeSurfaceAddrFromCoordLinear(
    const ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,         ///< [in] input structure
    const ADDR3_COMPUTE_SURFACE_INFO_INPUT*          pSurfInfoIn, ///< [in] input structure
    ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut         ///< [out] output structure
    ) const
{
    ADDR3_MIP_INFO mipInfo[MaxMipLevels];
    ADDR_ASSERT(pIn->numMipLevels <= MaxMipLevels);

    ADDR3_COMPUTE_SURFACE_INFO_OUTPUT surfInfoOut = {0};
    surfInfoOut.size     = sizeof(surfInfoOut);
    surfInfoOut.pMipInfo = mipInfo;

    ADDR_E_RETURNCODE returnCode = ComputeSurfaceInfo(pSurfInfoIn, &surfInfoOut);

    if (returnCode == ADDR_OK)
    {
        pOut->addr        = (surfInfoOut.sliceSize * pIn->slice) +
                            mipInfo[pIn->mipId].offset +
                            (pIn->y * mipInfo[pIn->mipId].pitch + pIn->x) * (pIn->bpp >> 3);

        pOut->bitPosition = 0;
    }

    return returnCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlComputeSurfaceAddrFromCoordTiled
*
*   @brief
*       Internal function to calculate address from coord for tiled swizzle surface
*
*   @return
*       ADDR_E_RETURNCODE
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlComputeSurfaceAddrFromCoordTiled(
     const ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,    ///< [in] input structure
     ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut    ///< [out] output structure
     ) const
{
    // 256B block cannot support 3D image.
    ADDR_ASSERT((IsTex3d(pIn->resourceType) && IsBlock256b(pIn->swizzleMode)) == FALSE);

    ADDR3_COMPUTE_SURFACE_INFO_INPUT  localIn               = {};
    ADDR3_COMPUTE_SURFACE_INFO_OUTPUT localOut              = {};
    ADDR3_MIP_INFO                    mipInfo[MaxMipLevels] = {};

    localIn.size         = sizeof(localIn);
    localIn.flags        = pIn->flags;
    localIn.swizzleMode  = pIn->swizzleMode;
    localIn.resourceType = pIn->resourceType;
    localIn.format       = ADDR_FMT_INVALID;
    localIn.bpp          = pIn->bpp;
    localIn.width        = Max(pIn->unAlignedDims.width, 1u);
    localIn.height       = Max(pIn->unAlignedDims.height, 1u);
    localIn.numSlices    = Max(pIn->unAlignedDims.depth, 1u);
    localIn.numMipLevels = Max(pIn->numMipLevels, 1u);
    localIn.numSamples   = Max(pIn->numSamples, 1u);

    localOut.size        = sizeof(localOut);
    localOut.pMipInfo    = mipInfo;

    addr_params params = {};
    ConvertSurfInfoToAddrParams(&localIn, &params);

    ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT input{ &localIn, &params };

    ADDR_E_RETURNCODE ret = ComputeSurfaceInfo(&localIn, &localOut);

    if (ret == ADDR_OK)
    {
        const UINT_32 elemLog2    = Log2(pIn->bpp >> 3);
        const UINT_32 blkSizeLog2 = GetBlockSizeLog2(pIn->swizzleMode);

        // Addr3 equation table excludes linear swizzle mode, and fortunately HwlComputeSurfaceAddrFromCoordTiled() is
        // only called for non-linear swizzle mode.
        const UINT_32 eqIndex     = GetEquationTableEntry(pIn->swizzleMode, Log2(localIn.numSamples), elemLog2);

        if (eqIndex != ADDR_INVALID_EQUATION_INDEX)
        {
            ADDR3_COORD  coords = {};
            UINT_32      zMacroBlockIndex  = 0;
            UINT_32      yxMacroBlockIndex = 0;

            const INT_32  mipInTail = HwlCalcMipInTail(&input, &localOut, pIn->mipId);

            HwlGetXyzBlockIndices(pIn,
                                    &params,
                                    &localOut,
                                    mipInTail,
                                    &yxMacroBlockIndex,
                                    &zMacroBlockIndex);

            HwlGetXyzOffsets(pIn, &params, mipInTail, &coords);

            // The below calculation to determine "addr" assumes that the "z" component has
            // already been included.  We're diverging from the original path here by adding
            // the "z" to the blkIdx value.
            const UINT_64  blkIdx = yxMacroBlockIndex + zMacroBlockIndex;

            // Convert to bytes
            coords.x = coords.x << elemLog2;

            const UINT_32 blkOffset  = ComputeOffsetFromEquation(m_pEquationTable[eqIndex],
                                                                 coords,
                                                                 pIn->sample);

            pOut->addr = mipInfo[pIn->mipId].macroBlockOffset +
                         (blkIdx << blkSizeLog2)              +
                         blkOffset;

            ADDR_ASSERT(pOut->addr < localOut.surfSize);
        }
        else
        {
            ret = ADDR_INVALIDPARAMS;
        }
    }

    return ret;
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetChannelValue
*
*   @brief
*       Returns the ordinal value that corresponds to the specified channel.
*
*   @return
*       Unsigned integer representing the ordinal associated with the specified channel.
************************************************************************************************************************
*/
UINT_32  Gfx13Lib::GetChannelValue(
    ADDR3_EQ_CHANNEL    channel,
    const ADDR3_COORD&  coord,
    UINT_32             s
    ) const
{
    UINT_32  retVal = 0;

    switch (channel)
    {
    case ADDR3_EQ_CHANNEL::_X:
        retVal = coord.x;
        break;
    case ADDR3_EQ_CHANNEL::_Y:
        retVal = coord.y;
        break;
    case ADDR3_EQ_CHANNEL::_Z:
        retVal = coord.z;
        break;
    case ADDR3_EQ_CHANNEL::_S:
        retVal = s;
        break;
    case ADDR3_EQ_CHANNEL::_B:
    default:
        // The "B" bits are only expected for the hTile equations and addrlib only provides
        // CPU-processing of the image equations.  So if we're here, then something has gone
        // very wrong.
        ADDR_ASSERT_ALWAYS();
        break;
    }

    return retVal;
}

/**
************************************************************************************************************************
*   Gfx13Lib::ComputeOffsetFromEquation
*
*   @brief
*       Compute offset from equation
*
*   @return
*       Byte ofset within a meta-block of the specified coordinate.
************************************************************************************************************************
*/
UINT_32 Gfx13Lib::ComputeOffsetFromEquation(
    const ADDR3_EQUATION* pEq,      ///< Equation
    const ADDR3_COORD&    coord,    ///< x/y/z coordinates
    UINT_32               s         ///< MSAA sample index
    ) const
{
    UINT_32 offset = 0;

    for (UINT_32 bitIdx = 0; bitIdx < pEq->numValidBits; bitIdx++)
    {
        const ADDR3_EQ_BIT*  pBit     = &pEq->bits[bitIdx];
        UINT_32              bitValue = 0;

        for (UINT_32 termIdx = 0; termIdx < pBit->numTerms; termIdx++)
        {
            const ADDR3_EQ_TERM*  pTerm      = &pBit->term[termIdx];
            const UINT_32         channelVal = GetChannelValue(pTerm->fields.channel, coord, s);
            const UINT_32         srcVal     = (channelVal >> pTerm->fields.ordinal) & 1;

            if (termIdx == 0)
            {
                bitValue = srcVal;
            }
            else
            {
                switch (pBit->term[termIdx - 1].fields.op)
                {
                case ADDR3_EQ_OPERATOR::_AND:
                    bitValue &= srcVal;
                    break;
                case ADDR3_EQ_OPERATOR::_XOR:
                    bitValue ^= srcVal;
                    break;
                default:
                    // Unknown operator
                    ADDR_ASSERT_ALWAYS();
                    break;
                }
            }
        }

        offset |= (bitValue << bitIdx);
    }

    return offset;
}

/**
************************************************************************************************************************
*   Gfx13Lib::ConvertEqBitToSetting
*
*   @brief
*       Translates an ADDR3 equation bit into its legacy equivalent.
*
*   @return
*       ADDR_OK on success.
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::ConvertEqBitToSetting(
    const ADDR3_EQ_BIT&  inputBit,
    ADDR_BIT_SETTING*    pOutputBit
    ) const
{
    ADDR_E_RETURNCODE  retCode= ADDR_OK;

    for (uint32  termIdx = 0; ((retCode == ADDR_OK) && (termIdx < inputBit.numTerms)); termIdx++)
    {
        const auto&  term = inputBit.term[termIdx];

        switch (term.fields.op)
        {
        case ADDR3_EQ_OPERATOR::_NONE:
        case ADDR3_EQ_OPERATOR::_XOR:
            switch (term.fields.channel)
            {
            case ADDR3_EQ_CHANNEL::_X:
            case ADDR3_EQ_CHANNEL::_Y:
            case ADDR3_EQ_CHANNEL::_Z:
            case ADDR3_EQ_CHANNEL::_S:
                // Normally the output value is zero, so we just store the return value of InitBit.
                // However, if the client is calling because of the "bit 8 xor" setting, then we
                // do need to use XOR here.
                pOutputBit->value ^= InitBit(term.fields.channel, term.fields.ordinal);
                break;
            default:
                // These channels can not be represented by the legacy equation structure.
                retCode = ADDR_NOTSUPPORTED;
                break;
            }
            break;

        case ADDR3_EQ_OPERATOR::_AND:
        default:
            // There is no way to represent this in the "bit setting" structure, so we have to fail.
            retCode = ADDR_NOTSUPPORTED;
            break;
        } // end switch on channel
    } // end loop through terms

    return retCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::ConvertEquationToBitSetting
*
*   @brief
*       Converts an ADDR3 equation into its legacy equivalent.
*
*   @return
*       Error or success.
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::ConvertEquationToBitSetting(
    const ADDR3_EQUATION*  pEquation,
    BOOL_32                bit8Xor,
    ADDR_BIT_SETTING*      pBitSetting
    ) const
{
    ADDR_E_RETURNCODE  retCode= ADDR_OK;

    for (uint32  bitIdx = 0; ((retCode == ADDR_OK) && (bitIdx < pEquation->numValidBits)); bitIdx++)
    {
        const ADDR3_EQ_BIT&  inputBit   = pEquation->bits[bitIdx];
        ADDR_BIT_SETTING*    pOutputBit = &pBitSetting[bitIdx];

        retCode = ConvertEqBitToSetting(inputBit, pOutputBit);
    } // end loop through bits

    if (bit8Xor && (retCode == ADDR_OK))
    {
        retCode = ConvertEqBitToSetting(pEquation->bits[11], &pBitSetting[8]);
    }

    return retCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlCopyMemToSurface
*
*   @brief
*       Copy multiple regions from memory to a non-linear surface.
*
*   @return
*       Error or success.
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlCopyMemToSurface(
    const ADDR3_COPY_MEMSURFACE_INPUT*  pIn,
    const ADDR3_COPY_MEMSURFACE_REGION* pRegions,
    UINT_32                             regionCount
    ) const
{
    // Copy memory to tiled surface. We will use the 'swizzler' object to dispatch to a version of the copy routine
    // optimized for a particular micro-swizzle mode if available.
    ADDR3_COMPUTE_SURFACE_INFO_INPUT  localIn  = {0};
    ADDR3_COMPUTE_SURFACE_INFO_OUTPUT localOut = {0};
    ADDR3_MIP_INFO                    mipInfo[MaxMipLevels] = {{0}};
    ADDR_ASSERT(pIn->numMipLevels <= MaxMipLevels);
    ADDR_E_RETURNCODE returnCode = ADDR_OK;

    if (pIn->numSamples > 1)
    {
        // TODO: MSAA
        returnCode = ADDR_NOTIMPLEMENTED;
    }

    localIn.size         = sizeof(localIn);
    localIn.flags        = pIn->flags;
    localIn.swizzleMode  = pIn->swizzleMode;
    localIn.resourceType = pIn->resourceType;
    localIn.format       = pIn->format;
    localIn.bpp          = pIn->bpp;
    localIn.width        = Max(pIn->unAlignedDims.width,  1u);
    localIn.height       = Max(pIn->unAlignedDims.height, 1u);
    localIn.numSlices    = Max(pIn->unAlignedDims.depth,  1u);
    localIn.numMipLevels = Max(pIn->numMipLevels,         1u);
    localIn.numSamples   = Max(pIn->numSamples,           1u);

    localOut.size     = sizeof(localOut);
    localOut.pMipInfo = mipInfo;

    if (returnCode == ADDR_OK)
    {
        returnCode = ComputeSurfaceInfo(&localIn, &localOut);
    }

    LutAddresser addresser = LutAddresser();
    UnalignedCopyMemImgFunc pfnCopyUnaligned = nullptr;
    if (returnCode == ADDR_OK)
    {
        const UINT_32          blkSizeLog2 = GetBlockSizeLog2(pIn->swizzleMode);
        const ADDR3_EQUATION*  pEquation   = GetEquation(Log2(localIn.bpp >> 3),
                                                         localIn.swizzleMode,
                                                         Log2(localIn.numSamples),
                                                         FALSE);

        if (pEquation != nullptr)
        {
            ADDR_BIT_SETTING fullSwizzlePattern[Log2Size256K] = {};

            returnCode = ConvertEquationToBitSetting(pEquation,
                                                     HasBit8Xor(localIn.swizzleMode),
                                                     &fullSwizzlePattern[0]);

            if (returnCode == ADDR_OK)
            {
                addresser.Init(fullSwizzlePattern, Log2Size256K, localOut.blockExtent, blkSizeLog2);
                pfnCopyUnaligned = addresser.GetCopyMemImgFunc(pIn->copyFlags);
                if (pfnCopyUnaligned == nullptr)
                {
                    returnCode = ADDR_INVALIDPARAMS;
                }
            }
        }
        else
        {
            returnCode = ADDR_INVALIDPARAMS;
        }

        ADDR_ASSERT(returnCode == ADDR_OK);
    }

    if (returnCode == ADDR_OK)
    {
        for (UINT_32  regionIdx = 0; regionIdx < regionCount; regionIdx++)
        {
            const ADDR3_COPY_MEMSURFACE_REGION* pCurRegion = &pRegions[regionIdx];
            const ADDR3_MIP_INFO* pMipInfo = &mipInfo[pCurRegion->mipId];
            UINT_64 mipOffset = pIn->singleSubres ? 0 : pMipInfo->macroBlockOffset;
            UINT_32 yBlks = pMipInfo->pitch / localOut.blockExtent.width;
            UINT_32 zBlks = localOut.sliceSize >> (addresser.GetBlockBits() - addresser.GetBlockZBits());

            ADDR_COORD3D rawOrigin = {
                pCurRegion->x + pMipInfo->mipTailCoordX,
                pCurRegion->y + pMipInfo->mipTailCoordY,
                pCurRegion->slice + pMipInfo->mipTailCoordZ
            };

            pfnCopyUnaligned(VoidPtrInc(pIn->pMappedSurface, mipOffset),
                             pCurRegion->pMem,
                             pCurRegion->memRowPitch,
                             pCurRegion->memSlicePitch,
                             yBlks,
                             zBlks,
                             rawOrigin,
                             pCurRegion->copyDims,
                             pIn->pbXor,
                             (pCurRegion->mipId >= localOut.firstMipIdInTail),
                             addresser);
        }
        addresser.DoCopyMemImgPostFlushes(pIn->copyFlags);
    }
    return returnCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlCopySurfaceToMem
*
*   @brief
*       Copy multiple regions from a non-linear surface to memory.
*
*   @return
*       Error or success.
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlCopySurfaceToMem(
    const ADDR3_COPY_MEMSURFACE_INPUT*  pIn,
    const ADDR3_COPY_MEMSURFACE_REGION* pRegions,
    UINT_32                             regionCount
    ) const
{
    // Copy memory to tiled surface. We will use the 'swizzler' object to dispatch to a version of the copy routine
    // optimized for a particular micro-swizzle mode if available.
    ADDR3_COMPUTE_SURFACE_INFO_INPUT  localIn  = {0};
    ADDR3_COMPUTE_SURFACE_INFO_OUTPUT localOut = {0};
    ADDR3_MIP_INFO                    mipInfo[MaxMipLevels] = {{0}};
    ADDR_ASSERT(pIn->numMipLevels <= MaxMipLevels);
    ADDR_E_RETURNCODE returnCode = ADDR_OK;

    if (pIn->numSamples > 1)
    {
        // TODO: MSAA
        returnCode = ADDR_NOTIMPLEMENTED;
    }

    localIn.size         = sizeof(localIn);
    localIn.flags        = pIn->flags;
    localIn.swizzleMode  = pIn->swizzleMode;
    localIn.resourceType = pIn->resourceType;
    localIn.format       = pIn->format;
    localIn.bpp          = pIn->bpp;
    localIn.width        = Max(pIn->unAlignedDims.width,  1u);
    localIn.height       = Max(pIn->unAlignedDims.height, 1u);
    localIn.numSlices    = Max(pIn->unAlignedDims.depth,  1u);
    localIn.numMipLevels = Max(pIn->numMipLevels,         1u);
    localIn.numSamples   = Max(pIn->numSamples,           1u);

    localOut.size     = sizeof(localOut);
    localOut.pMipInfo = mipInfo;

    if (returnCode == ADDR_OK)
    {
        returnCode = ComputeSurfaceInfo(&localIn, &localOut);
    }

    LutAddresser addresser = LutAddresser();
    UnalignedCopyMemImgFunc pfnCopyUnaligned = nullptr;
    if (returnCode == ADDR_OK)
    {
        const UINT_32          blkSizeLog2 = GetBlockSizeLog2(pIn->swizzleMode);
        const ADDR3_EQUATION*  pEquation   = GetEquation(Log2(localIn.bpp >> 3),
                                                         localIn.swizzleMode,
                                                         Log2(localIn.numSamples),
                                                         FALSE);

        if (pEquation != nullptr)
        {
            ADDR_BIT_SETTING fullSwizzlePattern[Log2Size256K] = {};

            returnCode = ConvertEquationToBitSetting(pEquation,
                                                     HasBit8Xor(localIn.swizzleMode),
                                                     &fullSwizzlePattern[0]);

            if (returnCode == ADDR_OK)
            {
                addresser.Init(fullSwizzlePattern, Log2Size256K, localOut.blockExtent, blkSizeLog2);
                pfnCopyUnaligned = addresser.GetCopyImgMemFunc(pIn->copyFlags);
                if (pfnCopyUnaligned == nullptr)
                {
                    returnCode = ADDR_INVALIDPARAMS;
                }
            }
        }
        else
        {
            returnCode = ADDR_INVALIDPARAMS;
        }

        ADDR_ASSERT(returnCode == ADDR_OK);
    }

    if (returnCode == ADDR_OK)
    {
        addresser.DoCopyImgMemPreFlushes(pIn->copyFlags);
        for (UINT_32  regionIdx = 0; regionIdx < regionCount; regionIdx++)
        {
            const ADDR3_COPY_MEMSURFACE_REGION* pCurRegion = &pRegions[regionIdx];
            const ADDR3_MIP_INFO* pMipInfo = &mipInfo[pCurRegion->mipId];
            UINT_64 mipOffset = pIn->singleSubres ? 0 : pMipInfo->macroBlockOffset;
            UINT_32 yBlks = pMipInfo->pitch / localOut.blockExtent.width;
            UINT_32 zBlks = localOut.sliceSize >> (addresser.GetBlockBits() - addresser.GetBlockZBits());

            ADDR_COORD3D rawOrigin = {
                pCurRegion->x + pMipInfo->mipTailCoordX,
                pCurRegion->y + pMipInfo->mipTailCoordY,
                pCurRegion->slice + pMipInfo->mipTailCoordZ
            };

            pfnCopyUnaligned(VoidPtrInc(pIn->pMappedSurface, mipOffset),
                             pCurRegion->pMem,
                             pCurRegion->memRowPitch,
                             pCurRegion->memSlicePitch,
                             yBlks,
                             zBlks,
                             rawOrigin,
                             pCurRegion->copyDims,
                             pIn->pbXor,
                             (pCurRegion->mipId >= localOut.firstMipIdInTail),
                             addresser);
        }
    }
    return returnCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlInitGlobalParams
*
*   @brief
*       Initializes global parameters
*
*   @return
*       TRUE if all settings are valid
*
************************************************************************************************************************
*/
BOOL_32 Gfx13Lib::HwlInitGlobalParams(
    const ADDR_CREATE_INPUT* pCreateIn) ///< [in] create input
{
    GB_ADDR_CONFIG_GFX13  gbAddrConfig;

    gbAddrConfig.u32All = pCreateIn->regValue.gbAddrConfig;
    m_bitEight2dXor = gbAddrConfig.bits.BIT8_2D_XOR;

    // Gfx10+ chips treat packed 8-bit 422 formats as 32bpe with 2pix/elem.
    m_configFlags.use32bppFor422Fmt = TRUE;

    InitEquationTable();
    InitBlockDimensionTable();

    return TRUE;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlComputeNonBlockCompressedView
*
*   @brief
*       Compute non-block-compressed view for a given mipmap level/slice.
*
*   @return
*       ADDR_E_RETURNCODE
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlComputeNonBlockCompressedView(
    const ADDR3_COMPUTE_NONBLOCKCOMPRESSEDVIEW_INPUT* pIn,    ///< [in] input structure
    ADDR3_COMPUTE_NONBLOCKCOMPRESSEDVIEW_OUTPUT*      pOut    ///< [out] output structure
    ) const
{
    ADDR_E_RETURNCODE returnCode = ADDR_OK;

    if (((pIn->format < ADDR_FMT_ASTC_4x4) || (pIn->format > ADDR_FMT_ETC2_128BPP)) &&
        ((pIn->format < ADDR_FMT_BC1) || (pIn->format > ADDR_FMT_BC7)))
    {
        // Only support BC1~BC7, ASTC, or ETC2 for now...
        returnCode = ADDR_NOTSUPPORTED;
    }
    else
    {
        UINT_32 bcWidth, bcHeight;
        const UINT_32 bpp = GetElemLib()->GetBitsPerPixel(pIn->format, NULL, &bcWidth, &bcHeight);

        ADDR3_COMPUTE_SURFACE_INFO_INPUT infoIn = {};
        infoIn.size         = sizeof(infoIn);
        infoIn.flags        = pIn->flags;
        infoIn.swizzleMode  = pIn->swizzleMode;
        infoIn.resourceType = pIn->resourceType;
        infoIn.format       = pIn->format;
        infoIn.bpp          = bpp;
        infoIn.width        = RoundUpQuotient(pIn->unAlignedDims.width, bcWidth);
        infoIn.height       = RoundUpQuotient(pIn->unAlignedDims.height, bcHeight);
        infoIn.numSlices    = pIn->unAlignedDims.depth;
        infoIn.numMipLevels = pIn->numMipLevels;
        infoIn.numSamples   = 1;

        ADDR3_MIP_INFO mipInfo[MaxMipLevels] = {};

        ADDR3_COMPUTE_SURFACE_INFO_OUTPUT infoOut = {};
        infoOut.size     = sizeof(infoOut);
        infoOut.pMipInfo = mipInfo;

        returnCode = HwlComputeSurfaceInfo(&infoIn, &infoOut);

        if (returnCode == ADDR_OK)
        {
            ADDR3_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT subOffIn = {};
            subOffIn.size             = sizeof(subOffIn);
            subOffIn.swizzleMode      = infoIn.swizzleMode;
            subOffIn.resourceType     = infoIn.resourceType;
            subOffIn.pipeBankXor      = pIn->pipeBankXor;
            subOffIn.slice            = pIn->slice;
            subOffIn.sliceSize        = infoOut.sliceSize;
            subOffIn.macroBlockOffset = mipInfo[pIn->mipId].macroBlockOffset;
            subOffIn.mipTailOffset    = mipInfo[pIn->mipId].mipTailOffset;

            ADDR3_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_OUTPUT subOffOut = {};
            subOffOut.size = sizeof(subOffOut);

            // For any mipmap level, move nonBc view base address by offset
            HwlComputeSubResourceOffsetForSwizzlePattern(&subOffIn, &subOffOut);
            pOut->offset = subOffOut.offset;

            ADDR3_COMPUTE_SLICE_PIPEBANKXOR_INPUT slicePbXorIn = {};
            slicePbXorIn.size            = sizeof(slicePbXorIn);
            slicePbXorIn.swizzleMode     = infoIn.swizzleMode;
            slicePbXorIn.resourceType    = infoIn.resourceType;
            slicePbXorIn.bpe             = infoIn.bpp;
            slicePbXorIn.basePipeBankXor = pIn->pipeBankXor;
            slicePbXorIn.slice           = pIn->slice;
            slicePbXorIn.numSamples      = 1;

            ADDR3_COMPUTE_SLICE_PIPEBANKXOR_OUTPUT slicePbXorOut = {};
            slicePbXorOut.size = sizeof(slicePbXorOut);

            // For any mipmap level, nonBc view should use computed pbXor
            HwlComputeSlicePipeBankXor(&slicePbXorIn, &slicePbXorOut);
            pOut->pipeBankXor = slicePbXorOut.pipeBankXor;

            const BOOL_32 tiled            = (pIn->swizzleMode != ADDR3_LINEAR);
            const BOOL_32 inTail           = tiled && (pIn->mipId >= infoOut.firstMipIdInTail);
            const UINT_32 requestMipWidth  =
                    RoundUpQuotient(Max(pIn->unAlignedDims.width  >> pIn->mipId, 1u), bcWidth);
            const UINT_32 requestMipHeight =
                    RoundUpQuotient(Max(pIn->unAlignedDims.height >> pIn->mipId, 1u), bcHeight);

            if (inTail)
            {
                // Basically all mipmap levels in tail block will be viewed as a small mipmap chain that all levels
                // are fit in tail block:

                // - mipId = relative mip id (which is counted from first mip ID in tail in original mip chain)
                pOut->mipId = pIn->mipId - infoOut.firstMipIdInTail;

                // - at least 2 mipmap levels (since only 1 mipmap level will not be viewed as mipmap!)
                pOut->numMipLevels = Max(infoIn.numMipLevels - infoOut.firstMipIdInTail, 2u);

                // - (mip0) width = requestMipWidth << mipId, the value can't exceed mip tail dimension threshold
                pOut->unAlignedDims.width  = Min(requestMipWidth << pOut->mipId, infoOut.blockExtent.width / 2);

                // - (mip0) height = requestMipHeight << mipId, the value can't exceed mip tail dimension threshold
                pOut->unAlignedDims.height = Min(requestMipHeight << pOut->mipId, infoOut.blockExtent.height);
            }
            // This check should cover at least mipId == 0
            else if ((requestMipWidth << pIn->mipId) == infoIn.width)
            {
                // For mipmap level [N] that is not in mip tail block and downgraded without losing element:
                // - only one mipmap level and mipId = 0
                pOut->mipId        = 0;
                pOut->numMipLevels = 1;

                // (mip0) width = requestMipWidth
                pOut->unAlignedDims.width  = requestMipWidth;

                // (mip0) height = requestMipHeight
                pOut->unAlignedDims.height = requestMipHeight;
            }
            else
            {
                // For mipmap level [N] that is not in mip tail block and downgraded with element losing,
                // We have to make it a multiple mipmap view (2 levels view here), add one extra element if needed,
                // because single mip view may have different pitch value than original (multiple) mip view...
                // A simple case would be:
                // - 64KB block swizzle mode, 8 Bytes-Per-Element. Block dim = [0x80, 0x40]
                // - 2 mipmap levels with API mip0 width = 0x401/mip1 width = 0x200 and non-BC view
                //   mip0 width = 0x101/mip1 width = 0x80
                // By multiple mip view, the pitch for mip level 1 would be 0x100 bytes, due to rounding up logic in
                // GetMipSize(), and by single mip level view the pitch will only be 0x80 bytes.

                // - 2 levels and mipId = 1
                pOut->mipId        = 1;
                pOut->numMipLevels = 2;

                const UINT_32 upperMipWidth  =
                    RoundUpQuotient(Max(pIn->unAlignedDims.width  >> (pIn->mipId - 1), 1u), bcWidth);
                const UINT_32 upperMipHeight =
                    RoundUpQuotient(Max(pIn->unAlignedDims.height >> (pIn->mipId - 1), 1u), bcHeight);

                const BOOL_32 needToAvoidInTail = tiled                                              &&
                                                  (requestMipWidth <= infoOut.blockExtent.width / 2) &&
                                                  (requestMipHeight <= infoOut.blockExtent.height);

                const UINT_32 hwMipWidth  =
                    PowTwoAlign(ShiftCeil(infoIn.width, pIn->mipId), infoOut.blockExtent.width);
                const UINT_32 hwMipHeight =
                    PowTwoAlign(ShiftCeil(infoIn.height, pIn->mipId), infoOut.blockExtent.height);

                const BOOL_32 needExtraWidth =
                    ((upperMipWidth < requestMipWidth * 2) ||
                     ((upperMipWidth == requestMipWidth * 2) &&
                      ((needToAvoidInTail == TRUE) ||
                       (hwMipWidth > PowTwoAlign(requestMipWidth, infoOut.blockExtent.width)))));

                const BOOL_32 needExtraHeight =
                    ((upperMipHeight < requestMipHeight * 2) ||
                     ((upperMipHeight == requestMipHeight * 2) &&
                      ((needToAvoidInTail == TRUE) ||
                       (hwMipHeight > PowTwoAlign(requestMipHeight, infoOut.blockExtent.height)))));

                // (mip0) width = requestLastMipLevelWidth
                pOut->unAlignedDims.width  = upperMipWidth + (needExtraWidth ? 1: 0);

                // (mip0) height = requestLastMipLevelHeight
                pOut->unAlignedDims.height = upperMipHeight + (needExtraHeight ? 1: 0);
            }

            // Assert the downgrading from this mip[0] width would still generate correct mip[N] width
            ADDR_ASSERT(ShiftRight(pOut->unAlignedDims.width, pOut->mipId)  == requestMipWidth);
            // Assert the downgrading from this mip[0] height would still generate correct mip[N] height
            ADDR_ASSERT(ShiftRight(pOut->unAlignedDims.height, pOut->mipId) == requestMipHeight);
        }
    }

    return returnCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlComputeSubResourceOffsetForSwizzlePattern
*
*   @brief
*       Compute sub resource offset to support swizzle pattern
*
*   @return
*       VOID
************************************************************************************************************************
*/
VOID Gfx13Lib::HwlComputeSubResourceOffsetForSwizzlePattern(
    const ADDR3_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT* pIn,    ///< [in] input structure
    ADDR3_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_OUTPUT*      pOut    ///< [out] output structure
    ) const
{
    pOut->offset = pIn->slice * pIn->sliceSize + pIn->macroBlockOffset;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlComputeSlicePipeBankXor
*
*   @brief
*       Generate slice PipeBankXor value based on base PipeBankXor value and slice id
*
*   @return
*       PipeBankXor value
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlComputeSlicePipeBankXor(
    const ADDR3_COMPUTE_SLICE_PIPEBANKXOR_INPUT* pIn,   ///< [in] input structure
    ADDR3_COMPUTE_SLICE_PIPEBANKXOR_OUTPUT*      pOut   ///< [out] output structure
    ) const
{
    ADDR_E_RETURNCODE returnCode = ADDR_OK;

    // PipeBankXor is only applied to 4KB, 64KB and 256KB on GFX13.
    if ((IsLinear(pIn->swizzleMode) == FALSE) && (IsBlock256b(pIn->swizzleMode) == FALSE))
    {
        if (pIn->bpe == 0)
        {
            // Require a valid bytes-per-element value passed from client...
            returnCode = ADDR_INVALIDPARAMS;
        }
        else
        {
            const UINT_32 elemLog2 = Log2(pIn->bpe >> 3);

            // Addr3 equation table excludes linear swizzle mode, and fortunately when calling
            // HwlComputeSlicePipeBankXor the swizzle mode is non-linear, so we don't need to worry about negative
            // table index.
            const UINT_32      eqIndex = GetEquationTableEntry(pIn->swizzleMode, Log2(pIn->numSamples), elemLog2);
            const ADDR3_COORD  coord   = { 0, 0, static_cast<INT_32>(pIn->slice) };

            const UINT_32 pipeBankXorOffset = ComputeOffsetFromEquation(m_pEquationTable[eqIndex], coord, 0);

            const UINT_32 pipeBankXor = pipeBankXorOffset >> PipeInterleaveLog2;

            // Should have no bit set under pipe interleave
            ADDR_ASSERT((pipeBankXor << PipeInterleaveLog2) == pipeBankXorOffset);

            pOut->pipeBankXor = pIn->basePipeBankXor ^ pipeBankXor;
        }
    }
    else
    {
        pOut->pipeBankXor = 0;
    }

    return returnCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlConvertChipFamily
*
*   @brief
*       Convert familyID defined in atiid.h to ChipFamily and set m_chipFamily/m_chipRevision
*   @return
*       ChipFamily
************************************************************************************************************************
*/
ChipFamily Gfx13Lib::HwlConvertChipFamily(
    UINT_32 chipFamily,        ///< [in] chip family defined in atiih.h
    UINT_32 chipRevision)      ///< [in] chip revision defined in "asic_family"_id.h
{
    return ADDR_CHIP_FAMILY_GFX13;
}

/**
************************************************************************************************************************
*   Gfx13Lib::SanityCheckSurfSize
*
*   @brief
*       Calculate the surface size via the exact hardware algorithm to see if it matches.
*
*   @return
************************************************************************************************************************
*/
void Gfx13Lib::SanityCheckSurfSize(
    const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
    const ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*       pOut
    ) const
{
#if DEBUG
    const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pSurfInfo = pIn->pSurfInfo;
    // Verify that the requested image size is valid for the below algorithm.  The below code includes
    // implicit assumptions about the surface dimensions being less than "MaxImageDim"; otherwise, it can't
    // calculate "firstMipInTail" accurately and the below assertion will trip incorrectly.
    //
    // Surfaces destined for use only on the SDMA engine can exceed the gfx-engine-imposed limitations of
    // the "maximum" image dimensions.
    if ((pSurfInfo->width <= MaxImageDim)         &&
        (pSurfInfo->height <= MaxImageDim)        &&
        (pSurfInfo->numMipLevels <= MaxMipLevels) &&
        (UseCustomPitch(pSurfInfo) == FALSE)      &&
        (UseCustomHeight(pSurfInfo) == FALSE)     &&
        // HiZS surfaces have a reduced image size (i.e,. each pixel represents an 8x8 region of the parent
        // image, at least for single samples) but they still have the same number of mip levels as the
        // parent image.  This disconnect produces false assertions below as the image size doesn't apparently
        // support the specified number of mip levels.
        ((pSurfInfo->flags.hiZHiS == 0) || (pSurfInfo->numMipLevels == 1)))
    {
        INT_32   mipInTail     = 0;
        UINT_64  dataChainSize = HwlGetMipOffset(pIn->pvAddrParams, 0, &mipInTail);
        if (CanTrimLinearPadding(pSurfInfo))
        {
            ADDR_ASSERT((pOut->sliceSize * pOut->blockExtent.depth) <= dataChainSize);
        }
        else
        {
            ADDR_ASSERT((pOut->sliceSize * pOut->blockExtent.depth) == dataChainSize);
        }
    }
#endif // DEBUG
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlGetMipOffset
*
*   @brief
*       Determines the starting location (in bytes) from the start of the image as to where the specified
*       mip level starts.
*
*   @return
*       Offset in bytes to the specified mip level.
************************************************************************************************************************
*/
INT_64 Gfx13Lib::HwlGetMipOffset(
    void*    pvAddrParams,
    INT_32   mipId,
    INT_32*  pMipInTail
    ) const
{
    addr_params* pAddrParams = reinterpret_cast<addr_params*>(pvAddrParams);
    INT_64       dataChainSize = 0;

    ADDR_ASSERT(pAddrParams != NULL);

    INT_64 dataOffset    = 0;
    INT_64 metaOffset    = 0;
    INT_64 metaChainSize = 0;

    getMipOffset(*pAddrParams, mipId, &dataOffset, &metaOffset, pMipInTail, &dataChainSize, &metaChainSize);

    return dataChainSize;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlCalcBlockSize
*
*   @brief
*       Determines the extent, in pixels of a swizzle block.
*
*   @return
*       None.
************************************************************************************************************************
*/
VOID Gfx13Lib::HwlCalcBlockSize(
    const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
    ADDR_EXTENT3D*                                 pExtent
    ) const
{
    ADDR_ASSERT(pIn->pvAddrParams != NULL);

    addr_params* pAddrParams = reinterpret_cast<addr_params*>(pIn->pvAddrParams);
    INT_32       widthLog2   = 0;
    INT_32       heightLog2  = 0;
    INT_32       depthLog2   = 0;

    calcBlockSizeLog2(*pAddrParams, pAddrParams->slice_block_size_log2, &widthLog2, &heightLog2, &depthLog2);

    pExtent->width  = 1 << widthLog2;
    pExtent->height = 1 << heightLog2;
    pExtent->depth  = 1 << depthLog2;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlGetMipInTailMaxSize
*
*   @brief
*       Determines the max size of a mip level that fits in the mip-tail.
*
*   @return
************************************************************************************************************************
*/
ADDR_EXTENT3D Gfx13Lib::HwlGetMipInTailMaxSize(
    const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
    const ADDR_EXTENT3D&                           blockDims
    ) const
{
    ADDR_ASSERT(pIn->pvAddrParams != NULL);

    ADDR_EXTENT3D mipTailDim = {};

    addr_params* pAddrParams = reinterpret_cast<addr_params*>(pIn->pvAddrParams);
    INT_32       widthLog2  = 0;
    INT_32       heightLog2 = 0;

    getMipInTaleMaxSize(*pAddrParams, &widthLog2, &heightLog2);

    mipTailDim.width  = 1u << widthLog2;
    mipTailDim.height = 1u << heightLog2;
    mipTailDim.depth  = 0;

    return mipTailDim;
}

/**
************************************************************************************************************************
*   Gfx13Lib::ComputeHtilePerMipInfo
*
*   @brief
*       Compute the per-mip information.
*
************************************************************************************************************************
*/
VOID Gfx13Lib::ComputeHtilePerMipInfo(
    const ADDR3_COMPUTE_HTILE_INFO_INPUT* pIn,    ///< [in] input structure
    ADDR3_COMPUTE_HTILE_INFO_OUTPUT*      pOut    ///< [out] output structure
    ) const
{
    if (pIn->numMipLevels > 1)
    {
        addr_params params = {};
        ConvertHtileInfoToAddrParams(pIn, &params);

        // This is the "real" size of a meta-block as viewed across all the RBs.  This is more
        // useful for computing hTile offsets and sizes, etc.  The mip levels in the tail, by
        // definition, take one block.
        const UINT_32  mipTailSizeInBytes = convertHtileElementsToBytes(
                                                    params,
                                                    convertHtileMetaBlocksToElements(params, 1));

        // The smallest mip levels are stored first, so iterate through the mip levels from smallest
        // to largest.  Our loop does not account for the mip-tail, so the offset of the first mip
        // is the size of the mip-tail.
        //
        // Exception:  if we effectively don't have a tail then our offset is zero since we'll
        //             calculate all the mip-levels individually below.
        UINT_32 offset = 0;
        if (pIn->numMipLevels != pIn->firstMipIdInTail)
        {
            offset = mipTailSizeInBytes;
            pOut->pMipInfo[pIn->firstMipIdInTail].sliceSize = mipTailSizeInBytes;
        }

        for (INT_32 mipIdx = static_cast<INT_32>(pIn->firstMipIdInTail) - 1; mipIdx >= 0; mipIdx--)
        {
            const UINT_32  mipSizeInBlocks   = getHtileNumMetaBlocksPerMipLevel(params,
                                                                                pIn->unalignedDims.width,
                                                                                pIn->unalignedDims.height,
                                                                                mipIdx);
            const UINT_32  mipSizeInElements = convertHtileMetaBlocksToElements(params,
                                                                                mipSizeInBlocks);
            const UINT_32  mipSizeInBytes    = convertHtileElementsToBytes(params,
                                                                           mipSizeInElements);

            pOut->pMipInfo[mipIdx].offset    = offset;
            pOut->pMipInfo[mipIdx].sliceSize = mipSizeInBytes;

            offset += mipSizeInBytes;
        }
    }
    else
    {
        pOut->pMipInfo[0].sliceSize = pOut->sliceSize;
    }
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlComputeHtileInfo
*
*   @brief
*       Interface function stub of Addr3ComputeHtileInfo
*
*   @return
*       ADDR_E_RETURNCODE
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlComputeHtileInfo(
    const ADDR3_COMPUTE_HTILE_INFO_INPUT* pIn,    ///< [in] input structure
    ADDR3_COMPUTE_HTILE_INFO_OUTPUT*      pOut    ///< [out] output structure
    ) const
{
    ADDR_E_RETURNCODE ret = ADDR_OK;

    if ((pIn->swizzleMode != ADDR3_64KB_2D_Z) &&
        (pIn->swizzleMode != ADDR3_256KB_2D_Z))
    {
        ret = ADDR_INVALIDPARAMS;
    }
    else
    {
        Addr3SwizzleMode swMode = pIn->swizzleMode;
        if (pOut->pEquation != nullptr)
        {
            const ADDR3_EQUATION*  pEquation = GetEquation(0, swMode, 0, TRUE);

            if (pEquation != nullptr)
            {
                memcpy(pOut->pEquation, pEquation, sizeof(ADDR3_EQUATION));
            }
            else
            {
                // No equation for this device yet, mark the output structure
                // as having no valid bits.
                pOut->pEquation->numValidBits = 0;
            }
        }

        addr_params params = {};
        ConvertHtileInfoToAddrParams(pIn, &params);

        INT_32 metaBlkDepth  = 0;

        // The "metaBlkSize" is in units of bytes per RB.  The number of RBs is configuration dependent.
        pOut->baseAlign = getMetaBlockSize(params,
                                           reinterpret_cast<int32*>(&pOut->metaBlkWidth),
                                           reinterpret_cast<int32*>(&pOut->metaBlkHeight),
                                           &metaBlkDepth);

        pOut->pitch         = RoundUpToMultiple(pIn->unalignedDims.width, pOut->metaBlkWidth);
        pOut->height        = RoundUpToMultiple(pIn->unalignedDims.height, pOut->metaBlkHeight);
        pOut->sliceSize     = getHTileMetaSlice(params, pIn->unalignedDims.width, pIn->unalignedDims.height);
        pOut->vrsSliceSize  = getMetaSlice(params, pIn->unalignedDims.width, pIn->unalignedDims.height);
        pOut->htileBytes    = getHTileMetaSize(params,
                                               pIn->unalignedDims.width,
                                               pIn->unalignedDims.height,
                                               pIn->unalignedDims.depth);

        // If the client has requested per-mip level information, provide it here.
        if (pOut->pMipInfo != NULL)
        {
            ComputeHtilePerMipInfo(pIn, pOut);
        }
    }

    return ret;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlComputeFmaskInfo
*
*   @brief
*       Calculate fmask addressing info
*
*   @return
*       ADDR_E_RETURNCODE
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlComputeFmaskInfo(
    const ADDR3_COMPUTE_FMASK_INFO_INPUT* pIn,
    ADDR3_COMPUTE_FMASK_INFO_OUTPUT*      pOut
    ) const
{

    ADDR3_COMPUTE_SURFACE_INFO_INPUT  localIn = {0};
    ADDR3_COMPUTE_SURFACE_INFO_OUTPUT localOut = {0};

    localIn.size = sizeof(ADDR3_COMPUTE_SURFACE_INFO_INPUT);
    localOut.size = sizeof(ADDR3_COMPUTE_SURFACE_INFO_OUTPUT);

    localIn.swizzleMode  = pIn->swizzleMode;
    localIn.numSlices    = Max(pIn->numSlices, 1u);
    localIn.width        = Max(pIn->unalignedWidth, 1u);
    localIn.height       = Max(pIn->unalignedHeight, 1u);
    localIn.bpp          = GetFmaskBpp(pIn->numSamples);
    localIn.flags.fmask  = 1;
    localIn.numSamples   = 1;
    localIn.resourceType = ADDR_RSRC_TEX_2D;

    if (localIn.bpp == 8)
    {
        localIn.format = ADDR_FMT_8;
    }
    else if (localIn.bpp == 16)
    {
        localIn.format = ADDR_FMT_16;
    }
    else if (localIn.bpp == 32)
    {
        localIn.format = ADDR_FMT_32;
    }
    else
    {
        localIn.format = ADDR_FMT_32_32;
    }

    ADDR_E_RETURNCODE returnCode = ComputeSurfaceInfo(&localIn, &localOut);

    if (returnCode == ADDR_OK)
    {
        pOut->pitch      = localOut.pitch;
        pOut->height     = localOut.height;
        pOut->baseAlign  = localOut.baseAlign;
        pOut->numSlices  = localOut.numSlices;
        pOut->fmaskBytes = localOut.surfSize;
        pOut->sliceSize  = localOut.sliceSize;
        pOut->bpp        = localIn.bpp;
    }

    ValidBaseAlignments(pOut->baseAlign);

    return returnCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::GetPossibleSwizzleModes
*
*   @brief
*       GFX13 specific implementation of Addr3GetPossibleSwizzleModes
*
*   @return
*       ADDR_E_RETURNCODE
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlGetPossibleSwizzleModes(
     const ADDR3_GET_POSSIBLE_SWIZZLE_MODE_INPUT* pIn,    ///< [in] input structure
     ADDR3_GET_POSSIBLE_SWIZZLE_MODE_OUTPUT*      pOut    ///< [out] output structure
     ) const
{
    ADDR_E_RETURNCODE returnCode = ADDR_OK;

    const ADDR3_SURFACE_FLAGS flags = pIn->flags;

    if (pIn->bpp == 96)
    {
        pOut->validModes.swLinear = 1;
    }
    else
    {
        // The newly added Z_2D modes are only used in two scenarios
        //      1. fmask
        //      2. depth/stencil views (DSV)
        // For R_2D, since they're expected to share same tiling equations as Z_2D for single-sample surfaces, then for
        // sake of legacy expectations, we restrict R for color surfaces.

        // GFX13 brings fmask back similar to GFX10 fmask concepts except that GFX13 has cmask gone while GFX10 couples
        // cmask and fmask together.
        if (flags.fmask)
        {
            // Current CSIM supports both 2D_Z and 2D_R swizzle modes and we expect FMask to be supported for
            // 64KB_Z_2D or 256KB_Z_2D.
            pOut->validModes.sw2d64kBz  = 1;
            pOut->validModes.sw2d256kBz = 1;
        }
        else if (flags.depth || flags.stencil)
        {
            // A depth/stencil view (DSV) is only supported by 64KB and 256KB block sizes
            //
            // MSAA depth/stencil view (DSV) resources must use one of the SW_*_Z_2D modes.
            // Non-MSAA DSV can support all 64KB/256KB_R/Z_2D from hwdoc. VMEM doc says Z/R modes are identical from a
            // tiling perspective for 1xAA (non-MSAA) surfaces, so it's easiest to just set Z for non-MSAA DSV.
            pOut->validModes.sw2d64kBz  = 1;
            pOut->validModes.sw2d256kBz = 1;
        }
        else if (pIn->numSamples > 1)
        {
            // Non-depth/stencil MSAA only supports SW_64KB_R_2D and SW_256KB_R_2D
            pOut->validModes.sw2d64kB   = 1;
            pOut->validModes.sw2d256kB  = 1;
        }
        // Some APIs (like Vulkan) require that PRT should always use 64KB blocks
        else if (flags.standardPrt)
        {
            if (IsTex3d(pIn->resourceType) && (flags.view3dAs2dArray == 0))
            {
                pOut->validModes.sw3d64kB = 1;
            }
            else
            {
                pOut->validModes.sw2d64kB = 1;
            }
        }
        // Block-compressed, 3D resources w/ view3dAs2dArray==1 and non-3D resources need to be either using 2D or
        // linear swizzle modes.
        else if (flags.blockCompressed || (IsTex3d(pIn->resourceType) == FALSE) || flags.view3dAs2dArray)
        {
            // RDNA5 doc said
            // PRT (tiled resources) must be one of the 4KB, 64KB, 256KB block sizes and they can be 1D.
            if (flags.prt == 0)
            {
                pOut->validModes.swLinear = 1;
            }

            // We find cases where Tex3d BlockCompressed image adopts SW_256B_R_2D should be prohibited.
            // Same for 3D images w/ view3dAs2dArray==1 that can't use 256B_R_2D.
            if ((IsTex3d(pIn->resourceType) == FALSE) && (flags.prt == 0) && (flags.display == 0))
            {
                pOut->validModes.sw2d256B = 1;
            }
            // Displayable images, on the other hand, support neither 256B nor 4KB swizzle.
            if (flags.display == 0)
            {
                pOut->validModes.sw2d4kB   = 1;
            }
            pOut->validModes.sw2d64kB  = 1;
            pOut->validModes.sw2d256kB = 1;
        }
        else if (IsTex3d(pIn->resourceType))
        {
            // An eventual determination would be based on pal setting of height_watermark and depth_watermark.
            // However, we just adopt the simpler logic currently.
            // For 3D images w/ view3dAs2dArray = 0, SW_3D is preferred.
            // For 3D images w/ view3dAs2dArray = 1, it should go to 2D path above.

            // 3D PRTs can't use linear swizzle mode
            if (flags.prt == 0)
            {
                // Enable linear since client may force linear tiling for 3D texture that does not set view3dAs2dArray.
                pOut->validModes.swLinear  = 1;
            }
            pOut->validModes.sw3d4kB   = 1;
            pOut->validModes.sw3d64kB  = 1;
            pOut->validModes.sw3d256kB = 1;
        }
    }

    // If client specifies a max alignment, remove swizzles that require alignment beyond it.
    if (pIn->maxAlign != 0)
    {
        if (pIn->maxAlign < Size256K)
        {
            pOut->validModes.value &= ~Blk256KBSwModeMask;
        }

        if (pIn->maxAlign < Size64K)
        {
            pOut->validModes.value &= ~Blk64KBSwModeMask;
        }

        if (pIn->maxAlign < Size4K)
        {
            pOut->validModes.value &= ~Blk4KBSwModeMask;
        }

        if (pIn->maxAlign < Size256)
        {
            pOut->validModes.value &= ~Blk256BSwModeMask;
        }
    }

    return returnCode;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlComputeStereoInfo
*
*   @brief
*       Compute height alignment and right eye pipeBankXor for stereo surface
*
*   @return
*       Error code
*
************************************************************************************************************************
*/
ADDR_E_RETURNCODE Gfx13Lib::HwlComputeStereoInfo(
    const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn,        ///< Compute surface info
    UINT_32*                                pAlignY,    ///< Stereo requested additional alignment in Y
    UINT_32*                                pRightXor   ///< Right eye xor
    ) const
{
    ADDR_E_RETURNCODE ret = ADDR_OK;

    *pRightXor = 0;

    const UINT_32 elemLog2    = Log2(pIn->bpp >> 3);
    const UINT_32 samplesLog2 = Log2(pIn->numSamples);
    const UINT_32 eqIndex     = GetEquationTableEntry(pIn->swizzleMode, samplesLog2, elemLog2);

    if (eqIndex != ADDR_INVALID_EQUATION_INDEX)
    {
        const ADDR3_EQUATION*  pEquation   = m_pEquationTable[eqIndex];
        const UINT_32          blkSizeLog2 = GetBlockSizeLog2(pIn->swizzleMode);

        UINT_32 yMax     = 0;
        UINT_32 yPosMask = 0;

        // First get "max y bit"
        for (UINT_32 bitIdx = PipeInterleaveLog2; bitIdx < blkSizeLog2; bitIdx++)
        {
            const auto*  pBit = &pEquation->bits[bitIdx];

            for (UINT_32  termIdx = 0; termIdx < pBit->numTerms; termIdx++)
            {
                const auto*  pTerm = &pBit->term[termIdx];

                if ((pTerm->fields.channel == ADDR3_EQ_CHANNEL::_Y) &&
                    (pTerm->fields.ordinal > yMax))
                {
                    yMax = pTerm->fields.ordinal;
                }
            }
        }

        // Then loop again for populating a position mask of "max Y bit"
        for (UINT_32 bitIdx = PipeInterleaveLog2; bitIdx < blkSizeLog2; bitIdx++)
        {
            const auto*  pBit = &pEquation->bits[bitIdx];

            for (UINT_32  termIdx = 0; termIdx < pBit->numTerms; termIdx++)
            {
                const auto*  pTerm = &pBit->term[termIdx];

                if ((pTerm->fields.channel == ADDR3_EQ_CHANNEL::_Y) &&
                    (pTerm->fields.ordinal == yMax))
                {
                    yPosMask |= 1u << bitIdx;
                }
            }
        }

        const UINT_32 additionalAlign = 1 << yMax;

        if (additionalAlign >= *pAlignY)
        {
            *pAlignY = additionalAlign;

            const UINT_32 alignedHeight = PowTwoAlign(pIn->height, additionalAlign);

            if ((alignedHeight >> yMax) & 1)
            {
                *pRightXor = yPosMask >> PipeInterleaveLog2;
            }
        }
    }
    else
    {
        ret = ADDR_INVALIDPARAMS;
    }

    return ret;
}

/**
************************************************************************************************************************
*   Gfx13Lib::HwlValidateNonSwModeParams
*
*   @brief
*       Validate compute surface info params except swizzle mode
*
*   @return
*       TRUE if parameters are valid, FALSE otherwise
************************************************************************************************************************
*/
BOOL_32 Gfx13Lib::HwlValidateNonSwModeParams(
    const ADDR3_GET_POSSIBLE_SWIZZLE_MODE_INPUT* pIn
    ) const
{
    const AddrResourceType    rsrcType  = pIn->resourceType;
    const UINT_32             bpp       = pIn->bpp;
    const BOOL_32             isMipmap  = (pIn->numMipLevels > 1);
    const BOOL_32             isMsaa    = (pIn->numSamples > 1);
    const ADDR3_SURFACE_FLAGS flags     = pIn->flags;
    const BOOL_32             isDisplay = flags.display;
    const BOOL_32             isStereo  = flags.qbStereo;
    const BOOL_32             isFmask   = flags.fmask;
    const BOOL_32             isPrt     = flags.prt;
    const BOOL_32             isDepth   = flags.depth || flags.stencil;

    BOOL_32                   valid     = TRUE;
    if ((bpp == 0) || (bpp > 128) || (pIn->width == 0) || (pIn->numSamples > 8))
    {
        ADDR_ASSERT_ALWAYS();
        valid = FALSE;
    }

    // Resource type check
    if (IsTex1d(rsrcType))
    {
        if (isMsaa || isStereo || isFmask || isDisplay)
        {
            ADDR_ASSERT_ALWAYS();
            valid = FALSE;
        }
    }
    else if (IsTex2d(rsrcType))
    {
        if ((isMsaa && isMipmap) || (isStereo && isMsaa) || (isStereo && isMipmap) ||
            // DRV (display) must be 2D texture and it can't have MSAA
            (isDisplay && isMsaa) ||
            // It's an issue if addrlib checks depth PRTs while PAL says they're not supported
            (isDepth && isPrt))
        {
            ADDR_ASSERT_ALWAYS();
            valid = FALSE;
        }
    }
    else if (IsTex3d(rsrcType))
    {
        if (isMsaa || isStereo || isFmask || isDisplay)
        {
            ADDR_ASSERT_ALWAYS();
            valid = FALSE;
        }
    }
    else
    {
        // An invalid resource type that is not 1D, 2D or 3D.
        ADDR_ASSERT_ALWAYS();
        valid = FALSE;
    }

    return valid;
}

} // V3
} // Addr
} // rocr