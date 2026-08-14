/*
 * Copyright © Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef EXT_IMAGE_RESOURCE_GFX13_H_
#define EXT_IMAGE_RESOURCE_GFX13_H_

#if defined(LITTLEENDIAN_CPU)
#elif defined(BIGENDIAN_CPU)
#else
#error "BIGENDIAN_CPU or LITTLEENDIAN_CPU must be defined"
#endif

namespace rocr {
namespace image {

/*** Buffer Resource Descriptor ***/

// Total 160 bits -- 5 dwords

// Bits 0-31
union SQ_BUF_RSRC_WORD0 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int BASE_ADDRESS : 32;
#elif defined(BIGENDIAN_CPU)
    unsigned int BASE_ADDRESS : 32;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 32-63
union SQ_BUF_RSRC_WORD1 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int BASE_ADDRESS_HI :16;
    unsigned int                 : 9;
    unsigned int NUM_RECORDS_1   : 7;
#elif defined(BIGENDIAN_CPU)
    unsigned int NUM_RECORDS_1   : 7;
    unsigned int                 : 9;
    unsigned int BASE_ADDRESS_HI :16;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 64-95
union SQ_BUF_RSRC_WORD2 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int NUM_RECORDS_2 : 32;
#elif defined(BIGENDIAN_CPU)
    unsigned int NUM_RECORDS_2 : 32;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 96-127
union SQ_BUF_RSRC_WORD3 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int NUM_RECORDS_3  : 6;
    unsigned int                : 6;
    unsigned int STRIDE         :14;
    unsigned int STRIDE_SCALE   : 2;
    unsigned int SWIZZLE_ENABLE : 1;
    unsigned int                : 1;
    unsigned int TYPE           : 2;
#elif defined(BIGENDIAN_CPU)
    unsigned int TYPE           : 2;
    unsigned int                : 1;
    unsigned int SWIZZLE_ENABLE : 1;
    unsigned int STRIDE_SCALE   : 2;
    unsigned int STRIDE         :14;
    unsigned int                : 6;
    unsigned int NUM_RECORDS_3  : 6;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 128-159
union SQ_BUF_RSRC_WORD4 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int DST_SEL_X : 3;
    unsigned int DST_SEL_Y : 3;
    unsigned int DST_SEL_Z : 3;
    unsigned int DST_SEL_W : 3;
    unsigned int FORMAT    : 7;
    unsigned int           :13;
#elif defined(BIGENDIAN_CPU)
    unsigned int           :13;
    unsigned int FORMAT    : 7;
    unsigned int DST_SEL_W : 3;
    unsigned int DST_SEL_Z : 3;
    unsigned int DST_SEL_Y : 3;
    unsigned int DST_SEL_X : 3;
#endif
  } bits;
  uint32_t u32All;
};


/*** Image Resource Descriptor ***/

// Total 256 bits -- 8 dwords

// Bits 0-31
union SQ_IMG_RSRC_WORD0 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int BASE_ADDRESS : 32;
#elif defined(BIGENDIAN_CPU)
    unsigned int BASE_ADDRESS : 32;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 32-63
union SQ_IMG_RSRC_WORD1 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int BASE_ADDRESS_HI : 8;
    unsigned int                 : 2;
    unsigned int MAX_MIP         : 5;
    unsigned int FORMAT          : 9;
    unsigned int BASE_LEVEL      : 5;
    unsigned int                 : 3;
#elif defined(BIGENDIAN_CPU)
    unsigned int                 : 3;
    unsigned int BASE_LEVEL      : 5;
    unsigned int FORMAT          : 9;
    unsigned int MAX_MIP         : 5;
    unsigned int                 : 2;
    unsigned int BASE_ADDRESS_HI : 8;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 64-95
union SQ_IMG_RSRC_WORD2 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int WIDTH  : 16;
    unsigned int HEIGHT : 16;
