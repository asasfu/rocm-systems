// Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT

#include "addr_shared.h"
#include <stdio.h>

#ifndef KAMAL_STANDALONE
namespace GFX13_METADATA_REFERENCE_MODEL {
#endif

#ifndef ADDR_SHARED
// Uncomment the line below to use Kamal's fix for VAR modes
#define USE_VAR_MODE_FIX

#include "features/address_features.h"

extern bool  isXBias(addr_params& p);
#endif

// sort of log base 2...
int64
addrLog2 (int64 num)
{
    int64 index = 0;
    while (num > 1) {
        index++;
        num = (num+1) >> 1;
    }
    return index;
}

void
getS3Start(
           // Inputs
           int32        position,
           const addr_params& p,

           // Outputs
           int32*       pX,
           int32*       pY,
           int32*       pZ
           )
{
    int32 base = (position / 3) - (p.bpp_log2 / 3);
    *pX = base;
    *pY = base;
    *pZ = base;
    if (position   % 3 > 0) (*pX)++;
    if (position   % 3 > 1) (*pZ)++;
    if (p.bpp_log2 % 3 > 0) (*pX)--;
    if (p.bpp_log2 % 3 > 1) (*pZ)--;
}

// Calculate the block size given the params and the block size log2
void
calcBlockSizeLog2(
              // Inputs
              const addr_params& p,
              int32        block_size_log2,

              // Outputs
              int32*       pWidth,
              int32*       pHeight,
              int32*       pDepth
              )
{
#ifndef ADDR_SHARED
    int block_bits;
#endif
    switch (p.sw)
    {
    case SW_L:
        *pWidth  = block_size_log2 - p.bpp_log2;
        *pHeight = 0;
        *pDepth  = 0;
        break;
#ifndef ADDR_SHARED
    case SW_S:
    case SW_R:
    case SW_Z:
    case SW_D:
        *pWidth  = (block_size_log2 >> 1) - (p.bpp_log2 >> 1) - (p.num_samples_log2 >> 1) - (p.num_samples_log2 & 1);
        *pHeight = (block_size_log2 >> 1) - (p.bpp_log2 >> 1) - (p.bpp_log2 & 1)          - (p.num_samples_log2 >> 1);
        *pDepth = 0;
        if(block_size_log2 & 1)
        {
            // Odd block sizes need to add 1 to either width or height
            if((*pHeight) < (*pWidth)) (*pHeight)++;
            else (*pWidth)++;
        }
        break;
    case SW_S3:
        getS3Start(block_size_log2, p, pWidth, pHeight, pDepth);
        break;
    case SW_D3:
    case SW_R3:
        block_bits = (block_size_log2 - p.bpp_log2);
        *pWidth  = (block_bits / 3) + (((block_bits % 3) > 0) ? 1 : 0);
        *pHeight = (block_bits / 3) + (((block_bits % 3) > 1) ? 1 : 0);
        *pDepth  = (block_bits / 3);
        break;
#endif
    case SW_D_2D:
    case SW_Z_2D:
        *pWidth  = (block_size_log2 >> 1) - (p.bpp_log2 >> 1) - (p.num_samples_log2 >> 1) -  (p.bpp_log2 & p.num_samples_log2  & 1);
        *pHeight = (block_size_log2 >> 1) - (p.bpp_log2 >> 1) - (p.num_samples_log2 >> 1) - ((p.bpp_log2 | p.num_samples_log2) & 1);
        *pDepth  = 0;
        break;
    case SW_S_3D:
        getS3Start(block_size_log2, p, pWidth, pHeight, pDepth);
        break;
    }
}

// Return W,H,D block sizes with a min of 256
// Will return 256 for width in Linear
void
getBlockSizeSlice(
                  // Inputs
                  const addr_params& p,

                  // Outputs
                  int32*       pWidth,
                  int32*       pHeight,
                  int32*       pDepth
                  )
{
    int32 block_size_log2 = p.getSliceBlockSizeLog2();
    calcBlockSizeLog2(p, block_size_log2, pWidth, pHeight, pDepth);
}

// Return W,H,D block sizes
// Will return 128 for width in Linear
// For GFX10, will return width = 256 for linear
void
getBlockSizePitch(
                  // Inputs
                  const addr_params& p,

                  // Outputs
                  int32*       pWidth,
                  int32*       pHeight,
                  int32*       pDepth
                  )
{
    int32 block_size_log2 = p.getPitchBlockSizeLog2();
#ifndef ADDR_SHARED
    if (p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10)
        block_size_log2 = p.getSliceBlockSizeLog2();
#endif
    calcBlockSizeLog2(p, block_size_log2, pWidth, pHeight, pDepth);
}

void
getMipInTaleMaxSize(
                    // Inputs
#ifndef ADDR_SHARED
                    addr_params& p,
#else
                    const addr_params& p,
#endif
                    //Outputs
                    int32*       pMax_mip_in_tail_width_log2,
                    int32*       pMax_mip_in_tail_height_log2
                    )
{
    int32 block_size_log2 = p.getSliceBlockSizeLog2();

#ifndef ADDR_SHARED
#ifdef ADDRESS__ADDRLIB_HTILE_STENCIL_Z32__1
    int32 old_bpp = p.bpp_log2;

//-----------------------------------------------------------------------------------------------------------------------------
// For htile, we need to make z16 and stencil enter the mip tail at the same time as z32 would
//-----------------------------------------------------------------------------------------------------------------------------
//    if (p.sw_orig == SW_Z && p.bpp_log2 < 2)      // sw_orig is only used if combining Z and R modes
    if (p.sw == SW_Z && p.bpp_log2 < 2)
        p.bpp_log2 = 2;
#endif
#endif
    int32 max_data_mip_in_tail_width_log2;
    int32 max_data_mip_in_tail_height_log2;
    int32 block_depth_log2;
    getBlockSizeSlice(p,
                      &max_data_mip_in_tail_width_log2,
                      &max_data_mip_in_tail_height_log2,
                      &block_depth_log2);

#ifndef ADDR_SHARED
    int32 max_meta_mip_in_tail_width_log2  = 0;
    int32 max_meta_mip_in_tail_height_log2 = 0;
    int32 meta_block_depth_log2            = 0;
    if (p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10 && p.RB_Plus_Flag == true)   // 10.2 only
    {
        getMetaBlockLog2(p,
                         &max_meta_mip_in_tail_width_log2,
                         &max_meta_mip_in_tail_height_log2,
                         &meta_block_depth_log2);
    }
#endif
#ifdef ADDRESS__ADDRLIB_HTILE_STENCIL_Z32__1
    p.bpp_log2 = old_bpp;
#endif
#ifndef ADDR_SHARED
    if (p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10 && p.RB_Plus_Flag == true && // 10.2 only
        max_meta_mip_in_tail_width_log2 < max_data_mip_in_tail_width_log2)
        (*pMax_mip_in_tail_width_log2) = max_meta_mip_in_tail_width_log2;
    else
#endif
        (*pMax_mip_in_tail_width_log2) = max_data_mip_in_tail_width_log2;

#ifndef ADDR_SHARED
    if (p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10 && p.RB_Plus_Flag == true && // 10.2 only
        max_meta_mip_in_tail_height_log2 < max_data_mip_in_tail_height_log2)
        (*pMax_mip_in_tail_height_log2) = max_meta_mip_in_tail_height_log2;
    else
#endif
        (*pMax_mip_in_tail_height_log2) = max_data_mip_in_tail_height_log2;

//--------------------------------------------------------------------------------------------------------------------------------------------
// This is generalized to handle VAR block sizes
// Since we only care about 64KB or 4KB blocks, we could simplify it to this:
//
// if (p.sw == some 3d mode && block_size_log2 == 12) {
//      (*pMax_mip_in_tail_height_log2)--;
//} else {
//      (*Pmax_mip_in_tail_width_log2)--;
//}
//--------------------------------------------------------------------------------------------------------------------------------------------

    if (p.sw == SW_S_3D
#ifndef ADDR_SHARED
        || p.sw == SW_S3 || p.sw == SW_D3 || p.sw == SW_R3
#endif
        )
    {
        switch (block_size_log2 % 3)
        {
        case 0: (*pMax_mip_in_tail_height_log2)--; break;
        case 1: (*pMax_mip_in_tail_width_log2)--; break;
        case 2:
            // would decrement the depth here,
            // if we didn't have all of the slices to begin with
            (*pMax_mip_in_tail_width_log2)--;
            break;
        }
    }
    else
    {
#if defined(USE_VAR_MODE_FIX)
        (*pMax_mip_in_tail_width_log2)--;
#else
        switch (block_size_log2 % 2)
        {
        case 0: (*pMax_mip_in_tail_width_log2)--; break;
        case 1: (*pMax_mip_in_tail_height_log2)--; break;
        }
#endif
    }
}

int32
shift_ceil(
           // Inputs
           int32 a,
           int32 b
           )
{
    // This is just ceil(a / (2^b))

    const int32 QUOT = (a >> b);
    const int32 MASK = (~(~0 << static_cast<uint32>(b)));
    const int32 ROUND_FACTOR = (((a & MASK) != 0) ? 1 : 0);

    int32 result = QUOT + ROUND_FACTOR;

    return result;
    //return (a >> b) + (((a & ~(~0 << b)) != 0) ? 1 : 0);
}

int32
divide_ceil(
            int32 src,
            int32 div)
{
    int32 remain = (src % div != 0) ? 1 : 0;
    return src / div + remain;
}

void
getMipSize2dCompute(
                    // Inputs
                    const addr_params& p,
                    int32        mip,

                    // Outputs
                    int32*       pWidth,
                    int32*       pHeight
                    )
{
    (*pWidth)  = (p.width <= 0)  ? 1 : p.width;
    (*pHeight) = (p.height <= 0) ? 1 : p.height;

    (*pWidth)  = shift_ceil(*pWidth,  mip);
    (*pHeight) = shift_ceil(*pHeight, mip);
}

int32
addr_params::MIP_CHAIN::Get_Width(
                                  // Input
                                  const int32 mip_id
                                  ) const
{
#if defined(CHECK_MIP_CHAIN_ARRAY_ACCESS)
    if (mip_id > MAX_POSSIBLE_MIP_LEVEL) {
        printf("\nGet_Width( mip_id: %d ) > MAX_POSSIBLE_MIP_LEVEL (%d) width: 0x%x \n\n",
               mip_id, MAX_POSSIBLE_MIP_LEVEL,
               mip_levels_array[mip_id].width);
//        assert( 0 && "mip_id <= MAX_POSSIBLE_MIP_LEVEL");
    }
#endif

    return mip_levels_array[mip_id].width;
}

int32
addr_params::MIP_CHAIN::Get_Height(
                                   // Input
                                   const int32 mip_id
                                   ) const
{

#if defined(CHECK_MIP_CHAIN_ARRAY_ACCESS)
    if (mip_id > MAX_POSSIBLE_MIP_LEVEL) {
        printf("\nGet_Height( mip_id: %d ) > MAX_POSSIBLE_MIP_LEVEL (%d)   height: 0x%x  \n\n",
               mip_id, MAX_POSSIBLE_MIP_LEVEL,
               mip_levels_array[mip_id].height);
//        assert( 0 && "mip_id <= MAX_POSSIBLE_MIP_LEVEL");
    }
#endif

    return mip_levels_array[mip_id].height;
}

//-----------------------------------------------------------------------------
void
getMipSize2d(
             // Inputs
             addr_params& p,
             int32        mip,

             // Outputs
             int32*       pWidth,
             int32*       pHeight
             )
{
    if (p.mip_chain.Get_Dirty_Bit() == true)
    {
        p.mip_chain.Init(p);
    }

    *pWidth  = p.mip_chain.Get_Width(mip);
    *pHeight = p.mip_chain.Get_Height(mip);
}

int32
calc_mip_in_tail(
                 // Inputs
                 const addr_params& p,
                 int32        mipId,
                 int32        first_mip_in_tail
                 )
{
    // Check
    int32 mip_in_tail = mipId - first_mip_in_tail;

#ifndef ADDR_SHARED
    if (p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10)
    {
        if (mip_in_tail < 0) mip_in_tail = TOTAL_MIP_CHAIN_LEVELS_GFX10;
        if (p.maxmip == 0)   mip_in_tail = TOTAL_MIP_CHAIN_LEVELS_GFX10;
// program as not in mip tail for <=256B size data blocks
        if (p.getSliceBlockSizeLog2() <= 8)
        {
            mip_in_tail = TOTAL_MIP_CHAIN_LEVELS_GFX10;
        }
    }
    else
    {
#endif
        if (mip_in_tail < 0) mip_in_tail = TOTAL_MIP_CHAIN_LEVELS_GFX13;
        if (p.maxmip == 0)   mip_in_tail = TOTAL_MIP_CHAIN_LEVELS_GFX13;
// program as not in mip tail for <=256B size data blocks
        if (p.getSliceBlockSizeLog2() <= 8)
        {
            mip_in_tail = TOTAL_MIP_CHAIN_LEVELS_GFX13;
        }
#ifndef ADDR_SHARED
    }
#endif
    return mip_in_tail;
}

//-----------------------------------------------------------------------------
void
getMipOffset(
             // Inputs
             addr_params&  p,
             int32         mip,

             // Outputs
             int64*        pData_offset,
             int64*        pMeta_offset,
             int32*        pMip_in_tail,
             int64*        pData_chain_size,
             int64*        pMeta_chain_size
             )
{
    int32 block_width_log2, block_height_log2, block_depth_log2;

//    int32 block_size_log2 = p.getPitchBlockSizeLog2();
    int32 block_size_log2 = p.getSliceBlockSizeLog2();

    int32 meta_block_width_log2, meta_block_height_log2, meta_block_depth_log2;

    getBlockSizeSlice(p,
                      &block_width_log2,
                      &block_height_log2,
                      &block_depth_log2);

    int32 meta_block_size_log2 = getMetaBlockLog2(p,
                                                  &meta_block_width_log2,
                                                  &meta_block_height_log2,
                                                  &meta_block_depth_log2);

    int32 meta_block_width, meta_block_height, meta_block_depth;
    int64 meta_block_size = getMetaBlockSize(p,
                                             &meta_block_width,
                                             &meta_block_height,
                                             &meta_block_depth);


    int32 mip_width, mip_height;
    //int32 mip_depth;

    int32 mip_block_width[TOTAL_MIP_CHAIN_LEVELS_GFX13]  = { 0 };
    int32 mip_block_height[TOTAL_MIP_CHAIN_LEVELS_GFX13] = { 0 };

    int32 mip_meta_block_width [TOTAL_MIP_CHAIN_LEVELS_GFX13] = { 0 };
    int32 mip_meta_block_height[TOTAL_MIP_CHAIN_LEVELS_GFX13] = { 0 };

    int32 i;
    int32 first_mip_in_tail = p.maxmip;     // Set to maxmip as the default

    int32 num_mips_in_tail = getNumMipsInTail(p);

    int32 max_mip_in_tail_width_log2;
    int32 max_mip_in_tail_height_log2;

    getMipInTaleMaxSize(p,
                        &max_mip_in_tail_width_log2,
                        &max_mip_in_tail_height_log2);

    const int32 MAX_MIP_IN_TAIL_WIDTH_ELEMENTS  = (1 << max_mip_in_tail_width_log2);
    const int32 MAX_MIP_IN_TAIL_HEIGHT_ELEMENTS = (1 << max_mip_in_tail_height_log2);

    int32 LAST_POSSIBLE_MIP_LEVEL = p.getMaxPossibleMipLevel();
    const int32 starting_mip_level = LAST_POSSIBLE_MIP_LEVEL;

    for (i = starting_mip_level; i >= 0; i--)
    {
        getMipSize2d(p,
                     i,
                     &mip_width,
                     &mip_height);  //, mip_depth);  don't use depth

        if ((mip_width <= MAX_MIP_IN_TAIL_WIDTH_ELEMENTS)
            && (mip_height <= MAX_MIP_IN_TAIL_HEIGHT_ELEMENTS)
            && (p.maxmip - i < num_mips_in_tail))
        {
            first_mip_in_tail = i;
        }

        mip_block_width[i]  = shift_ceil(mip_width, block_width_log2);
        mip_block_height[i] = shift_ceil(mip_height, block_height_log2);

        if (p.isGfx13NativeGen() && (p.num_se == 3))
        {
            mip_meta_block_width[i]  = divide_ceil(mip_width, meta_block_width);
            mip_meta_block_height[i] = shift_ceil(mip_height, meta_block_height_log2);
        }
        else
        {
            mip_meta_block_width[i]  = shift_ceil(mip_width, meta_block_width_log2);
            mip_meta_block_height[i] = shift_ceil(mip_height, meta_block_height_log2);
        }
    }

    //if (p.maxmip == 0) {
    //    // Optimization to exit early for non-mip chains
    //    first_mip_in_tail = 1;
    //}

    *pMip_in_tail = calc_mip_in_tail(p, mip, first_mip_in_tail);

    //#if 0
    //    if (*pMip_in_tail != 15) {
    //        bool is_x_bias = isXBias( p );
    //
    //        if (is_x_bias) {
    //            if (mip_block_height[ first_mip_in_tail] > mip_meta_block_height[ first_mip_in_tail] ) {
    //                printf("ERROR: Mip level[*pMip_in_tail] Data block_height 0x%x > meta_mip_block_height 0x%x \n\n\n",
    //                        mip_block_height[ first_mip_in_tail],
    //                        mip_meta_block_height[ first_mip_in_tail]
    //                       );
    //                fflush(NULL);
    //                assert(0 && "X_Bias");
    //            }
    //
    //            if (block_height_log2 >  meta_block_height_log2) {
    //                printf("ERROR: Mip level[%d] Data block_height_log2 0x%x > meta_block_height_log2 0x%x \n",
    //                       mip,
    //                       block_height_log2,
    //                       meta_block_height_log2
    //                       );
    //                fflush(NULL);
    //                //assert(0 && "X_Bias meta_block_log2");
    //        }
    //
    //        }
    //        else {
    //
    //            if ( mip_block_width[first_mip_in_tail] > mip_meta_block_width[first_mip_in_tail]) {
    //                printf("ERROR: Mip level[*pMip_in_tail] Data block_width 0x%x > meta_mip_block_width 0x%x   \n\n\n",
    //                            mip_block_width[ first_mip_in_tail],
    //                            mip_meta_block_width[ first_mip_in_tail]
    //                           );
    //                fflush(NULL);
    //                assert(0 && "Y_Bias");
    //            }
    //
    //            if (block_width_log2 > meta_block_width_log2) {
    //                printf("ERROR: Mip level[%d] Data block_width_log2 0x%x > meta_block_width_log2 0x%x   \n",
    //                       mip,
    //                       block_width_log2,
    //                       meta_block_width_log2
    //                      );
    //                fflush(NULL);
    //                assert(0 && "Y_Bias");
    //        }
    //
    //    }
    //
    //    }
    //#endif

    int64 last_mip_size = 1;
    int32 last_meta_mip_size = 1;

#if 0
#ifndef ADDR_SHARED
    if (1) { //*pMip_in_tail != 15) {

        if (p.maxmip != 0) {

            if (  (  mip_meta_block_width [first_mip_in_tail]
                     * mip_meta_block_height[first_mip_in_tail]
                     ) > 1
                  ) {
                bool is_x_bias = isXBias( p );

                printf("ERROR: Mip level[%d]   x_bias %d    mip_in_tail %d first_in_tail(%d): mip_meta_block_width[ first: %d ] 0x%x  * mip_meta_block_height[ first: %d ] 0x%x  > 1   \n"
                       " MAX_MIP_IN_TAIL_WIDTH_ELEMENTS 0x%x  MAX_MIP_IN_TAIL_HEIGHT_ELEMENTS 0x%x \n",
                       mip,
                       (is_x_bias==true?1:0),
                       *pMip_in_tail,
                       first_mip_in_tail,

                       first_mip_in_tail,
                       mip_meta_block_width [ first_mip_in_tail ],

                       first_mip_in_tail,
                       mip_meta_block_height[ first_mip_in_tail ],

                       MAX_MIP_IN_TAIL_WIDTH_ELEMENTS,
                       MAX_MIP_IN_TAIL_HEIGHT_ELEMENTS
                       );
                fflush(NULL);

                assert(0 && "First_mip_In_tail > 1 metablock");
            }
        }
    }
#endif
#endif
    *pData_offset = 0;
    *pMeta_offset = 0;

    *pData_chain_size = 0;
    *pMeta_chain_size = 0;

    for (i = first_mip_in_tail - 1; i >= -1; i--)
    {
        if (i < p.maxmip)
        {
            if (i >= mip)
            {
                *pData_offset += last_mip_size;
                *pMeta_offset += last_meta_mip_size;
            }
            *pData_chain_size += last_mip_size;
            *pMeta_chain_size += last_meta_mip_size;
        }

        if (i >= 0)
        {
            last_mip_size = 4 * last_mip_size
                - ((mip_block_width[i] & 1) ? mip_block_height[i] : 0)
                - ((mip_block_height[i] & 1) ? mip_block_width[i] : 0)
                - ((mip_block_width[i] & mip_block_height[i] & 1) ? 1 : 0);


            last_meta_mip_size = 4 * last_meta_mip_size
                - ((mip_meta_block_width [i] & 1) ? mip_meta_block_height[i] : 0)
                - ((mip_meta_block_height[i] & 1) ? mip_meta_block_width [i] : 0)
                - ((mip_meta_block_width[i] & mip_meta_block_height[i] & 1) ? 1 : 0);

#ifndef ADDR_SHARED
            assert(last_mip_size >= 0 && "last_mip_size is invalid");
            assert(last_meta_mip_size >= 0 && "last_meta_mip_size is invalid");
#endif
        }
    }

    *pData_offset     = (*pData_offset)     << block_size_log2;
    *pData_chain_size = (*pData_chain_size) << block_size_log2;

    if (p.isGfx13NativeGen() && (p.num_se == 3))
    {
        *pMeta_offset     = (*pMeta_offset) * meta_block_size;
        *pMeta_chain_size = (*pMeta_chain_size) * meta_block_size;
    }
    else
    {
        *pMeta_offset     = (*pMeta_offset)     << meta_block_size_log2;
        *pMeta_chain_size = (*pMeta_chain_size) << meta_block_size_log2;
    }
}

//-----------------------------------------------------------------------------
int32
calc_byte_offset(
                 // Inputs
                 const addr_params& p,
                 int32        mip_in_tail
                 )
{
    int32 byte_offset = 0;

    int32 mips_available = getNumMipsInTail(p);

    // m is mips_in_tail in reverse
    int32 m = mips_available - 1 - mip_in_tail;

    // Clamp to origin if mip_in_tail exceeds mips_available.
    // This is a convernient way to handle non-tail mips,
    // by setting mip_in_tail to a very large value
    if (m < 0) m = 0;

    if (m > 6)
    {
// Over 2KB (16 << 7) offsets: byte offset at every power of 2 over 2KB
        byte_offset = 16 << m;
    }
    else
    {
// Under 2KB: byte offset at every 256 B
        byte_offset = m << 8;
    }

    return byte_offset;
}

//-----------------------------------------------------------------------------
int32
getNumMipsInTail(
                 // Inputs
                 const addr_params& p
                 )
{
    int32 block_size_log2 = p.getSliceBlockSizeLog2();

    int32 effective_block_size_log2 = block_size_log2;
    if (p.sw == SW_S_3D
#ifndef ADDR_SHARED
        || p.sw == SW_S3 || p.sw == SW_D3 || p.sw == SW_R3
#endif
        )
    {
// for 3d tiling modes, we can't usee the z-term for mip-in-tail offset generation.
// This reduces the space available in the block to use for mips within a tail.
// So the effectvie block size is 1/3 less than what it otherwise would be (in 256B units)
        effective_block_size_log2 -= (block_size_log2 - 8) / 3;
    }

//-----------------------------------------------------------------------------------------------------------------------------------
// if the block size is <= 256B, then we have only 1 mip in the tail
// if block size is <= 2KB, then we have 1 mip that takes half the block plus (block_size/2) / 256 mips in the tail
// otherwise, we will have a mip for each power of 2 above 2KB, plus seven (that is for every 256B up to 1536 Bytes)
//-----------------------------------------------------------------------------------------------------------------------------------
    int32 mips_in_tail = (effective_block_size_log2 <= 8) ? 1
        : ((effective_block_size_log2 <= 11) ? 1 + (1 << (effective_block_size_log2 - 9))    // 1 + (block_size_log2 - 1) - 8
           : (effective_block_size_log2 - 11) + 7);
    return mips_in_tail;
}

//-----------------------------------------------------------------------------
void
getMicroBlockSize(
                  // Inputs
                  const addr_params&  p,

                  // Outputs
                  int32*        pWidth,
                  int32*        pHeight,
                  int32*        pDepth
                  )
{
    int32 block_bits;

    switch (p.sw)
    {
    case SW_L:
        *pWidth  = (8 - p.bpp_log2);
        *pHeight = 0;
        *pDepth  = 0;
        break;
#ifndef ADDR_SHARED
    case SW_S:
    case SW_R:
    case SW_Z:
    case SW_D:
    case SW_R3:
        block_bits = (8 - p.bpp_log2);
        if(p.sw == SW_Z) block_bits -= p.num_samples_log2;
        *pWidth = (block_bits >> 1) + (block_bits & 1);
        *pHeight = (block_bits >> 1);
        *pDepth = 0;
        break;
    case SW_S3:
    case SW_D3:
        block_bits = (8 - p.bpp_log2);
        *pWidth  = (block_bits / 3) + (((block_bits % 3) > 1) ? 1 : 0);
        *pHeight = (block_bits / 3);
        *pDepth  = (block_bits / 3) + (((block_bits % 3) > 0) ? 1 : 0);
        break;
#endif
    case SW_D_2D:
    case SW_Z_2D:
        block_bits = (8 - p.bpp_log2);
        *pWidth  = (block_bits >> 1) + (block_bits & 1);
        *pHeight = (block_bits >> 1);
        *pDepth  = 0;
        break;
    case SW_S_3D:
        block_bits = (8 - p.bpp_log2);
        *pWidth  = (block_bits / 3) + (((block_bits % 3) > 1) ? 1 : 0);
        *pHeight = (block_bits / 3);
        *pDepth  = (block_bits / 3) + (((block_bits % 3) > 0) ? 1 : 0);
        break;
    }
}

void
getMipOrigin(
             // Inputs
             const addr_params& p,
             int32        mip_in_tail,

             //Outputs
             int32*       pMip_x,
             int32*       pMip_y,
             int32*       pMip_z
             )
{
    int32 byte_offset = calc_byte_offset(p, mip_in_tail);

    // Initialize
    *pMip_x = 0;
    *pMip_y = 0;
    *pMip_z = 0;

#if defined(USE_VAR_MODE_FIX)
    const int32 BLOCK_SIZE_LOG2 = p.getPitchBlockSizeLog2();
#endif
                               //  8KB  0xb0100                                               54_3210
                               // x=32  0x040  >> 3 = 0010, 0001, 0000_1000   => 0x08     0b0000_1000

                       //V     // 16KB  0xb0100                                               54_3210
    switch (p.sw) {            // y=64  0x040  >> 3 = 0010, 0001, 0000_1000   => 0x08     0b0000_1000

#ifndef ADDR_SHARED
                               // 32KB  0xb0100                                               54_3210
    case SW_S3:                // x=64  0x040  >> 3 = 0010, 0001, 0000_1000   => 0x08     0b0000_1000
    case SW_D3:
    case SW_R3:         //V    // 64KB  0xb1000                                               54_3210        ==============
    case SW_S:                 // y=128 0x080  >> 3 = 0100, 0010, 0001_0000   => 0x10     0b0001_0000        ==============
    case SW_D:
    case SW_R:                 // 128KB  0b000?                                            54_3210
    case SW_Z:                 // ?=???  0x?  >> 3 = 1000, 0100, 0010   => 0x20      0b0010_0000

                         //V   // 256KB  0b0001                                            54_3210
                               // y=256  0x100  >> 3 = 1000, 0100, 0010   => 0x20      0b0010_0000
//                                2
//                                5       6   3   1
//                                6       4   2   6   8   4   2   1
//                                K       K   K   K   K   K   K   K
//                           19  18  17  16  15  14  13  12  11  10   9   8
        // This does the following: {x5, y5, x4, y4, x3, y3, x2, y2, x1, y1, x0, y0} = byte_offset[15:8]
//                       x0                          x1                          x2                          x3                           x4                           x5
        *pMip_x = ((byte_offset >> 9) & 1) | ((byte_offset >> 10) & 2) | ((byte_offset >> 11) & 4) | ((byte_offset >> 12) & 8) | ((byte_offset >> 13) & 16) | ((byte_offset >> 14) & 32);
        *pMip_y = ((byte_offset >> 8) & 1) | ((byte_offset >> 9)  & 2) | ((byte_offset >> 10) & 4) | ((byte_offset >> 11) & 8) | ((byte_offset >> 12) & 16) | ((byte_offset >> 13) & 32);
//                       y0                          y1                          y2                          y3                           y4                           y5

#if defined(USE_VAR_MODE_FIX)
// For odd block sizes swap mip_x/y, in order for it to be x-biased
        if (BLOCK_SIZE_LOG2 & 1)
        {
            int32 ttt = *pMip_x;
            *pMip_x = *pMip_y;
            *pMip_y = ttt;

//-----------------------------------------------------------------------------------------------------
// For odd bpp, the micro block width is twice that of the height.  To compensate for this,
// we need divide mip_x by two, and
// multiply mip_y by 2, and OR in the lsb of mip_x
//-----------------------------------------------------------------------------------------------------
            if (p.bpp_log2 & 1)
            {
                *pMip_y = ((*pMip_y) << 1) | ((*pMip_x) & 1);   // Preserve lsb of mip_x by pushing it into y dimension
                *pMip_x = ((*pMip_x) >> 1);                 // Decrease x dimension to compensate for increase in micro block width of odd BPE
            }
        }
#endif
//        *pMip_z = 0;    // Already initialized
        break;
#endif
    case SW_L:
        *pMip_x = byte_offset >> 8;
//        *pMip_y = 0;    // Already initialized
//        *pMip_z = 0;    // Already initialized
        break;
    case SW_D_2D:
    case SW_Z_2D:
    case SW_S_3D:
//                                2
//                                5       6   3   1
//                                6       4   2   6   8   4   2   1
//                                K       K   K   K   K   K   K   K
//                           19  18  17  16  15  14  13  12  11  10   9   8
// This does the following: {x5, y5, x4, y4, x3, y3, x2, y2, x1, y1, x0, y0} = byte_offset[15:8]
//                       x0                          x1                          x2                          x3                           x4                           x5
        *pMip_x = ((byte_offset >> 9) & 1) | ((byte_offset >> 10) & 2) | ((byte_offset >> 11) & 4) | ((byte_offset >> 12) & 8) | ((byte_offset >> 13) & 16) | ((byte_offset >> 14) & 32);
        *pMip_y = ((byte_offset >> 8) & 1) | ((byte_offset >> 9)  & 2) | ((byte_offset >> 10) & 4) | ((byte_offset >> 11) & 8) | ((byte_offset >> 12) & 16) | ((byte_offset >> 13) & 32);
//                       y0                          y1                          y2                          y3                           y4                           y5
    }

    int32 u_block_width_log2, u_block_height_log2, u_block_depth_log2;

    getMicroBlockSize(p, &u_block_width_log2, &u_block_height_log2, &u_block_depth_log2);

    *pMip_x = (*pMip_x) << u_block_width_log2;
    *pMip_y = (*pMip_y) << u_block_height_log2;
    *pMip_z = (*pMip_z) << u_block_depth_log2;
}

// Calculate the xyz offsets
void
getXYZoffsets(
              // Inputs
              const addr_params& p,
              int32 x,
              int32 y,
              int32 z,
              int32 mip_in_tail,

              // Outputs
              int32* pX_offset,
              int32* pY_offset,
              int32* pZ_offset,
              int32* pX_mip_orig,
              int32* pY_mip_orig,
              int32* pZ_mip_orig
              )
{

    getMipOrigin(p,
                 mip_in_tail,
                 pX_mip_orig, pY_mip_orig, pZ_mip_orig);

    *pX_offset = x + *pX_mip_orig;
    *pY_offset = y + *pY_mip_orig;
    *pZ_offset = z + *pZ_mip_orig;
}

// Calculate the block indexes
// Return false if offset is beyond data block, otherwise true.
bool
getXYZblockIndexes(
                   // Inputs
#ifndef ADDR_SHARED
                   bool check_assert,      // Not used in SW
#endif
                   const addr_params& p,
                   int32 x,
                   int32 y,
                   int32 z,
                   int32 mip_in_tail,
                   int32 pitch_in_elements,
                   int64 slice_in_elements,

                   // Outputs
                   int64* pZ_macro_block_index,
                   int64* pYX_macro_block_index
                   )
{
//--------------------------------------------------------------------------------------
// Get block dimensions in elements
//--------------------------------------------------------------------------------------
    int32 slice_block_width_in_elements,  slice_block_width_in_elements_log2;
    int32 slice_block_height_in_elements, slice_block_height_in_elements_log2;
    int32 slice_block_depth_in_elements,  slice_block_depth_in_elements_log2;

    getBlockSizeSlice(p,
                      &slice_block_width_in_elements_log2,
                      &slice_block_height_in_elements_log2,
                      &slice_block_depth_in_elements_log2
                      );

// Convert block dimensions to log2 elements
    slice_block_width_in_elements  = 1 << slice_block_width_in_elements_log2;
    slice_block_height_in_elements = 1 << slice_block_height_in_elements_log2;
    slice_block_depth_in_elements  = 1 << slice_block_depth_in_elements_log2;

    int32 pitch_block_width_in_elements,  pitch_block_width_in_elements_log2;
    int32 pitch_block_height_in_elements, pitch_block_height_in_elements_log2;
    int32                                 pitch_block_depth_in_elements_log2;

    getBlockSizePitch(p,
                      &pitch_block_width_in_elements_log2,
                      &pitch_block_height_in_elements_log2,
                      &pitch_block_depth_in_elements_log2
                      );

// Convert block dimensions to log2 elements
    pitch_block_width_in_elements  = 1 << pitch_block_width_in_elements_log2;
    pitch_block_height_in_elements = 1 << pitch_block_height_in_elements_log2;

// Calculate the xyz offsets
    int32 x_offset;
    int32 y_offset;
    int32 z_offset;
    int32 x_mip_orig;
    int32 y_mip_orig;
    int32 z_mip_orig;
    getXYZoffsets(p,
                  x,
                  y,
                  z,
                  mip_in_tail,
                  &x_offset,
                  &y_offset,
                  &z_offset,
                  &x_mip_orig,
                  &y_mip_orig,
                  &z_mip_orig);

#ifndef ADDR_SHARED
    if (check_assert)
    {
        int m_mip_not_in_tail_default = 0x11;
        if (p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10 ||
            p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX11) m_mip_not_in_tail_default = 0xf;
        if (mip_in_tail != m_mip_not_in_tail_default)
        {
//-------------------------------------------------------------------------------------------------------
// Inisde mip tail
//-------------------------------------------------------------------------------------------------------
            if ( x_offset >= pitch_block_width_in_elements )
            {
                printf("getZYXblockIndexes: x_offset: 0x%x within miptail is beyond data block width:  0x%x (mip_orig: x,y,z 0x%x 0x%x 0x%x)  mip_in_tail 0x%x Log2_BPE: %d \n",
                       x_offset,
                       pitch_block_width_in_elements,
                       x_mip_orig, y_mip_orig, z_mip_orig,
                       mip_in_tail,
                       p.bpp_log2
                       );
                return false;
            }

            if (y_offset >= slice_block_height_in_elements)
            {
                printf("getZYXblockIndexes: y_offset: 0x%x within miptail is beyond data block height:  0x%x (mip_orig: x,y,z 0x%x 0x%x 0x%x)  mip_in_tail 0x%x Log2_BPE: %d \n",
                       y_offset,
                       slice_block_height_in_elements,
                       x_mip_orig, y_mip_orig, z_mip_orig,
                       mip_in_tail,

                       p.bpp_log2
                       );
                return false;
            }
        }
    }
#endif 
    int64 pitch_in_macro_blocks = (pitch_in_elements / pitch_block_width_in_elements);
    int64 slice_in_macro_blocks = (slice_in_elements / slice_block_height_in_elements) / slice_block_width_in_elements;

    int32 x_block_units = x_offset / pitch_block_width_in_elements;
    int32 y_block_units = y_offset / pitch_block_height_in_elements;
    int32 z_block_units = z_offset / slice_block_depth_in_elements;

// Need to separate for SW_LINEAR
    *pZ_macro_block_index = (slice_in_macro_blocks * z_block_units);

    *pYX_macro_block_index = (pitch_in_macro_blocks * y_block_units)
        + x_block_units;

    return true;
}

//-----------------------------------------------------------------------------
void
getCompressedBlockSize(
                       // Inputs
                       const addr_params& p,

                       //Outputs
                       int32* pWidth,
                       int32* pHeight,
                       int32* pDepth
                       )
{
    switch(p.surf_type) {
    case SURF_COLOR:
        getMicroBlockSize(p, pWidth, pHeight, pDepth);
        break;
    case SURF_DEPTH:
    case SURF_FMASK:
        *pWidth  = 3;
        *pHeight = 3;
        *pDepth  = 0;
        break;
    }
}

//-----------------------------------------------------------------------------
int32
getMetaElementSize(
                   // Inputs
                   const addr_params& p
                   )
{
    switch(p.surf_type) {
    case SURF_COLOR: return  0;
    case SURF_DEPTH: return  2;
    case SURF_FMASK: return -1;
    }
    return 0;
}

//-----------------------------------------------------------------------------
int32
getMetaCachelineSize(
                     // Inputs
                     const addr_params& p
                     )
{
    switch(p.surf_type) {
    case SURF_COLOR: return 6;
    case SURF_DEPTH: return 8;
    case SURF_FMASK: return 8;
    }
    return 0;
}

//-----------------------------------------------------------------------------
int32
getMetaOverlap(
               // Inputs
               const addr_params& p
               )
{
    if(p.pitch_block_size_log2 == 0) return 0;

    int32 comp_block_width_log2, comp_block_height_log2, comp_block_depth_log2, comp_size;
    int32 u_block_width_log2, u_block_height_log2, u_block_depth_log2, u_size;
    getCompressedBlockSize(p, &comp_block_width_log2, &comp_block_height_log2, &comp_block_depth_log2);
    getMicroBlockSize(p, &u_block_width_log2, &u_block_height_log2, &u_block_depth_log2);
    comp_size = comp_block_width_log2 + comp_block_height_log2 + comp_block_depth_log2;
    u_size = u_block_width_log2 + u_block_height_log2 + u_block_depth_log2;
    int32 max_size = (comp_size > u_size) ? comp_size : u_size;
    int32 num_pipes_log2 = p.getEffectiveNumPipes();

    int32 overlap = num_pipes_log2 - max_size;
    if(num_pipes_log2 > 1 && p.pipe_dist == PIPE_DIST_16X16) overlap++;

// In 16Bpp 8xaa, we lose 1 overlap bit because the block size reduction eats into a pipe anchor bit (y4)
    if (p.chip_engine >= ADDR_ASIC_ID_GFX_ENGINE_GFX11) // 11.0
    {
        if(p.bpp_log2 == 4 && p.num_samples_log2 == 3 && p.getPitchBlockSizeLog2() == 16) overlap--;
        overlap += 16 - p.getPitchBlockSizeLog2();
    }
    else
    {
        if(p.bpp_log2 == 4 && p.num_samples_log2 == 3) overlap--;
    }
    if(overlap < 0) overlap = 0;
    return overlap;
}

//-----------------------------------------------------------------------------
int32
get3DMetaOverlap(
                 // Inputs
                 const addr_params& p
                 )
{
    if(p.pitch_block_size_log2 == 0) return 0;

    int32 u_block_width_log2, u_block_height_log2, u_block_depth_log2;
    getMicroBlockSize(p, &u_block_width_log2, &u_block_height_log2, &u_block_depth_log2);
        
    int32 overlap = p.getEffectiveNumPipes() - u_block_width_log2;
    if(p.pipe_dist == PIPE_DIST_16X16)
        overlap++;
    if(overlap < 0 || p.sw == SW_S_3D
#ifndef ADDR_SHARED
       || p.sw == SW_S3
#endif
       ) overlap = 0;

    return overlap;
}

//-----------------------------------------------------------------------------
//  getMetaBlockSize
//      get width/height/depth/block_size with size number
//-----------------------------------------------------------------------------
int64
getMetaBlockSize(const addr_params& p, 
                 int32* pWidth,
                 int32* pHeight,
                 int32* pDepth)
{
    int64 blkSize;
    int32 WidthLog2, HeightLog2, DepthLog2;
    int32 BlockLog2 = getMetaBlockLog2(p, &WidthLog2, &HeightLog2, &DepthLog2);

    if (p.isGfx13NativeGen() && (p.num_se == 3))
    {
        *pWidth  = 768;
        *pHeight = 1 << HeightLog2;
        *pDepth  = 1 << DepthLog2;
        blkSize  = 24 * 1024;
    }
    else
    {
        *pWidth  = 1    << WidthLog2;
        *pHeight = 1    << HeightLog2;
        *pDepth  = 1    << DepthLog2;
        blkSize  = 1ull << BlockLog2;
    }

    return blkSize;
}

//-----------------------------------------------------------------------------
//  getMetaBlockLog2
//      get width/height/depth/block_size with log2 bit number
//-----------------------------------------------------------------------------
int32
getMetaBlockLog2(
                 // Inputs
                 const addr_params& p,

                 // Outputs
                 int32* pWidth,
                 int32* pHeight,
                 int32* pDepth
                 )
{
    int32 block_size = 0;

#ifndef ADDR_SHARED
    int32 block_bits = 0;

    int32 meta_element_size   = getMetaElementSize(p);
    int32 meta_cacheline_size = getMetaCachelineSize(p);

    int32 comp_block_size = (p.surf_type == SURF_COLOR) ? 8 : 6 + p.num_samples_log2 + p.bpp_log2;

    int32 samples_in_meta_block = (
                                   p.sw == SW_Z || p.chip_engine != ADDR_ASIC_ID_GFX_ENGINE_GFX10) ? p.num_samples_log2 : p.getMaxCompFragLog2();

    int32 num_pipes_log2  = p.num_pipes_log2;

    int32 block_size_log2 = p.getPitchBlockSizeLog2();
#endif

    switch(p.sw)
    {
#ifndef ADDR_SHARED
    case SW_L:
        block_size = block_size_log2 - 7;
        *pWidth =  block_size + comp_block_size - p.bpp_log2 - samples_in_meta_block - meta_element_size;
        *pHeight = 0;
        *pDepth = 0;
        break;

    case SW_S:
    case SW_D:
    case SW_R:
    case SW_Z:

        if (!p.pipe_aligned || p.sw == SW_S || p.sw == SW_D )
        {
            if (p.pipe_aligned)
            {
                block_size = p.pipe_interleave_log2 + num_pipes_log2;

                if (block_size < 12) block_size = 12;
                if (block_size > block_size_log2) block_size = block_size_log2;
            }
            else
            {
                block_size = (block_size_log2 < 12) ? block_size_log2 : 12;
            }
        }
        else
        {
            if (p.chip_engine > ADDR_ASIC_ID_GFX_ENGINE_GFX10)  // 10.3
            {
                if (p.RB_Plus_Flag == true)
                {
                    if(p.num_pipes_log2 == p.Get_Num_Sas_Log2()+1 && p.num_pipes_log2 > 1) num_pipes_log2++;
                }
            }
            int32 pipe_rotate_amount = p.getPipeRotateAmount();
            if (num_pipes_log2 >= 4)
            {
                int32 meta_overlap = getMetaOverlap(p);

// In 16Bpe 8xaa, we have an extra overlap bit
                if(pipe_rotate_amount > 0 && p.bpp_log2 == 4 && p.num_samples_log2 == 3 && (
                                                                                            p.sw == SW_Z ||
                                                                                            p.getEffectiveNumPipes() > 3)) meta_overlap++;

                block_size = meta_cacheline_size + meta_overlap + num_pipes_log2;

                if (block_size < p.pipe_interleave_log2 + num_pipes_log2)
                    block_size = p.pipe_interleave_log2 + num_pipes_log2;

                if (p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10)
                {
                    if (p.pipe_dist == PIPE_DIST_16X16
                        && p.sw == SW_R
                        && num_pipes_log2 == 6
                        && p.num_samples_log2 == 3
                        && p.max_comp_frag_log2 == 3
                        && block_size < 15
                        )
                        block_size = 15;
                }
            }
            else
            {
                block_size = p.pipe_interleave_log2 + num_pipes_log2;
                if (block_size < 12)
                    block_size = 12;
            }
            
            if(p.surf_type == SURF_DEPTH)
            {
                // For htile surfaces, pad meta block size to 2K * num_pipes
                if(block_size < 11 + num_pipes_log2)
                    block_size = 11 + num_pipes_log2;
            }

            if (p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10)
            {
                int32 max_comp_frag_log2 = p.getMaxCompFragLog2();
                if (   p.sw == SW_R
                       && max_comp_frag_log2 > 1
                       && pipe_rotate_amount >= 1
                    )
                {
                    int32 new_block_size = 8 + p.num_pipes_log2 + ((pipe_rotate_amount > max_comp_frag_log2-1) ? pipe_rotate_amount
                                                                   : max_comp_frag_log2-1);
                    if (block_size < new_block_size)
                        block_size = new_block_size;
                }
            }
        }
        block_bits = block_size + comp_block_size - p.bpp_log2 - samples_in_meta_block - meta_element_size;

        *pWidth = (block_bits >> 1) + (block_bits & 1);
        *pHeight = (block_bits >> 1);
        *pDepth = 0;
        break;

    case SW_S3:
    case SW_D3:
    case SW_R3:
        if(p.pipe_aligned)
        {
            if (p.chip_engine > ADDR_ASIC_ID_GFX_ENGINE_GFX10)  // 10.3
            {
                if (p.RB_Plus_Flag == true)
                {
                    if(p.num_pipes_log2 == p.Get_Num_Sas_Log2()+1 && p.num_pipes_log2 > 1 && p.isRBAligned()) num_pipes_log2++;
                }
            }
            block_size = meta_cacheline_size + get3DMetaOverlap(p) + num_pipes_log2;
            if(block_size < p.pipe_interleave_log2 + num_pipes_log2) block_size = p.pipe_interleave_log2 + num_pipes_log2;
            if(block_size < 12) block_size = 12;
        }
        else
        {
            block_size = 12;
        }
        block_bits = block_size + comp_block_size - p.bpp_log2 - samples_in_meta_block - meta_element_size;
        *pWidth = (block_bits / 3) + (((block_bits % 3) > 0) ? 1 : 0);
        *pHeight = (block_bits / 3) + (((block_bits % 3) > 1) ? 1 : 0);
        *pDepth = (block_bits / 3);
        break;
#endif
    case SW_S_3D:	// These two need to be here since getMetaBlockLog2 is called for data
    case SW_D_2D:

    case SW_Z_2D:
        *pDepth = 0;
        if (p.num_se == 1)
        {
            *pWidth  = 9;                 // 512
            *pHeight = 8;                 // 256
        }
        if (p.num_se == 2)
        {
            *pWidth  = 9;                 // 512
            *pHeight = 9;                 // 512
        }
        if (p.num_se == 3)
        {
            // se = 3 has Non power of 2 block width
            // handle this outside this func
            // return width with 10 for equation
            *pWidth  = 10;      // 768
            *pHeight = 9;       // 512
        }
#ifdef ADDRESS__BACK_COMPAT_FAMILY_EITHER__0
        // AT2
#endif
        if (p.num_se == 4)      
        {
            *pWidth  = 10;                // 1024
            *pHeight = 9;                 // 512
        }
        if (p.num_se == 5)
        {
            *pWidth  = 1280;              // Non power of 2
            *pHeight = 9;                 // 512
        }
        if (p.num_se == 6)      // AT1
        {
            *pWidth  = 1536;              // Non power of 2
            *pHeight = 9;                 // 512
        }
        if (p.num_se == 12)     // AT0
        {
            *pWidth  = 1536;              // Non power of 2
            *pHeight = 10;                // 1024
        }
        block_size = LOG2_NATIVE_HTILE_BLOCK_SIZE + getNumRbIdBits(p);
        break;
    default:
#ifndef ADDR_SHARED
        printf("\nERROR: getMetaBlockLog2: Unsupported swizzle type: %d \n\n", p.sw);
        assert(0);
#endif
        break;
    }
    return block_size;
}

//-----------------------------------------------------------------------------
unsigned long
getMetaSlice(
             // Inputs
             addr_params& c_model_surface_state,
             int base_width,
             int base_height
             )
{
//    int meta_block_mip_width = 0;
//    int meta_block_mip_height = 0;

    int log2_block_size = 0;

    //**********************************************************************************************************************************
    // Really means get block size and expand SW_VAR to full log2 size and NOT 0x0 (which is used for encoding VAR block)
    //**********************************************************************************************************************************
    // Mip calculate related block size are data block size
    log2_block_size = c_model_surface_state.getPitchBlockSizeLog2();

    int meta_block_width_log2  = 0;
    int meta_block_height_log2 = 0;
    int meta_block_depth_log2  = 0;
    int meta_block_size_log2   = 0;

    meta_block_size_log2 = getMetaBlockLog2(c_model_surface_state,
                                            &meta_block_width_log2,
                                            &meta_block_height_log2,
                                            &meta_block_depth_log2);

    int max_mip_in_tail_width_log2;
    int max_mip_in_tail_height_log2;
    getMipInTaleMaxSize(c_model_surface_state,
                        &max_mip_in_tail_width_log2,
                        &max_mip_in_tail_height_log2);

    int i, first_mip_in_tail = c_model_surface_state.maxmip;

    const int NUM_MIPS_IN_TAIL = getNumMipsInTail(c_model_surface_state);

    for (i = c_model_surface_state.getMaxPossibleMipLevel(); i >= 0; i--) {
        int mip_width   = 0;
        int mip_height  = 0;
        //int mip_depth = 0;

        getMipSize2d(c_model_surface_state, i,
                     &mip_width, &mip_height
                     //,mip_depth
                    );

        if ( (mip_width  <= (1 << (max_mip_in_tail_width_log2))) && 
             (mip_height <= (1 << max_mip_in_tail_height_log2)) && 
             (c_model_surface_state.maxmip - i < NUM_MIPS_IN_TAIL) )
        {
            first_mip_in_tail = i;
        }
    }

    if (log2_block_size == 8) {
        first_mip_in_tail = c_model_surface_state.getMaxPossibleMipLevel();
    }

    long int total_blocks = 0;
    int      last_mip     = (c_model_surface_state.maxmip < first_mip_in_tail) ?
                            c_model_surface_state.maxmip :
                            first_mip_in_tail;

    for (int iter_mip_id = 0;
             iter_mip_id <= last_mip;
             iter_mip_id++
        )
    {
        int32  blocks_this_mip = getHtileNumMetaBlocksPerMipLevel(c_model_surface_state,
                                                                  base_width,
                                                                  base_height,
                                                                  iter_mip_id);

        total_blocks += blocks_this_mip;
    }

//---------------------------------------------------------------------------------------------------------------
// Total meta blocks * meta_block_size_in_elements
//---------------------------------------------------------------------------------------------------------------
    unsigned long meta_slice_in_elements = convertHtileMetaBlocksToElements(c_model_surface_state, total_blocks);

    return meta_slice_in_elements;
}

//-----------------------------------------------------------------------------
unsigned long
convertHtileMetaBlocksToElements(
                                 // Inputs
                                 const addr_params& p,
                                 int32        num_meta_blocks
                                 )
{
    int32 meta_block_width, meta_block_height, meta_block_depth;

    getMetaBlockSize(p,
                     &meta_block_width,
                     &meta_block_height,
                     &meta_block_depth);

    unsigned long meta_slice_in_elements = num_meta_blocks * meta_block_width * meta_block_height;

#ifndef ADDR_SHARED
    if (meta_slice_in_elements <= 0) {
        printf("ERROR: meta_slice_in_elements (%ld) must be > zero. \n\n\n", meta_slice_in_elements);
        fflush(NULL);

        assert(0 && "ERROR: meta_slice_in_elements must be > zero");
    }
#endif

    return meta_slice_in_elements;
}

//-----------------------------------------------------------------------------
unsigned long
convertHtileElementsToBytes(
                            // Inputs
                            const addr_params&   p,
                            unsigned long  elements
                            )
{
    int32 meta_block_width, meta_block_height, meta_block_depth;
    int64 meta_block_size = getMetaBlockSize(p,
                                             &meta_block_width,
                                             &meta_block_height,
                                             &meta_block_depth);

    elements = elements / (meta_block_width * meta_block_height);
    elements = elements * meta_block_size;

    return elements;
}

//-----------------------------------------------------------------------------
unsigned long
getHTileMetaSlice(
                  // Inputs
                  addr_params& c_model_surface_state,
                  int          base_width,
                  int          base_height
                  )
{
    int meta_block_width_log2  = 0;
    int meta_block_height_log2 = 0;
    int meta_block_depth_log2  = 0;
    int meta_block_size_log2   = 0;

    meta_block_size_log2 = getMetaBlockLog2(c_model_surface_state,
                                            &meta_block_width_log2,
                                            &meta_block_height_log2,
                                            &meta_block_depth_log2);


    unsigned long meta_slice_in_elements = getMetaSlice(c_model_surface_state, base_width, base_height);

    return convertHtileElementsToBytes(c_model_surface_state, meta_slice_in_elements);
}

//-----------------------------------------------------------------------------
int32
getHtileNumMetaBlocksPerMipLevel(
                                 // Inputs
                                 addr_params& p,
                                 int          base_width,
                                 int          base_height,
                                 int          mip_level
                                 )
{
    int32 meta_block_width_log2, meta_block_height_log2, meta_block_depth_log2;
    int32 meta_block_width, meta_block_height, meta_block_depth;
    getMetaBlockLog2(p,
                     &meta_block_width_log2, &meta_block_height_log2, &meta_block_depth_log2);

    getMetaBlockSize(p,
                     &meta_block_width,
                     &meta_block_height,
                     &meta_block_depth);

    getMipSize2d(p,
                 mip_level,
                 &base_width, &base_height);

    int meta_block_mip_width;
    int meta_block_mip_height;
    if (p.isGfx13NativeGen() && (p.num_se == 3))
    {
        meta_block_mip_width  = divide_ceil(base_width, meta_block_width);
        meta_block_mip_height = shift_ceil(base_height, meta_block_height_log2);
    }
    else
    {
        meta_block_mip_width  = shift_ceil(base_width,  meta_block_width_log2);
        meta_block_mip_height = shift_ceil(base_height, meta_block_height_log2);
    }

    return meta_block_mip_width * meta_block_mip_height;
}

//-----------------------------------------------------------------------------
int32
getNumRbIdBits(const addr_params& p)
{
    int32 rb_bits;

    switch(p.num_se)
    {
    case 1:
        rb_bits = 2;
        break;
    case 2:
        rb_bits = 3;
        break;
    case 3:
    case 4:
        rb_bits = 4;
        break;
    case 5:
    case 6:
    default:
        rb_bits = 2;
        break;
    }

    return rb_bits;
}

//-----------------------------------------------------------------------------
unsigned long
getHTileMetaSize(
                 // Inputs
                 addr_params& p,
                 int          scaled_width,
                 int          scaled_height,
                 int          depth
                 )
{
    unsigned long slice = getHTileMetaSlice(p, scaled_width, scaled_height);

    return slice * depth;
}

#ifndef KAMAL_STANDALONE    // End of namespace
}
#endif
