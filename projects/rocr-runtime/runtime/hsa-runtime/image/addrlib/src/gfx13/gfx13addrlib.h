/*
************************************************************************************************************************
*
*  Copyright (C) 2024-2025 Advanced Micro Devices, Inc.  All rights reserved.
*  SPDX-License-Identifier: MIT
*
***********************************************************************************************************************/

/**
************************************************************************************************************************
* @file  gfx13addrlib.h
* @brief Contains the Gfx13Lib class definition.
************************************************************************************************************************
*/

#ifndef __GFX13_ADDR_LIB_H__
#define __GFX13_ADDR_LIB_H__

#include "addrlib3.h"
#include "coord.h"
#include "gfx13_gb_reg.h"
#include "gfx13/shared/addr_shared.h"
#include "gfx13ImageSwizzlePattern.h"

using namespace GFX13_METADATA_REFERENCE_MODEL;

namespace rocr {
namespace Addr
{
namespace V3
{

/**
************************************************************************************************************************
* @brief This class is the GFX13 specific address library
*        function set.
************************************************************************************************************************
*/
class Gfx13Lib : public Lib
{
public:
    /// Creates Gfx13Lib object
    static Addr::Lib* CreateObj(const Client* pClient)
    {
        VOID* pMem = Object::ClientAlloc(sizeof(Gfx13Lib), pClient);
        return (pMem != NULL) ? new (pMem) Gfx13Lib(pClient) : NULL;
    }

protected:
    Gfx13Lib(const Client* pClient);
    virtual ~Gfx13Lib();

    ADDR_E_RETURNCODE HwlComputeStereoInfo(
        const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn,
        UINT_32*                                pAlignY,
        UINT_32*                                pRightXor) const override final;

    // Meta surfaces such as Hi-S/Z are essentially images on GFX13, so just return the max
    // image alignment.
    UINT_32 HwlComputeMaxMetaBaseAlignments() const override final { return 256 * 1024; }

    UINT_32 GetMaxNumMipsInTail(
        const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn) const;

    BOOL_32 IsZSwizzle(Addr3SwizzleMode  swizzleMode) const
        { return ((swizzleMode == ADDR3_64KB_2D_Z) || (swizzleMode == ADDR3_256KB_2D_Z)); }

    BOOL_32 IsInMipTail(
        const ADDR_EXTENT3D&  mipTailDim,       ///< The output of GetMipTailDim() function which is dimensions of the
                                                ///  largest mip level in the tail (again, only 4kb/64kb/256kb block).
        const ADDR_EXTENT3D&  mipDims,          ///< The dimensions of the mip level being queried now.
        INT_32                maxNumMipsInTail, ///< The output of GetMaxNumMipsInTail() function which is the maximal
                                                ///  number of the mip levels that could fit in the tail of larger
                                                ///  block.
        INT_32                numMipsToTheEnd   ///< This is (numMipLevels - mipIdx) and it may be negative when called
                                                ///  in SanityCheckSurfSize() since mipIdx has to be in [0, 16].
        ) const
    {
        BOOL_32 inTail = ((mipDims.width   <= mipTailDim.width)  &&
                          (mipDims.height  <= mipTailDim.height) &&
                          (numMipsToTheEnd <= maxNumMipsInTail));

        return inTail;
    }

    ADDR_E_RETURNCODE HwlComputeSurfaceAddrFromCoordLinear(
        const ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
        const ADDR3_COMPUTE_SURFACE_INFO_INPUT*          pSurfInfoIn,
        ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut) const override final;

    ADDR_E_RETURNCODE HwlComputeSurfaceAddrFromCoordTiled(
        const ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
        ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT*      pOut) const override final;

    ADDR_E_RETURNCODE HwlCopyMemToSurface(
        const ADDR3_COPY_MEMSURFACE_INPUT*  pIn,
        const ADDR3_COPY_MEMSURFACE_REGION* pRegions,
        UINT_32                             regionCount) const override final;

    ADDR_E_RETURNCODE HwlCopySurfaceToMem(
        const ADDR3_COPY_MEMSURFACE_INPUT*  pIn,
        const ADDR3_COPY_MEMSURFACE_REGION* pRegions,
        UINT_32                             regionCount) const override final;

    ADDR_E_RETURNCODE HwlComputeNonBlockCompressedView(
        const ADDR3_COMPUTE_NONBLOCKCOMPRESSEDVIEW_INPUT* pIn,
        ADDR3_COMPUTE_NONBLOCKCOMPRESSEDVIEW_OUTPUT*      pOut) const override final;