#elif defined(BIGENDIAN_CPU)
    unsigned int HEIGHT : 16;
    unsigned int WIDTH  : 16;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 96-127
union SQ_IMG_RSRC_WORD3 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int               : 2;
    unsigned int DST_SEL_X     : 3;
    unsigned int DST_SEL_Y     : 3;
    unsigned int DST_SEL_Z     : 3;
    unsigned int DST_SEL_W     : 3;
    unsigned int               : 1;
    unsigned int LAST_LEVEL    : 5;
    unsigned int SW_MODE       : 5;
    unsigned int BC_SWIZZLE    : 3;
    unsigned int TYPE          : 4;
#elif defined(BIGENDIAN_CPU)
    unsigned int TYPE          : 4;
    unsigned int BC_SWIZZLE    : 3;
    unsigned int SW_MODE       : 5;
    unsigned int LAST_LEVEL    : 5;
    unsigned int               : 1;
    unsigned int DST_SEL_W     : 3;
    unsigned int DST_SEL_Z     : 3;
    unsigned int DST_SEL_Y     : 3;
    unsigned int DST_SEL_X     : 3;
    unsigned int               : 2;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 128-159
union SQ_IMG_RSRC_WORD4 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int DEPTH          :14;
    unsigned int PITCH_MSB      : 2;
    unsigned int                :16;
#elif defined(BIGENDIAN_CPU)
    unsigned int                :16;
    unsigned int PITCH_MSB      : 2;
    unsigned int DEPTH          :14;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 160-191
union SQ_IMG_RSRC_WORD5 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int                 :32;
#elif defined(BIGENDIAN_CPU)
    unsigned int                 :32;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 192-223
union SQ_IMG_RSRC_WORD6 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int                         :32;
#elif defined(BIGENDIAN_CPU)
    unsigned int                         :32;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 224-255
union SQ_IMG_RSRC_WORD7 {
  struct {
#if defined(LITTLEENDIAN_CPU)
  unsigned int                    :32;
#elif defined(BIGENDIAN_CPU)
  unsigned int                    :32;
#endif
  } bits;
  uint32_t u32All;
};

/*** Sampler Resource Descriptor ***/

// Total 128 bits -- 4 dwords

// Bits 0-31
union SQ_IMG_SAMP_WORD0 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int CLAMP_X            : 3;
    unsigned int CLAMP_Y            : 3;
    unsigned int CLAMP_Z            : 3;
    unsigned int                    : 6;
    unsigned int FORCE_UNNORMALIZED : 1;
    unsigned int                    :16;
#elif defined(BIGENDIAN_CPU)
    unsigned int                    :16;
    unsigned int FORCE_UNNORMALIZED : 1;
    unsigned int                    : 6;
    unsigned int CLAMP_Z            : 3;
    unsigned int CLAMP_Y            : 3;
    unsigned int CLAMP_X            : 3;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 32-63
union SQ_IMG_SAMP_WORD1 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int                 :13;
    unsigned int MAX_LOD         :13;
    unsigned int                 : 6;
#elif defined(BIGENDIAN_CPU)
    unsigned int                 : 6;
    unsigned int MAX_LOD         :13;
    unsigned int                 :13;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 64-95
union SQ_IMG_SAMP_WORD2 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int                 :20;
    unsigned int XY_MAG_FILTER   : 2;
    unsigned int XY_MIN_FILTER   : 2;
    unsigned int Z_FILTER        : 2;
    unsigned int MIP_FILTER      : 2;
    unsigned int                 : 4;
#elif defined(BIGENDIAN_CPU)
    unsigned int                 : 4;
    unsigned int MIP_FILTER      : 2;
    unsigned int Z_FILTER        : 2;
    unsigned int XY_MIN_FILTER   : 2;
    unsigned int XY_MAG_FILTER   : 2;
    unsigned int                 :20;
#endif
  } bits;
  uint32_t u32All;
};

