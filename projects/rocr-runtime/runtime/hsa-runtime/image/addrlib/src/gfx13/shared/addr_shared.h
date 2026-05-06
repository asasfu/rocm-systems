// Copyright (c) 2016-2025 Advanced Micro Devices, Inc.  All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef __ADDR_SHARED_H
#define __ADDR_SHARED_H

#include <string.h>

#ifndef ADDR_SHARED
#include <cmntypes.h>
#include <assert.h>
#else
#include "stdtypes.h"
#endif

// From addr_interface_chip_engine.h
#define ADDR_ASIC_ID_GFX_ENGINE_GFX10 0x0000000A
#define ADDR_ASIC_ID_GFX_ENGINE_GFX11 0x0000000B
#define ADDR_ASIC_ID_GFX_ENGINE_GFX12 0x0000000C
#define ADDR_ASIC_ID_GFX_ENGINE_GFX13 0x0000000D

#define TOTAL_MIP_CHAIN_LEVELS_GFX13 17
#define MAX_POSSIBLE_MIP_LEVEL_GFX13 (TOTAL_MIP_CHAIN_LEVELS_GFX13 - 1)

#define TOTAL_MIP_CHAIN_LEVELS_GFX10 15
#define MAX_POSSIBLE_MIP_LEVEL_GFX10 (TOTAL_MIP_CHAIN_LEVELS_GFX10 - 1)

#ifndef KAMAL_STANDALONE
namespace GFX13_METADATA_REFERENCE_MODEL {
#endif

  const int32 SW_L    = 0;
  const int32 SW_D_2D = 1;
  const int32 SW_Z_2D = 2;
  const int32 SW_S_3D = 3;

  const int32 SW_S    = 4;
  const int32 SW_D    = 5;
  const int32 SW_Z    = 6;
  const int32 SW_R    = 7;
  const int32 SW_S3   = 8;
  const int32 SW_D3   = 9;
  const int32 SW_R3   = 10;


  const int32 SURF_COLOR = 0;
  const int32 SURF_DEPTH = 1;
  const int32 SURF_FMASK = 2;

  const int32 PIPE_DIST_8X8   = 0;
  const int32 PIPE_DIST_16X16 = 1;

  const int32 LOG2_NATIVE_HTILE_BLOCK_SIZE = 11;

    class MIP_LEVEL
    {
    public:
        MIP_LEVEL(void) { width = 0; height = 0; }

        int32 width;
        int32 height;
    };


    // Forward declaration
    struct addr_params;

    void getMipSize2dCompute(const addr_params& p, int32 mip, int32* pWidth, int32* pHeight);
    void getMipSize2d(addr_params& p, int32 mip, int32* pWidth, int32* pHeight);

    struct addr_params
    {
        addr_params(void)
        {
            int32 len = sizeof(*this);
            memset(static_cast<void*>(this), 0, len);
        }

        class MIP_CHAIN
        {
        public:
            MIP_LEVEL  mip_levels_array[TOTAL_MIP_CHAIN_LEVELS_GFX13];

            MIP_CHAIN(void)
            {
                int32 len = sizeof(*this);
                memset(static_cast<void*>(this), 0, len);

                is_clean = false;
            }

            void Init(addr_params& p)
            {
                int32 w = 0;
                int32 h = 0;

                int32 mip_id;
                int32 total_levels = TOTAL_MIP_CHAIN_LEVELS_GFX13;
#ifndef ADDR_SHARED
                if (p.chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10) total_levels = TOTAL_MIP_CHAIN_LEVELS_GFX10;
#endif
                for (mip_id = 0; mip_id < total_levels; mip_id++) {
                    getMipSize2dCompute(p,
                                        mip_id,
                                        &w,
                                        &h);

                    mip_levels_array[mip_id].width  = w;
                    mip_levels_array[mip_id].height = h;
                }

                Set_Dirty_Bit(false);
            }

            int32 Get_Width (const int32 mip_id) const;
            int32 Get_Height(const int32 mip_id) const;

            bool Get_Dirty_Bit(void) const { return is_clean == false; }
            void       Set_Dirty_Bit(const bool is_dirty_flag) { is_clean = !is_dirty_flag; }

        private:
            bool is_clean;
        };


        MIP_CHAIN    mip_chain;


        //===================================================================================================================
        // RB+ variable defaults
        //===================================================================================================================
        int32 chip_engine               = ADDR_ASIC_ID_GFX_ENGINE_GFX13;
        int32 num_se                    = 0;    // Number shader engines based on variant, native meta only
        bool  RB_Plus_Flag              = true;
        bool  Bank_Xor_Flag             = RB_Plus_Flag;
        bool  Allow_4_Terms_For_D3_Flag = RB_Plus_Flag;
        bool  Allow_Var_Flag            = RB_Plus_Flag;
        bool  Var_Includes_Bank_Flag    = RB_Plus_Flag;