    VOID HwlComputeSubResourceOffsetForSwizzlePattern(
        const ADDR3_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_INPUT* pIn,
        ADDR3_COMPUTE_SUBRESOURCE_OFFSET_FORSWIZZLEPATTERN_OUTPUT*      pOut) const override final;

    ADDR_E_RETURNCODE HwlComputeSlicePipeBankXor(
        const ADDR3_COMPUTE_SLICE_PIPEBANKXOR_INPUT* pIn,
        ADDR3_COMPUTE_SLICE_PIPEBANKXOR_OUTPUT*      pOut) const override final;

    BOOL_32 HwlValidateNonSwModeParams(const ADDR3_GET_POSSIBLE_SWIZZLE_MODE_INPUT* pIn) const override final;

    ADDR_E_RETURNCODE HwlGetPossibleSwizzleModes(
        const ADDR3_GET_POSSIBLE_SWIZZLE_MODE_INPUT*   pIn,
        ADDR3_GET_POSSIBLE_SWIZZLE_MODE_OUTPUT*        pOut) const override final;

    UINT_32 HwlGetEquationIndex(
        const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn) const override final;

    UINT_32 HwlGetEquationTableInfo(const ADDR_EQUATION** ppEquationTable) const override final
    {
        *ppEquationTable = this->m_legacyEquationTable;

        return m_numEquations;
    }

    ChipFamily HwlConvertChipFamily(UINT_32 uChipFamily, UINT_32 uChipRevision) override final;

    VOID HwlCalcBlockSize(
        const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
        ADDR_EXTENT3D* pExtent) const override final;

    ADDR_EXTENT3D HwlGetMipInTailMaxSize(
        const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
        const ADDR_EXTENT3D&                           blockDims) const override final;

    ADDR_E_RETURNCODE HwlComputeHtileInfo(
        const ADDR3_COMPUTE_HTILE_INFO_INPUT* pIn,
        ADDR3_COMPUTE_HTILE_INFO_OUTPUT*      pOut) const override final;

    ADDR_E_RETURNCODE HwlComputeFmaskInfo(
        const ADDR3_COMPUTE_FMASK_INFO_INPUT* pIn,
        ADDR3_COMPUTE_FMASK_INFO_OUTPUT*      pOut) const override final;

    UINT_32 GetNumSe(UINT_32 chipRevision) const;

private:
    static const SwizzleModeFlags SwizzleModeTable[ADDR3_MAX_TYPE];

    // The "BIT8_2D_XOR" field from the input gb_addr_config register.  Note that this is the only
    // field in the register that is meaningful to the HW; all other fields are deprecated.
    UINT_32  m_bitEight2dXor;

    // "Legacy" equation table...  
    ADDR_EQUATION         m_legacyEquationTable[Gfx13NumImageEquations];

    // Actual tables are defined in gfx13ImageSwizzlePattern.h...  These are just pointers to those.
    const ADDR3_EQUATION* m_pEquationTable[Gfx13NumImageEquations];


    VOID ConvertToLegacyEquation(
        const ADDR3_EQUATION*  pEquation,
        ADDR_EQUATION*         pLegacyEquation) const;

    UINT_32 GetBlockSizeIndex(Addr3SwizzleMode  swMode) const;
    UINT_32 GetSwizzleTypeIndex(Addr3SwizzleMode  swMode) const;

    UINT_32 GetNumSupportedMsaaRates(Addr3SwizzleMode  swMode) const;

    ADDR_E_RETURNCODE ConvertEqBitToSetting(
        const ADDR3_EQ_BIT&  inputBit,
        ADDR_BIT_SETTING*    pOutputBit) const;

    ADDR_E_RETURNCODE ConvertEquationToBitSetting(
        const ADDR3_EQUATION*  pEquation,
        BOOL_32                bit8Xor,
        ADDR_BIT_SETTING*      pBitSetting) const;

    /**
    ************************************************************************************************************************
    * @brief Bitmasks for swizzle mode determination on GFX13
    ************************************************************************************************************************
    */
    static const UINT_32 Blk256KBSwModeMask = (1u << ADDR3_256KB_2D)   |
                                              (1u << ADDR3_256KB_2D_Z) |
                                              (1u << ADDR3_256KB_3D);
    static const UINT_32 Blk64KBSwModeMask  = (1u << ADDR3_64KB_2D)   |
                                              (1u << ADDR3_64KB_2D_Z) |
                                              (1u << ADDR3_64KB_3D);
    static const UINT_32 Blk4KBSwModeMask   = (1u << ADDR3_4KB_2D)    |
                                              (1u << ADDR3_4KB_3D);
    static const UINT_32 Blk256BSwModeMask  = (1u << ADDR3_256B_2D);