// Bits 96-127
union SQ_IMG_SAMP_WORD3 {
  struct {
#if defined(LITTLEENDIAN_CPU)
    unsigned int                   :30;
    unsigned int BORDER_COLOR_TYPE : 2;
#elif defined(BIGENDIAN_CPU)
    unsigned int BORDER_COLOR_TYPE : 2;
    unsigned int                   :30;
#endif
  } bits;
  uint32_t u32All;
};


typedef enum FMT {
  FMT_INVALID      = 0x00000000,
  FMT_8            = 0x00000001,
  FMT_16           = 0x00000002,
  FMT_8_8          = 0x00000003,
  FMT_32           = 0x00000004,
  FMT_16_16        = 0x00000005,
  FMT_10_11_11     = 0x00000006,
  FMT_11_11_10     = 0x00000007,
  FMT_10_10_10_2   = 0x00000008,
  FMT_2_10_10_10   = 0x00000009,
  FMT_8_8_8_8      = 0x0000000a,
  FMT_32_32        = 0x0000000b,
  FMT_16_16_16_16  = 0x0000000c,
  FMT_32_32_32     = 0x0000000d,
  FMT_32_32_32_32  = 0x0000000e,
  FMT_RESERVED_78  = 0x0000000f,
  FMT_5_6_5        = 0x00000010,
  FMT_1_5_5_5      = 0x00000011,
  FMT_5_5_5_1      = 0x00000012,
  FMT_4_4_4_4      = 0x00000013,
  FMT_8_24         = 0x00000014,
  FMT_24_8         = 0x00000015,
  FMT_X24_8_32     = 0x00000016,
  FMT_RESERVED_155 = 0x00000017,
} FMT;

typedef enum type {
  TYPE_UNORM          = 0x00000000,
  TYPE_SNORM          = 0x00000001,
  TYPE_USCALED        = 0x00000002,
  TYPE_SSCALED        = 0x00000003,
  TYPE_UINT           = 0x00000004,
  TYPE_SINT           = 0x00000005,
  TYPE_SRGB           = 0x00000006,
  TYPE_FLOAT          = 0x00000007,
  TYPE_RESERVED_8     = 0x00000008,
  TYPE_RESERVED_9     = 0x00000009,
  TYPE_UNORM_UINT     = 0x0000000a,
  TYPE_REVERSED_UNORM = 0x0000000b,
  TYPE_FLOAT_CLAMP    = 0x0000000c,
} type;

enum FORMAT {
  CFMT_INVALID             = 0,
  CFMT_8_UNORM             = 1,
  CFMT_8_SNORM             = 2,
  CFMT_8_USCALED           = 3,
  CFMT_8_SSCALED           = 4,
  CFMT_8_UINT              = 5,
  CFMT_8_SINT              = 6,
  CFMT_16_UNORM            = 7,
  CFMT_16_SNORM            = 8,
  CFMT_16_USCALED          = 9,
  CFMT_16_SSCALED          = 10,
  CFMT_16_UINT             = 11,
  CFMT_16_SINT             = 12,
  CFMT_16_FLOAT            = 13,
  CFMT_8_8_UNORM           = 14,
  CFMT_8_8_SNORM           = 15,
  CFMT_8_8_USCALED         = 16,
  CFMT_8_8_SSCALED         = 17,
  CFMT_8_8_UINT            = 18,
  CFMT_8_8_SINT            = 19,
  CFMT_32_UINT             = 20,
  CFMT_32_SINT             = 21,
  CFMT_32_FLOAT            = 22,
  CFMT_16_16_UNORM         = 23,
  CFMT_16_16_SNORM         = 24,
  CFMT_16_16_USCALED       = 25,
  CFMT_16_16_SSCALED       = 26,
  CFMT_16_16_UINT          = 27,
  CFMT_16_16_SINT          = 28,
  CFMT_16_16_FLOAT         = 29,

  /* Note: Values different for GFX13 from 30 (compared to previous ASICs) */