        int32 sw;                       // For GFX11 only R maps to Z
        int32 sw_orig;                  // Unmodified sw type (Not used in gfx13)
        int32 num_pipes_log2;
        bool  bit8_2d_xor;              // Native only
        int32 bpp_log2;
        int32 num_samples_log2;

        int32 pitch_block_size_log2;
        int32 slice_block_size_log2;    // Block size that can't be < 256
        int32 pipe_interleave_log2;

        int32 xor_mode;
        bool  pipe_aligned;

        int32 max_comp_frag_log2;
        int32 surf_type;

        int32 Get_Width(void) const { return width; }
        void  Set_Width(const int32 input_width) { width = input_width; }

        int32 Get_Height(void) const { return height; }
        void  Set_Height(const int32 input_height) { height = input_height; }

        //private:
        int32 width;
        int32 height;

    public:
        int32 depth;
        int32 maxmip;

        int32 pipe_dist;
        int32 num_sas_log2;
        bool  msaa_bank_xor;


        int32 Get_Num_Sas_Log2(void) const
        {
#ifndef ADDR_SHARED
            if (chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX13)
            {
#endif
                return 2;       // Hard coded value ( total_num_packers / 2) for Navi4X: 5 / 2 = 2
#ifndef ADDR_SHARED
            }
            else
            {
                return num_sas_log2;
            }
#endif
        }

        int32 getEffectiveNumPipes() const { return (pipe_dist == PIPE_DIST_8X8 || Get_Num_Sas_Log2() >= num_pipes_log2 - 1) ? num_pipes_log2 : Get_Num_Sas_Log2() + 1; }
        //int32 getNumSasLog2() { return (num_pipes_log2 < 2) ? 0 : num_pipes_log2 -2; }

        int32 getMaxCompFragLog2() const { return (num_samples_log2 < max_comp_frag_log2) ? num_samples_log2 : max_comp_frag_log2; }

        bool isRBAligned() const
        {
#ifndef ADDR_SHARED
          if (RB_Plus_Flag == true)
            return (sw == SW_Z || sw == SW_R || sw == SW_D3) ? true : false;
          else
#endif
            return false;
        }

        int32 getPipeRotateAmount() const
        {
#ifndef ADDR_SHARED
          if (chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10)
          {
            if(pipe_dist == PIPE_DIST_16X16 && num_pipes_log2 > num_sas_log2+1)
            {
              return num_pipes_log2 - (num_sas_log2+1);
            }
            else
            {
              return 0;
            }
          }
          else
          {
#endif
            if (pipe_dist == PIPE_DIST_16X16 && num_pipes_log2 >= Get_Num_Sas_Log2() + 1 && num_pipes_log2 > 1)
            {
                return (num_pipes_log2 == Get_Num_Sas_Log2() + 1 && isRBAligned()) ? 1 : num_pipes_log2 - (Get_Num_Sas_Log2() + 1);
            }
            else
            {
                return 0;
            }
#ifndef ADDR_SHARED
          }
#endif
        }

        int32 getPitchBlockSizeLog2() const
        {
#ifndef ADDR_SHARED
          if(pitch_block_size_log2 != 0)
#endif
            return pitch_block_size_log2;

#ifndef ADDR_SHARED
          if (sw != SW_R && sw != SW_Z && sw != SW_D3) return 16;
          int b;
          if (Var_Includes_Bank_Flag == true) {
            b = 14 + num_pipes_log2;
          } else {
            b = 10 + num_pipes_log2;
          }

#ifdef VAR_NO_OVERLAP
          int b_no_overlap = 8 + num_pipes_log2 + bpp_log2 + num_samples_log2;

          if(getEffectiveNumPipes() > 1 && getPipeRotateAmount() == 0) b_no_overlap++;
          if(b < b_no_overlap) b = b_no_overlap;
#endif
          return b;
#endif
        }

        int32 getSliceBlockSizeLog2() const
        {
#ifndef ADDR_SHARED
          if(slice_block_size_log2 != 0)
#endif
            return slice_block_size_log2;
#ifndef ADDR_SHARED
          if (sw != SW_R && sw != SW_Z && sw != SW_D3) return 16;
          int b;
          if (Var_Includes_Bank_Flag == true) {
            b = 14 + num_pipes_log2;
          } else {
            b = 10 + num_pipes_log2;
          }

#ifdef VAR_NO_OVERLAP
          int b_no_overlap = 8 + num_pipes_log2 + bpp_log2 + num_samples_log2;

          if(getEffectiveNumPipes() > 1 && getPipeRotateAmount() == 0) b_no_overlap++;
          if(b < b_no_overlap) b = b_no_overlap;
#endif
          return b;
#endif
        }