    static const UINT_32 MaxImageDim  = 65536; // Max image size is 64k
    static const UINT_32 MaxMipLevels = 17;

    BOOL_32 HwlInitGlobalParams(const ADDR_CREATE_INPUT* pCreateIn) override final;

    void SanityCheckSurfSize(
        const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT*   pIn,
        const ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*         pOut) const;

    // Initialize equation table
    VOID InitEquationTable();

    // Initialize block dimension table
    VOID InitBlockDimensionTable();

    BOOL_32 HasBit8Xor(
        Addr3SwizzleMode swMode) const;

    const ADDR3_EQUATION* GetEquation(
        UINT_32                elemLog2,
        Addr3SwizzleMode       swMode,
        UINT_32                msaaIdx,
        BOOL_32                isMeta) const;

    ADDR_EXTENT3D GetBaseMipExtents(
        const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn) const;

    INT_32 HwlCalcMipInTail(
        const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT*  pIn,
        const ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*        pOut,
        UINT_32                                         mipLevel) const;

    UINT_32 HwlCalcMipOffset(
        const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
        UINT_32                                        mipInTail) const;

    ADDR_E_RETURNCODE HwlComputeSurfaceInfo(
         const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn,
         ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*      pOut) const override final;

    static ADDR_EXTENT3D GetMipExtent(
        const ADDR_EXTENT3D&  mip0,
        UINT_32               mipId)
    {
        return {
            ShiftCeil(Max(mip0.width, 1u),  mipId),
            ShiftCeil(Max(mip0.height, 1u), mipId),
            ShiftCeil(Max(mip0.depth, 1u),  mipId)
        };
    }

    VOID GetMipOffset(
         const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
         ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*             pOut) const;

    VOID GetMipOrigin(
         const ADDR3_COMPUTE_SURFACE_INFO_PARAMS_INPUT* pIn,
         const ADDR_EXTENT3D&                           mipExtentFirstInTail,
         ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*             pOut) const;

    VOID ConvertSurfInfoToAddrParams(
        const ADDR3_COMPUTE_SURFACE_INFO_INPUT* pIn,
        addr_params*                            pOut,
        BOOL_32                                 fullUpdate = TRUE) const;

    VOID ConvertHtileInfoToAddrParams(
        const ADDR3_COMPUTE_HTILE_INFO_INPUT* pIn,
        addr_params*                          pOut) const;
    
    VOID ComputeHtilePerMipInfo(
        const ADDR3_COMPUTE_HTILE_INFO_INPUT* pIn,
        ADDR3_COMPUTE_HTILE_INFO_OUTPUT*      pOut) const;

    // This is the index of the smallest mip level, not the number of miplevels and it's meant to be populated to
    // addr_params structure
    INT_32 GetMaxMipNumber(UINT_32 numMipLevels) const
    {
        return Max(INT_32(numMipLevels - 1), 0);
    }

    VOID HwlGetMipOrigin(
        void*        pAddrParams,
        UINT_32      mipInTail,
        ADDR3_COORD* pCoord) const;

    ADDR_EXTENT3D HwlGetMicroBlockSize(
        void*        pAddrParams) const;

    INT_64 HwlGetMipOffset(
        void*    pvAddrParams,
        INT_32   mipId,
        INT_32*  pMipInTail) const;

    VOID HwlGetXyzBlockIndices(
        const ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
        addr_params*                                     pAddrParams,
        const ADDR3_COMPUTE_SURFACE_INFO_OUTPUT*         pComputeSurfOut,
        INT_32                                           mipInTail,
        UINT_32*                                         pYxMacroBlockIndex,
        UINT_32*                                         pZmacroBlockIndex) const;

    VOID HwlGetXyzOffsets(
        const ADDR3_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT* pIn,
        addr_params*                                     pAddrParams,
        INT_32                                           mipInTail,
        ADDR3_COORD*                                     pCoord) const;

    UINT_32  GetChannelValue(
        ADDR3_EQ_CHANNEL    channel,
        const ADDR3_COORD&  coord,
        UINT_32             s
        ) const;

    UINT_32 ComputeOffsetFromEquation(
        const ADDR3_EQUATION* pEq,   ///< Equation
        const ADDR3_COORD&    coord,
        UINT_32               s      ///< MSAA sample index
        ) const;
};

} // V3
} // Addr
} // rocr
#endif