  //30-35 reserved
  CFMT_10_11_11_FLOAT      = 36,
  //37-42 reserved
  CFMT_11_11_10_FLOAT      = 43,
  CFMT_10_10_10_2_UNORM    = 44,
  CFMT_10_10_10_2_SNORM    = 45,
  //46-47 reserved
  CFMT_10_10_10_2_UINT     = 48,
  CFMT_10_10_10_2_SINT     = 49,
  CFMT_2_10_10_10_UNORM    = 50,
  CFMT_2_10_10_10_SNORM    = 51,
  CFMT_2_10_10_10_USCALED  = 52,
  CFMT_2_10_10_10_SSCALED  = 53,
  CFMT_2_10_10_10_UINT     = 54,
  CFMT_2_10_10_10_SINT     = 55,
  CFMT_8_8_8_8_UNORM       = 56,
  CFMT_8_8_8_8_SNORM       = 57,
  CFMT_8_8_8_8_USCALED     = 58,
  CFMT_8_8_8_8_SSCALED     = 59,
  CFMT_8_8_8_8_UINT        = 60,
  CFMT_8_8_8_8_SINT        = 61,
  CFMT_32_32_UINT          = 62,
  CFMT_32_32_SINT          = 63,
  CFMT_32_32_FLOAT         = 64,
  CFMT_16_16_16_16_UNORM   = 65,
  CFMT_16_16_16_16_SNORM   = 66,
  CFMT_16_16_16_16_USCALED = 67,
  CFMT_16_16_16_16_SSCALED = 68,
  CFMT_16_16_16_16_UINT    = 69,
  CFMT_16_16_16_16_SINT    = 70,
  CFMT_16_16_16_16_FLOAT   = 71,
  CFMT_32_32_32_UINT       = 72,
  CFMT_32_32_32_SINT       = 73,
  CFMT_32_32_32_FLOAT      = 74,
  CFMT_32_32_32_32_UINT    = 75,
  CFMT_32_32_32_32_SINT    = 76,
  CFMT_32_32_32_32_FLOAT   = 77,
  //78-127 reserved
  CFMT_8_SRGB              = 128,
  CFMT_8_8_SRGB            = 129,
  CFMT_8_8_8_8_SRGB        = 130,
  CFMT_6E4_FLOAT           = 131,
  CFMT_5_9_9_9_FLOAT       = 132,
  CFMT_5_6_5_UNORM         = 133,
  CFMT_1_5_5_5_UNORM       = 134,
  CFMT_5_5_5_1_UNORM       = 135,
  CFMT_4_4_4_4_UNORM       = 136,
  CFMT_4_4_UNORM           = 137,
  CFMT_1_UNORM             = 138,
  CFMT_1_REVERSED_UNORM    = 139,
  CFMT_32_FLOAT_CLAMP      = 140,
  CFMT_8_24_UNORM          = 141,
  CFMT_8_24_UINT           = 142,
  CFMT_24_8_UNORM          = 143,
  CFMT_24_8_UINT           = 144,
  CFMT_X24_8_32_UINT       = 145,
  CFMT_X24_8_32_FLOAT      = 146,
};

typedef enum SEL {
  SEL_0 = 0x00000000,
  SEL_1 = 0x00000001,
  SEL_X = 0x00000004,
  SEL_Y = 0x00000005,
  SEL_Z = 0x00000006,
  SEL_W = 0x00000007,
} SEL;

typedef enum SQ_RSRC_IMG_TYPE {
  SQ_RSRC_IMG_1D            = 0x00000008,
  SQ_RSRC_IMG_2D            = 0x00000009,
  SQ_RSRC_IMG_3D            = 0x0000000a,
  SQ_RSRC_IMG_CUBE_ARRAY    = 0x0000000b,
  SQ_RSRC_IMG_1D_ARRAY      = 0x0000000c,
  SQ_RSRC_IMG_2D_ARRAY      = 0x0000000d,
  SQ_RSRC_IMG_2D_MSAA       = 0x0000000e,
  SQ_RSRC_IMG_2D_MSAA_ARRAY = 0x0000000f,
} SQ_RSRC_IMG_TYPE;

