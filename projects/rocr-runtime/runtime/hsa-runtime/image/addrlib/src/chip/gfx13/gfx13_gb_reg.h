/*
************************************************************************************************************************
*
*  Copyright (C) 2024-2025 Advanced Micro Devices, Inc.  All rights reserved.
*  SPDX-License-Identifier: MIT
*
***********************************************************************************************************************/

/**
************************************************************************************************************************
* @file  gfx13_gb_reg.h
* @brief GB_ADDR_CONFIG for gfx13.
************************************************************************************************************************
*/

#if !defined (__GFX13_GB_REG_H__)
#define __GFX13_GB_REG_H__

//
// Make sure the necessary endian defines are there.
//
#if defined(LITTLEENDIAN_CPU)
#elif defined(BIGENDIAN_CPU)
#else
#error "BIGENDIAN_CPU or LITTLEENDIAN_CPU must be defined"
#endif

union GB_ADDR_CONFIG_GFX13 {
    struct {
#if defined(LITTLEENDIAN_CPU)
        uint32_t                       NUM_PIPES : 3;
        uint32_t            PIPE_INTERLEAVE_SIZE : 3;
        uint32_t            MAX_COMPRESSED_FRAGS : 2;
        uint32_t                        NUM_PKRS : 3;
        uint32_t                                 : 1;
        uint32_t                     BIT8_2D_XOR : 1;
        uint32_t                                 : 6;
        uint32_t              NUM_SHADER_ENGINES : 4;
        uint32_t                                 : 3;
        uint32_t                   NUM_RB_PER_SE : 2;
        uint32_t                                 : 4;
#elif defined(BIGENDIAN_CPU)
        uint32_t                                 : 4;
        uint32_t                   NUM_RB_PER_SE : 2;
        uint32_t                                 : 3;
        uint32_t              NUM_SHADER_ENGINES : 4;
        uint32_t                                 : 6;
        uint32_t                     BIT8_2D_XOR : 1;
        uint32_t                                 : 1;
        uint32_t                        NUM_PKRS : 3;
        uint32_t            MAX_COMPRESSED_FRAGS : 2;
        uint32_t            PIPE_INTERLEAVE_SIZE : 3;
        uint32_t                       NUM_PIPES : 3;
#endif
    } bitfields, bits;
    uint32_t    u32All;
    int32_t     i32All;
    float       f32All;
};

#endif