        int32 getMaxPossibleMipLevel() const
        {
            return (chip_engine == ADDR_ASIC_ID_GFX_ENGINE_GFX10) ?
                    MAX_POSSIBLE_MIP_LEVEL_GFX10 : MAX_POSSIBLE_MIP_LEVEL_GFX13;
        }

        bool isGfx13NativeGen() const
        {
            // Some clients uses GFX12/GFX13
            // so it's native if it's not GFX10 BC Gen
            // GFX8 BC Gen should use c_reference for now
            return (chip_engine > ADDR_ASIC_ID_GFX_ENGINE_GFX10);
        }
    };

    int32  divide_ceil(int32 src, int32 div);
    int32  shift_ceil(int32 a, int32 b);

    void getS3Start(int32 position, const addr_params& p, int32* x, int32* y, int32* z);

    void getBlockSizeSlice(const addr_params& p,                      int32* pWidth, int32* pHeight, int32* pDepth);
    void getBlockSizePitch(const addr_params& p,                      int32* pWidth, int32* pHeight, int32* pDepth);
    void calcBlockSizeLog2(const addr_params& p, int32 blockSizeLog2, int32* pWidth, int32* pHeight, int32* pDepth);

    void getMicroBlockSize(const addr_params& p, int32* pWidth, int32* pHeight, int32* pDepth);

    //void getMipSize2d(addr_params& p, int mip, int32& width, int32& height);
    void getMipSize(addr_params& p, int32 mip, int32& width, int32& height, int32& depth);

    int  getNumMipsInTail(const addr_params& p);
    void getMipInTaleMaxSize(
#ifndef ADDR_SHARED
                             addr_params& p,
#else
                             const addr_params& p,
#endif
                             int32* pMax_mip_in_tail_width_log2, int32* pMax_mip_in_tail_height_log2);

    int32 calc_mip_in_tail(const addr_params& p,
                           int32        mipId,
                           int32        first_mip_in_fail);
    void getMipOffset(addr_params& p, int32 mip,
                      int64* pData_offset, int64* pMeta_offset,
                      int32* pMip_in_tail, int64* pData_chain_size, int64* pMeta_chain_size);
    //void getMipOffsetData(addr_params& p, int32 mip, int64& data_offset,
    //                      int32& mip_in_tail, int64& data_chain_size);

    int32 calc_byte_offset(const addr_params& p, int32 mip_in_tail);
    void getMipOrigin(const addr_params& p, int32 mip_in_tail, int32* pMip_x, int32* pMip_y, int32* pMip_z);

    void getXYZoffsets(const addr_params& p, int32 x, int32 y, int32 z, int32 mip_in_tail,
                       int32* pX_offset,   int32* pY_offset,   int32* pZ_offset,
                       int32* pX_mip_orig, int32* pY_mip_orig, int32* pZ_mip_orig);

    bool getXYZblockIndexes(
#ifndef ADDR_SHARED
                            bool check_assert,  // Not used in SW
#endif
                            const addr_params& p,
                            int32 x, int32 y, int32 z, int32 mip_in_tail,
                            int32 pitch_in_elements, int64 slice_in_elements,
                            int64* pZ_macro_block_index, int64* pYX_macro_block_index);


    void  getCompressedBlockSize(const addr_params& p, int32* pWidth, int32* pHeight, int32* pDepth);
    int32 getMetaElementSize(const addr_params& p);
    int32 getMetaCachelineSize(const addr_params& p);
    int32 getMetaOverlap(const addr_params& p);
    int32 get3DMetaOverlap(const addr_params& p);
    int32 getMetaBlockLog2(const addr_params& p, int32* pWidth, int32* pHeight, int32* pDepth);
    int64 getMetaBlockSize(const addr_params& p, int32* pWidth, int32* pHeight, int32* pDepth);
    unsigned long getMetaSlice(addr_params& c_model_surface_state, int base_width, int base_height);
    unsigned long getHTileMetaSlice(addr_params& c_model_surface_state, int base_width, int base_height);
    unsigned long getHTileMetaSize(addr_params& p, int scaled_width, int scaled_height, int depth);

    int32 getHtileNumMetaBlocksPerMipLevel(addr_params& p,
                                           int          base_width,
                                           int          base_height,
                                           int          mip_level);

    unsigned long convertHtileMetaBlocksToElements(const addr_params& p,
                                                   int32        num_meta_blocks);
    unsigned long convertHtileElementsToBytes(const addr_params&   p,
                                              unsigned long  elements);

    int32 getNumRbIdBits(const addr_params& p);

#ifndef KAMAL_STANDALONE
}
#endif

#endif