typedef enum SQ_TEX_XY_FILTER {
  SQ_TEX_XY_FILTER_POINT          = 0x00000000,
  SQ_TEX_XY_FILTER_BILINEAR       = 0x00000001,
  SQ_TEX_XY_FILTER_ANISO_POINT    = 0x00000002,
  SQ_TEX_XY_FILTER_ANISO_BILINEAR = 0x00000003,
} SQ_TEX_XY_FILTER;

typedef enum SQ_TEX_Z_FILTER {
  SQ_TEX_Z_FILTER_NONE   = 0x00000000,
  SQ_TEX_Z_FILTER_POINT  = 0x00000001,
  SQ_TEX_Z_FILTER_LINEAR = 0x00000002,
} SQ_TEX_Z_FILTER;

typedef enum SQ_TEX_MIP_FILTER {
  SQ_TEX_MIP_FILTER_NONE                = 0x00000000,
  SQ_TEX_MIP_FILTER_POINT               = 0x00000001,
  SQ_TEX_MIP_FILTER_LINEAR              = 0x00000002,
  SQ_TEX_MIP_FILTER_POINT_ANISO_ADJ__VI = 0x00000003,
} SQ_TEX_MIP_FILTER;

typedef enum SQ_TEX_CLAMP {
  SQ_TEX_WRAP                    = 0x00000000,
  SQ_TEX_MIRROR                  = 0x00000001,
  SQ_TEX_CLAMP_LAST_TEXEL        = 0x00000002,
  SQ_TEX_MIRROR_ONCE_LAST_TEXEL  = 0x00000003,
  SQ_TEX_CLAMP_HALF_BORDER       = 0x00000004,
  SQ_TEX_MIRROR_ONCE_HALF_BORDER = 0x00000005,
  SQ_TEX_CLAMP_BORDER            = 0x00000006,
  SQ_TEX_MIRROR_ONCE_BORDER      = 0x00000007,
} SQ_TEX_CLAMP;

typedef enum SQ_TEX_BORDER_COLOR {
  SQ_TEX_BORDER_COLOR_TRANS_BLACK  = 0x00000000,
  SQ_TEX_BORDER_COLOR_OPAQUE_BLACK = 0x00000001,
  SQ_TEX_BORDER_COLOR_OPAQUE_WHITE = 0x00000002,
  SQ_TEX_BORDER_COLOR_REGISTER     = 0x00000003,
} SQ_TEX_BORDER_COLOR;

typedef enum TEX_BC_SWIZZLE {
  TEX_BC_Swizzle_XYZW = 0x00000000,
  TEX_BC_Swizzle_XWYZ = 0x00000001,
  TEX_BC_Swizzle_WZYX = 0x00000002,
  TEX_BC_Swizzle_WXYZ = 0x00000003,
  TEX_BC_Swizzle_ZYXW = 0x00000004,
  TEX_BC_Swizzle_YXWZ = 0x00000005,
} TEX_BC_SWIZZLE;

typedef struct metadata_amd_gfx13_s {
  uint32_t version;   // Must be 1
  uint32_t vendorID;  // AMD
  SQ_IMG_RSRC_WORD0 word0;
  SQ_IMG_RSRC_WORD1 word1;
  SQ_IMG_RSRC_WORD2 word2;
  SQ_IMG_RSRC_WORD3 word3;
  SQ_IMG_RSRC_WORD4 word4;
  SQ_IMG_RSRC_WORD5 word5;
  SQ_IMG_RSRC_WORD6 word6;
  SQ_IMG_RSRC_WORD7 word7;
  uint32_t mip_offsets[0];
} metadata_amd_gfx13_t;

}  // namespace image
}  // namespace rocr
#endif  // EXT_IMAGE_RESOURCE_GFX13_H_
