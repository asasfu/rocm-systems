/*
 * Copyright © Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#include "image_manager_gfx13.h"

#include <assert.h>
#include <algorithm>

#include "core/util/utils.h"
#include "addrlib/src/core/addrlib.h"
#include "image_runtime.h"
#include "resource.h"
#include "resource_gfx13.h"
#include "util.h"

namespace rocr {
namespace image {

static_assert(sizeof(SQ_BUF_RSRC_WORD0) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_BUF_RSRC_WORD1) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_BUF_RSRC_WORD2) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_BUF_RSRC_WORD3) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_BUF_RSRC_WORD4) == sizeof(uint32_t), "struct size is invalid");

static_assert(sizeof(SQ_IMG_RSRC_WORD0) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_RSRC_WORD1) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_RSRC_WORD2) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_RSRC_WORD3) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_RSRC_WORD4) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_RSRC_WORD5) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_RSRC_WORD6) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_RSRC_WORD7) == sizeof(uint32_t), "struct size is invalid");

static_assert(sizeof(SQ_IMG_SAMP_WORD0) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_SAMP_WORD1) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_SAMP_WORD2) == sizeof(uint32_t), "struct size is invalid");
static_assert(sizeof(SQ_IMG_SAMP_WORD3) == sizeof(uint32_t), "struct size is invalid");

static TEX_BC_SWIZZLE GetBcSwizzle(const Swizzle& swizzle) {
    SEL r = (SEL)swizzle.x;
    SEL g = (SEL)swizzle.y;
    SEL b = (SEL)swizzle.z;
    SEL a = (SEL)swizzle.w;

    TEX_BC_SWIZZLE bcSwizzle = TEX_BC_Swizzle_XYZW;

    if (a == SEL_X) {
        // Have to use either TEX_BC_Swizzle_WZYX or TEX_BC_Swizzle_WXYZ
        //
        // For the pre-defined border color values (white, opaque black,
        // transparent black), the only thing that matters is that the alpha
        // channel winds up in the correct place (because the RGB channels are
        // all the same) so either of these TEX_BC_Swizzle enumerations will
        // work.  Not sure what happens with border color palettes.
        if (b == SEL_Y) {
            // ABGR
            bcSwizzle = TEX_BC_Swizzle_WZYX;
        } else if ((r == SEL_X) && (g == SEL_X) && (b == SEL_X)) {
            // RGBA
            bcSwizzle = TEX_BC_Swizzle_XYZW;
        } else {
            // ARGB
            bcSwizzle = TEX_BC_Swizzle_WXYZ;
        }
    } else if (r == SEL_X) {
        // Have to use either TEX_BC_Swizzle_XYZW or TEX_BC_Swizzle_XWYZ
        if (g == SEL_Y) {
            // RGBA
            bcSwizzle = TEX_BC_Swizzle_XYZW;
        } else if ((g == SEL_X) && (b == SEL_X) && (a == SEL_W)) {
            // RGBA
            bcSwizzle = TEX_BC_Swizzle_XYZW;
        } else {
            // RAGB
            bcSwizzle = TEX_BC_Swizzle_XWYZ;
        }
    } else if (g == SEL_X) {
        // GRAB, have to use TEX_BC_Swizzle_YXWZ
        bcSwizzle = TEX_BC_Swizzle_YXWZ;
    } else if (b == SEL_X) {
        // BGRA, have to use TEX_BC_Swizzle_ZYXW
        bcSwizzle = TEX_BC_Swizzle_ZYXW;
    }

    return bcSwizzle;
}

struct formatconverstion_t {
  FMT fmt;
  type type;
  FORMAT format;
};

// Map addrlib swizzle type to hw encoded swizzle
static const uint32_t hwSwizzleModeLUT[] {
   0, // ADDR3_LINEAR     0
   3, // ADDR3_256B_2D    1
   7, // ADDR3_4KB_2D     2
  11, // ADDR3_64KB_2D    3
  15, // ADDR3_256KB_2D   4
   4, // ADDR3_4KB_3D     5
   8, // ADDR3_64KB_3D    6
  12, // ADDR3_256KB_3D   7
  29, // ADDR3_64KB_2D_Z  8
  30  // ADDR3_256KB_2D_Z 9
};

// Convert addrlib swizzle to hw encoding for SW_MODE field
static uint32_t addrLibToHwSwizzle(uint32_t swizzle) {
  if (swizzle < ADDR3_MAX_TYPE) {
    return hwSwizzleModeLUT[swizzle];
  } else {
    return (uint32_t) -1;
  }
}

// Format/Type to combined format code table.
// Sorted and indexed to allow fast searches.
static const formatconverstion_t FormatLUT[] = {
    {FMT_1_5_5_5, TYPE_UNORM, CFMT_1_5_5_5_UNORM},              // 0
    {FMT_10_10_10_2, TYPE_UNORM, CFMT_10_10_10_2_UNORM},        // 1
    {FMT_10_10_10_2, TYPE_SNORM, CFMT_10_10_10_2_SNORM},        // 2
    {FMT_10_10_10_2, TYPE_UINT, CFMT_10_10_10_2_UINT},          // 3
    {FMT_10_10_10_2, TYPE_SINT, CFMT_10_10_10_2_SINT},          // 4
    {FMT_16, TYPE_UNORM, CFMT_16_UNORM},                        // 5
    {FMT_16, TYPE_SNORM, CFMT_16_SNORM},                        // 6
    {FMT_16, TYPE_UINT, CFMT_16_UINT},                          // 7
    {FMT_16, TYPE_SINT, CFMT_16_SINT},                          // 8
    {FMT_16, TYPE_FLOAT, CFMT_16_FLOAT},                        // 9
    {FMT_16, TYPE_USCALED, CFMT_16_USCALED},                    // 10
    {FMT_16, TYPE_SSCALED, CFMT_16_SSCALED},                    // 11
    {FMT_16_16, TYPE_UNORM, CFMT_16_16_UNORM},                  // 12
    {FMT_16_16, TYPE_SNORM, CFMT_16_16_SNORM},                  // 13
    {FMT_16_16, TYPE_UINT, CFMT_16_16_UINT},                    // 14
    {FMT_16_16, TYPE_SINT, CFMT_16_16_SINT},                    // 15
    {FMT_16_16, TYPE_FLOAT, CFMT_16_16_FLOAT},                  // 16
    {FMT_16_16, TYPE_USCALED, CFMT_16_16_USCALED},              // 17
    {FMT_16_16, TYPE_SSCALED, CFMT_16_16_SSCALED},              // 18
    {FMT_16_16_16_16, TYPE_UNORM, CFMT_16_16_16_16_UNORM},      // 19
    {FMT_16_16_16_16, TYPE_SNORM, CFMT_16_16_16_16_SNORM},      // 20
    {FMT_16_16_16_16, TYPE_UINT, CFMT_16_16_16_16_UINT},        // 21
    {FMT_16_16_16_16, TYPE_SINT, CFMT_16_16_16_16_SINT},        // 22
    {FMT_16_16_16_16, TYPE_FLOAT, CFMT_16_16_16_16_FLOAT},      // 23
    {FMT_16_16_16_16, TYPE_USCALED, CFMT_16_16_16_16_USCALED},  // 24
    {FMT_16_16_16_16, TYPE_SSCALED, CFMT_16_16_16_16_SSCALED},  // 25
    {FMT_2_10_10_10, TYPE_UNORM, CFMT_2_10_10_10_UNORM},        // 26
    {FMT_2_10_10_10, TYPE_SNORM, CFMT_2_10_10_10_SNORM},        // 27
    {FMT_2_10_10_10, TYPE_UINT, CFMT_2_10_10_10_UINT},          // 28
    {FMT_2_10_10_10, TYPE_SINT, CFMT_2_10_10_10_SINT},          // 29
    {FMT_2_10_10_10, TYPE_USCALED, CFMT_2_10_10_10_USCALED},    // 30
    {FMT_2_10_10_10, TYPE_SSCALED, CFMT_2_10_10_10_SSCALED},    // 31
    {FMT_24_8, TYPE_UNORM, CFMT_24_8_UNORM},                    // 32
    {FMT_24_8, TYPE_UINT, CFMT_24_8_UINT},                      // 33
    {FMT_32, TYPE_UINT, CFMT_32_UINT},                          // 34
    {FMT_32, TYPE_SINT, CFMT_32_SINT},                          // 35
    {FMT_32, TYPE_FLOAT, CFMT_32_FLOAT},                        // 36
    {FMT_32_32, TYPE_UINT, CFMT_32_32_UINT},                    // 37
    {FMT_32_32, TYPE_SINT, CFMT_32_32_SINT},                    // 38
    {FMT_32_32, TYPE_FLOAT, CFMT_32_32_FLOAT},                  // 39
    {FMT_32_32_32, TYPE_UINT, CFMT_32_32_32_UINT},              // 40
    {FMT_32_32_32, TYPE_SINT, CFMT_32_32_32_SINT},              // 41
    {FMT_32_32_32, TYPE_FLOAT, CFMT_32_32_32_FLOAT},            // 42
    {FMT_32_32_32_32, TYPE_UINT, CFMT_32_32_32_32_UINT},        // 43
    {FMT_32_32_32_32, TYPE_SINT, CFMT_32_32_32_32_SINT},        // 44
    {FMT_32_32_32_32, TYPE_FLOAT, CFMT_32_32_32_32_FLOAT},      // 45
    {FMT_5_5_5_1, TYPE_UNORM, CFMT_5_5_5_1_UNORM},              // 46
    {FMT_5_6_5, TYPE_UNORM, CFMT_5_6_5_UNORM},                  // 47
    {FMT_8, TYPE_UNORM, CFMT_8_UNORM},                          // 48
    {FMT_8, TYPE_SNORM, CFMT_8_SNORM},                          // 49
    {FMT_8, TYPE_UINT, CFMT_8_UINT},                            // 50
    {FMT_8, TYPE_SINT, CFMT_8_SINT},                            // 51
    {FMT_8, TYPE_SRGB, CFMT_8_SRGB},                            // 52
    {FMT_8, TYPE_USCALED, CFMT_8_USCALED},                      // 53
    {FMT_8, TYPE_SSCALED, CFMT_8_SSCALED},                      // 54
    {FMT_8_24, TYPE_UNORM, CFMT_8_24_UNORM},                    // 55
    {FMT_8_24, TYPE_UINT, CFMT_8_24_UINT},                      // 56
    {FMT_8_8, TYPE_UNORM, CFMT_8_8_UNORM},                      // 57
    {FMT_8_8, TYPE_SNORM, CFMT_8_8_SNORM},                      // 58
    {FMT_8_8, TYPE_UINT, CFMT_8_8_UINT},                        // 59
    {FMT_8_8, TYPE_SINT, CFMT_8_8_SINT},                        // 60
    {FMT_8_8, TYPE_SRGB, CFMT_8_8_SRGB},                        // 61
    {FMT_8_8, TYPE_USCALED, CFMT_8_8_USCALED},                  // 62
    {FMT_8_8, TYPE_SSCALED, CFMT_8_8_SSCALED},                  // 63
    {FMT_8_8_8_8, TYPE_UNORM, CFMT_8_8_8_8_UNORM},              // 64
    {FMT_8_8_8_8, TYPE_SNORM, CFMT_8_8_8_8_SNORM},              // 65
    {FMT_8_8_8_8, TYPE_UINT, CFMT_8_8_8_8_UINT},                // 66
    {FMT_8_8_8_8, TYPE_SINT, CFMT_8_8_8_8_SINT},                // 67
    {FMT_8_8_8_8, TYPE_SRGB, CFMT_8_8_8_8_SRGB},                // 68
    {FMT_8_8_8_8, TYPE_USCALED, CFMT_8_8_8_8_USCALED},          // 69
    {FMT_8_8_8_8, TYPE_SSCALED, CFMT_8_8_8_8_SSCALED}           // 70
};
static const int FormatLUTSize = sizeof(FormatLUT)/sizeof(formatconverstion_t);
//Index in FormatLUT to start search, indexed by FMT enum.
static const int FormatEntryPoint[] = {
  71, // FMT_INVALID
  48, // FMT_8
  5,  // FMT_16
  57, // FMT_8_8
  34, // FMT_32
  12, // FMT_16_16
  71, // FMT_10_11_11
  71, // FMT_11_11_10
  1,  // FMT_10_10_10_2
  26, // FMT_2_10_10_10
  64, // FMT_8_8_8_8
  37, // FMT_32_32
  19, // FMT_16_16_16_16
  40, // FMT_32_32_32
  43, // FMT_32_32_32_32
  71, // RESERVED
  47, // FMT_5_6_5
  0,  // FMT_1_5_5_5
  46, // FMT_5_5_5_1
  71, // FMT_4_4_4_4
  55, // FMT_8_24
  32  // FMT_24_8
};

static FORMAT GetCombinedFormat(uint8_t fmt, uint8_t type) {
  assert(fmt < sizeof(FormatEntryPoint)/sizeof(int) && "FMT out of range.");
  int start = FormatEntryPoint[fmt];
  int stop = std::min(start + 7, FormatLUTSize); // Only 7 types are used in LUT table

  for(int i=start; i<stop; i++) {
    if((FormatLUT[i].fmt == fmt) && (FormatLUT[i].type == type))
      return FormatLUT[i].format;
  }
  return CFMT_INVALID;
};


ImageManagerGfx13::ImageManagerGfx13() : ImageManagerGfx12() {}

ImageManagerGfx13::~ImageManagerGfx13() {}

hsa_status_t ImageManagerGfx13::PopulateImageSrd(Image& image,
                                     const metadata_amd_t* descriptor) const {
  const metadata_amd_gfx13_t* desc = reinterpret_cast<const metadata_amd_gfx13_t*>(descriptor);
  const void* image_data_addr = image.data;

  ImageProperty image_prop = ImageLut().MapFormat(image.desc.format, image.desc.geometry);
  if ((image_prop.cap == HSA_EXT_IMAGE_CAPABILITY_NOT_SUPPORTED) ||
     (image_prop.element_size == 0))
    return (hsa_status_t)HSA_EXT_STATUS_ERROR_IMAGE_FORMAT_UNSUPPORTED;

  const Swizzle swizzle = ImageLut().MapSwizzle(image.desc.format.channel_order);

  if (IsLocalMemory(image.data)) {
    image_data_addr = reinterpret_cast<const void*>(
        reinterpret_cast<uintptr_t>(image.data) - local_memory_base_address_);
  }

  image.srd[0] = desc->word0.u32All;
  image.srd[1] = desc->word1.u32All;
  image.srd[2] = desc->word2.u32All;
  image.srd[3] = desc->word3.u32All;
  image.srd[4] = desc->word4.u32All;
  image.srd[5] = desc->word5.u32All;
  image.srd[6] = desc->word6.u32All;
  image.srd[7] = desc->word7.u32All;

  if (image.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DB) {
    SQ_BUF_RSRC_WORD0 word0;
    SQ_BUF_RSRC_WORD1 word1;
    SQ_BUF_RSRC_WORD3 word3;
    SQ_BUF_RSRC_WORD4 word4;

    word0.u32All = 0;
    word0.bits.BASE_ADDRESS = PtrLow32(image_data_addr);

    word1.u32All = image.srd[1];
    word1.bits.BASE_ADDRESS_HI = PtrHigh32(image_data_addr);

    word3.u32All = image.srd[3];
    word3.bits.STRIDE = image_prop.element_size;
    word3.bits.STRIDE_SCALE = image_prop.element_size;

    word4.u32All = image.srd[4];
    word4.bits.DST_SEL_X = swizzle.x;
    word4.bits.DST_SEL_Y = swizzle.y;
    word4.bits.DST_SEL_Z = swizzle.z;
    word4.bits.DST_SEL_W = swizzle.w;
    word4.bits.FORMAT = GetCombinedFormat(image_prop.data_format, image_prop.data_type);

    image.srd[0] = word0.u32All;
    image.srd[1] = word1.u32All;
    image.srd[3] = word3.u32All;
    image.srd[4] = word4.u32All;
  } else {
    uint32_t hwPixelSize = ImageLut().GetPixelSize(image_prop.data_format, image_prop.data_type);

    if (image_prop.element_size != hwPixelSize) {
      return (hsa_status_t)HSA_EXT_STATUS_ERROR_IMAGE_FORMAT_UNSUPPORTED;
    }
    reinterpret_cast<SQ_IMG_RSRC_WORD0*>(&image.srd[0])->bits.BASE_ADDRESS =
        PtrLow40Shift8(image_data_addr);
    reinterpret_cast<SQ_IMG_RSRC_WORD1*>(&image.srd[1])->bits.BASE_ADDRESS_HI =
        PtrHigh64Shift40(image_data_addr);

    reinterpret_cast<SQ_IMG_RSRC_WORD1*>(&image.srd[1])->bits.FORMAT = GetCombinedFormat(image_prop.data_format, image_prop.data_type);
    reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&image.srd[3])->bits.DST_SEL_X = swizzle.x;
    reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&image.srd[3])->bits.DST_SEL_Y = swizzle.y;
    reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&image.srd[3])->bits.DST_SEL_Z = swizzle.z;
    reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&image.srd[3])->bits.DST_SEL_W = swizzle.w;
    if (image.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DA ||
        image.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1D) {
      reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&image.srd[3])->bits.TYPE =
          ImageLut().MapGeometry(image.desc.geometry);
    }
  }

  // Looks like this is only used for CPU copies.
  image.row_pitch = 0;
  image.slice_pitch = 0;

  // Used by HSAIL shader ABI
  image.srd[8] = image.desc.format.channel_type;
  image.srd[9] = image.desc.format.channel_order;
  image.srd[10] = static_cast<uint32_t>(image.desc.width);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t ImageManagerGfx13::PopulateImageSrd(Image& image) const {
  ImageProperty image_prop = ImageLut().MapFormat(image.desc.format, image.desc.geometry);
  assert(image_prop.cap != HSA_EXT_IMAGE_CAPABILITY_NOT_SUPPORTED);
  assert(image_prop.element_size != 0);
  const void* image_data_addr = image.data;

  if (IsLocalMemory(image.data))
    image_data_addr = reinterpret_cast<const void*>(
        reinterpret_cast<uintptr_t>(image.data) - local_memory_base_address_);

  if (image.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DB) {
    SQ_BUF_RSRC_WORD0 word0;
    SQ_BUF_RSRC_WORD1 word1;
    SQ_BUF_RSRC_WORD2 word2;
    SQ_BUF_RSRC_WORD3 word3;
    SQ_BUF_RSRC_WORD4 word4;

    uint64_t num_records = image.desc.width * image_prop.element_size;

    word0.u32All = 0;
    word0.bits.BASE_ADDRESS = PtrLow32(image_data_addr);

    word1.u32All = 0;
    word1.bits.BASE_ADDRESS_HI = PtrHigh32(image_data_addr);
    word1.bits.NUM_RECORDS_1 = BitSelect<0, 6>(num_records);

    word2.u32All = 0;
    word2.bits.NUM_RECORDS_2 = BitSelect<7, 39>(num_records);

    const Swizzle swizzle = ImageLut().MapSwizzle(image.desc.format.channel_order);
    word3.u32All = 0;
    word3.bits.NUM_RECORDS_3 = BitSelect<40, 45>(num_records);
    word3.bits.STRIDE = image_prop.element_size;
    word3.bits.SWIZZLE_ENABLE = 0;
    word3.bits.TYPE = ImageLut().MapGeometry(image.desc.geometry);

    word4.u32All = 0;
    word4.bits.DST_SEL_X = swizzle.x;
    word4.bits.DST_SEL_Y = swizzle.y;
    word4.bits.DST_SEL_Z = swizzle.z;
    word4.bits.DST_SEL_W = swizzle.w;
    word4.bits.FORMAT = GetCombinedFormat(image_prop.data_format, image_prop.data_type);

    image.srd[0] = word0.u32All;
    image.srd[1] = word1.u32All;
    image.srd[2] = word2.u32All;
    image.srd[3] = word3.u32All;
    image.srd[4] = word4.u32All;

    image.row_pitch = image.desc.width * image_prop.element_size;
    image.slice_pitch = image.row_pitch;
  } else {
    SQ_IMG_RSRC_WORD0 word0;
    SQ_IMG_RSRC_WORD1 word1;
    SQ_IMG_RSRC_WORD2 word2;
    SQ_IMG_RSRC_WORD3 word3;
    SQ_IMG_RSRC_WORD4 word4;
    SQ_IMG_RSRC_WORD5 word5;
    SQ_IMG_RSRC_WORD6 word6;
    SQ_IMG_RSRC_WORD7 word7;

    ADDR3_COMPUTE_SURFACE_INFO_OUTPUT out = {0};

    uint32_t swizzleMode = GetAddrlibSurfaceInfoNv(image.component, image.desc,
                  1, image.tile_mode, image.row_pitch, image.slice_pitch, out);
    if (swizzleMode == (uint32_t)(-1)) {
      return HSA_STATUS_ERROR;
    }

    assert((out.bpp / 8) == image_prop.element_size);

    const size_t row_pitch_size = out.pitch * image_prop.element_size;

    word0.bits.BASE_ADDRESS = PtrLow40Shift8(image_data_addr);

    word1.u32All = 0;
    word1.bits.BASE_ADDRESS_HI = PtrHigh64Shift40(image_data_addr);
    word1.bits.FORMAT = GetCombinedFormat(image_prop.data_format, image_prop.data_type);

    word2.u32All = 0;
    word2.bits.WIDTH = image.desc.width - 1;
    word2.bits.HEIGHT = image.desc.height ? image.desc.height - 1 : 0;

    const Swizzle swizzle = ImageLut().MapSwizzle(image.desc.format.channel_order);
    word3.u32All = 0;
    word3.bits.DST_SEL_X = swizzle.x;
    word3.bits.DST_SEL_Y = swizzle.y;
    word3.bits.DST_SEL_Z = swizzle.z;
    word3.bits.DST_SEL_W = swizzle.w;

    const uint32_t swMode = addrLibToHwSwizzle(swizzleMode);
    if (swMode != (uint32_t)(-1)) {
      word3.bits.SW_MODE = swMode;
    } else {
      return HSA_STATUS_ERROR;
    }

    word3.bits.BC_SWIZZLE = GetBcSwizzle(swizzle);
    word3.bits.TYPE = ImageLut().MapGeometry(image.desc.geometry);

    const bool image_array =
        (image.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DA ||
         image.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_2DA ||
         image.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_2DADEPTH);
    const bool image_3d = (image.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_3D);

    word4.u32All = 0;

    // For 1d, 2d and 2d-msaa, fields DEPTH+PITCH_MSB encode pitch-1
    if (!image_array && !image_3d) {
      uint32_t encPitch = out.pitch - 1;
      word4.bits.DEPTH = encPitch & 0x3fff;           // first 14 bits
      word4.bits.PITCH_MSB = (encPitch >> 14) & 0x3;  // last 2 bits
    } else {
      word4.bits.DEPTH =
        (image_array)
            ? std::max(image.desc.array_size, static_cast<size_t>(1)) - 1
            : (image_3d) ? image.desc.depth - 1 : 0;
    }

    word5.u32All = 0;
    word6.u32All = 0;
    word7.u32All = 0;

    image.srd[0] = word0.u32All;
    image.srd[1] = word1.u32All;
    image.srd[2] = word2.u32All;
    image.srd[3] = word3.u32All;
    image.srd[4] = word4.u32All;
    image.srd[5] = word5.u32All;
    image.srd[6] = word6.u32All;
    image.srd[7] = word7.u32All;

    image.row_pitch = row_pitch_size;
    image.slice_pitch = out.sliceSize;
  }

  image.srd[8] = image.desc.format.channel_type;
  image.srd[9] = image.desc.format.channel_order;
  image.srd[10] = static_cast<uint32_t>(image.desc.width);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t ImageManagerGfx13::ModifyImageSrd(
    Image& image, hsa_ext_image_format_t& new_format) const {
  image.desc.format = new_format;

  ImageProperty image_prop = ImageLut().MapFormat(image.desc.format, image.desc.geometry);
  assert(image_prop.cap != HSA_EXT_IMAGE_CAPABILITY_NOT_SUPPORTED);
  assert(image_prop.element_size != 0);

  if (image.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DB) {
    const Swizzle swizzle = ImageLut().MapSwizzle(image.desc.format.channel_order);
    SQ_BUF_RSRC_WORD4* word4 =
        reinterpret_cast<SQ_BUF_RSRC_WORD4*>(&image.srd[4]);
    word4->bits.DST_SEL_X = swizzle.x;
    word4->bits.DST_SEL_Y = swizzle.y;
    word4->bits.DST_SEL_Z = swizzle.z;
    word4->bits.DST_SEL_W = swizzle.w;
    word4->bits.FORMAT = GetCombinedFormat(image_prop.data_format, image_prop.data_type);
  } else {
    SQ_IMG_RSRC_WORD1* word1 =
        reinterpret_cast<SQ_IMG_RSRC_WORD1*>(&image.srd[1]);
    word1->bits.FORMAT = GetCombinedFormat(image_prop.data_format, image_prop.data_type);

    const Swizzle swizzle = ImageLut().MapSwizzle(image.desc.format.channel_order);
    SQ_IMG_RSRC_WORD3* word3 =
        reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&image.srd[3]);
    word3->bits.DST_SEL_X = swizzle.x;
    word3->bits.DST_SEL_Y = swizzle.y;
    word3->bits.DST_SEL_Z = swizzle.z;
    word3->bits.DST_SEL_W = swizzle.w;
  }

  image.srd[8] = image.desc.format.channel_type;
  image.srd[9] = image.desc.format.channel_order;
  image.srd[10] = static_cast<uint32_t>(image.desc.width);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t ImageManagerGfx13::PopulateSamplerSrd(Sampler& sampler) const {
  const hsa_ext_sampler_descriptor_v2_t &sampler_descriptor = sampler.desc;

  SQ_IMG_SAMP_WORD0 word0;
  SQ_IMG_SAMP_WORD1 word1;
  SQ_IMG_SAMP_WORD2 word2;
  SQ_IMG_SAMP_WORD3 word3;

  word0.u32All = 0;
  hsa_status_t status = convertAddressMode<SQ_IMG_SAMP_WORD0, SQ_TEX_CLAMP>
                                       (word0, sampler_descriptor.address_modes);
  if (status != HSA_STATUS_SUCCESS) return status;
  word0.bits.FORCE_UNNORMALIZED = (sampler_descriptor.coordinate_mode ==
                                  HSA_EXT_SAMPLER_COORDINATE_MODE_UNNORMALIZED);

  word1.u32All = 0;
  word1.bits.MAX_LOD = 4095;

  word2.u32All = 0;
  switch (sampler_descriptor.filter_mode) {
    case HSA_EXT_SAMPLER_FILTER_MODE_NEAREST:
      word2.bits.XY_MAG_FILTER = static_cast<int>(SQ_TEX_XY_FILTER_POINT);
      break;
    case HSA_EXT_SAMPLER_FILTER_MODE_LINEAR:
      word2.bits.XY_MAG_FILTER = static_cast<int>(SQ_TEX_XY_FILTER_BILINEAR);
      break;
    default:
      return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }
  word2.bits.XY_MIN_FILTER = word2.bits.XY_MAG_FILTER;
  word2.bits.Z_FILTER = SQ_TEX_Z_FILTER_NONE;
  word2.bits.MIP_FILTER = SQ_TEX_MIP_FILTER_NONE;

  word3.u32All = 0;
  word3.bits.BORDER_COLOR_TYPE = SQ_TEX_BORDER_COLOR_TRANS_BLACK;

  sampler.srd[0] = word0.u32All;
  sampler.srd[1] = word1.u32All;
  sampler.srd[2] = word2.u32All;
  sampler.srd[3] = word3.u32All;

  return HSA_STATUS_SUCCESS;
}

hsa_status_t ImageManagerGfx13::PopulateMipmapSrd(MipmappedArray& mipmap) const {
  // Map format/geometry to hardware encoding
  ImageProperty mipmap_prop = ImageLut().MapFormat(mipmap.desc.format, mipmap.desc.geometry);
  assert(mipmap_prop.cap != HSA_EXT_IMAGE_CAPABILITY_NOT_SUPPORTED);
  assert(mipmap_prop.element_size != 0);
  assert(mipmap.num_levels >= 1);
  const void* mipmap_data_addr = mipmap.data;

  if (IsLocalMemory(mipmap.data)) {
    mipmap_data_addr = reinterpret_cast<const void*>(
        reinterpret_cast<uintptr_t>(mipmap.data) - local_memory_base_address_);
  }

  if (mipmap.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DB) {
    SQ_BUF_RSRC_WORD0 word0;
    SQ_BUF_RSRC_WORD1 word1;
    SQ_BUF_RSRC_WORD2 word2;
    SQ_BUF_RSRC_WORD3 word3;
    SQ_BUF_RSRC_WORD4 word4;

    uint64_t num_records = mipmap.desc.width * mipmap_prop.element_size;

    word0.u32All = 0;
    word0.bits.BASE_ADDRESS = PtrLow32(mipmap_data_addr);

    word1.u32All = 0;
    word1.bits.BASE_ADDRESS_HI = PtrHigh32(mipmap_data_addr);
    word1.bits.NUM_RECORDS_1 = BitSelect<0, 6>(num_records);

    word2.u32All = 0;
    word2.bits.NUM_RECORDS_2 = BitSelect<7, 39>(num_records);

    const Swizzle swizzle = ImageLut().MapSwizzle(mipmap.desc.format.channel_order);
    word3.u32All = 0;
    word3.bits.NUM_RECORDS_3 = BitSelect<40, 45>(num_records);
    word3.bits.STRIDE = mipmap_prop.element_size;
    word3.bits.STRIDE_SCALE = mipmap_prop.element_size;
    word3.bits.SWIZZLE_ENABLE = 0;
    word3.bits.TYPE = ImageLut().MapGeometry(mipmap.desc.geometry);

    word4.u32All = 0;
    word4.bits.DST_SEL_X = swizzle.x;
    word4.bits.DST_SEL_Y = swizzle.y;
    word4.bits.DST_SEL_Z = swizzle.z;
    word4.bits.DST_SEL_W = swizzle.w;
    word4.bits.FORMAT = GetCombinedFormat(mipmap_prop.data_format, mipmap_prop.data_type);

    mipmap.srd[0] = word0.u32All;
    mipmap.srd[1] = word1.u32All;
    mipmap.srd[2] = word2.u32All;
    mipmap.srd[3] = word3.u32All;
    mipmap.srd[4] = word4.u32All;

    // 1DB mipmaps don't use words 5-7
    mipmap.srd[5] = 0;
    mipmap.srd[6] = 0;
    mipmap.srd[7] = 0;

    mipmap.row_pitch = mipmap.desc.width * mipmap_prop.element_size;
    mipmap.slice_pitch = mipmap.row_pitch;
  } else {
    SQ_IMG_RSRC_WORD0 word0;
    SQ_IMG_RSRC_WORD1 word1;
    SQ_IMG_RSRC_WORD2 word2;
    SQ_IMG_RSRC_WORD3 word3;
    SQ_IMG_RSRC_WORD4 word4;
    SQ_IMG_RSRC_WORD5 word5;
    SQ_IMG_RSRC_WORD6 word6;
    SQ_IMG_RSRC_WORD7 word7;

    // Get ADDR3 surface information
    ADDR3_COMPUTE_SURFACE_INFO_OUTPUT out = {0};

    // pMipInfo not needed - set to nullptr and AddrLib will ignore it
    out.pMipInfo = nullptr;

    unsigned int swizzleMode = GetAddrlibSurfaceInfoNv(mipmap.component,
                            mipmap.desc, mipmap.num_levels, mipmap.tile_mode,
                            mipmap.row_pitch, mipmap.slice_pitch, out);
    if (swizzleMode == (uint32_t)(-1)) {
      return HSA_STATUS_ERROR;
    }
    mipmap.addr_output.addr3 = out;
    mipmap.size = out.surfSize;

    assert((out.bpp / 8) == mipmap_prop.element_size);

    const size_t row_pitch_size = out.pitch * mipmap_prop.element_size;

    word0.u32All = 0;
    word0.bits.BASE_ADDRESS = PtrLow40Shift8(mipmap_data_addr);

    word1.u32All = 0;
    word1.bits.BASE_ADDRESS_HI = PtrHigh64Shift40(mipmap_data_addr);
    word1.bits.MAX_MIP = mipmap.num_levels - 1;
    word1.bits.BASE_LEVEL = 0;
    word1.bits.FORMAT = GetCombinedFormat(mipmap_prop.data_format, mipmap_prop.data_type);

    word2.u32All = 0;
    word2.bits.WIDTH = mipmap.desc.width - 1;
    word2.bits.HEIGHT = mipmap.desc.height ? mipmap.desc.height - 1 : 0;

    const Swizzle swizzle = ImageLut().MapSwizzle(mipmap.desc.format.channel_order);
    word3.u32All = 0;
    word3.bits.DST_SEL_X = swizzle.x;
    word3.bits.DST_SEL_Y = swizzle.y;
    word3.bits.DST_SEL_Z = swizzle.z;
    word3.bits.DST_SEL_W = swizzle.w;
    word3.bits.LAST_LEVEL = mipmap.num_levels - 1;

    const uint32_t swMode = addrLibToHwSwizzle(swizzleMode);
    if (swMode != (uint32_t)(-1)) {
      word3.bits.SW_MODE = swMode;
    } else {
      return HSA_STATUS_ERROR;
    }

    word3.bits.BC_SWIZZLE = GetBcSwizzle(swizzle);
    word3.bits.TYPE = ImageLut().MapGeometry(mipmap.desc.geometry);

    const bool mipmap_array =
        (mipmap.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DA ||
         mipmap.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_2DA ||
         mipmap.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_2DADEPTH);
    const bool mipmap_3d = (mipmap.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_3D);

    word4.u32All = 0;

    // For 1d, 2d and 2d-msaa, fields DEPTH+PITCH_MSB encode pitch-1
    if (!mipmap_array && !mipmap_3d) {
      uint32_t encPitch = out.pitch - 1;
      word4.bits.DEPTH = encPitch & 0x3fff;           // first 14 bits
      word4.bits.PITCH_MSB = (encPitch >> 14) & 0x3;  // last 2 bits
    } else {
      word4.bits.DEPTH =
        (mipmap_array)
            ? std::max(mipmap.desc.array_size, static_cast<size_t>(1)) - 1
            : (mipmap_3d) ? mipmap.desc.depth - 1 : 0;
    }

    word5.u32All = 0;
    word6.u32All = 0;
    word7.u32All = 0;

    mipmap.srd[0] = word0.u32All;
    mipmap.srd[1] = word1.u32All;
    mipmap.srd[2] = word2.u32All;
    mipmap.srd[3] = word3.u32All;
    mipmap.srd[4] = word4.u32All;
    mipmap.srd[5] = word5.u32All;
    mipmap.srd[6] = word6.u32All;
    mipmap.srd[7] = word7.u32All;

    mipmap.row_pitch = row_pitch_size;
    mipmap.slice_pitch = out.sliceSize;
  }

  mipmap.srd[8] = mipmap.desc.format.channel_type;
  mipmap.srd[9] = mipmap.desc.format.channel_order;
  mipmap.srd[10] = static_cast<uint32_t>(mipmap.desc.width);

  // Mipmap-specific
  mipmap.srd[11] = mipmap.num_levels;

  return HSA_STATUS_SUCCESS;
}

hsa_status_t ImageManagerGfx13::PopulateMipmapSrd(MipmappedArray& mipmap_array, const metadata_amd_t* desc) const {
  const metadata_amd_gfx13_t* desc_gfx13 = reinterpret_cast<const metadata_amd_gfx13_t*>(desc);
  const void* mipmap_data_addr = mipmap_array.data;

  ImageProperty mipmap_prop = ImageLut().MapFormat(mipmap_array.desc.format, mipmap_array.desc.geometry);
  if (mipmap_prop.cap == HSA_EXT_IMAGE_CAPABILITY_NOT_SUPPORTED || mipmap_prop.element_size == 0) {
    return (hsa_status_t)HSA_EXT_STATUS_ERROR_IMAGE_FORMAT_UNSUPPORTED;
  }

  const Swizzle swizzle = ImageLut().MapSwizzle(mipmap_array.desc.format.channel_order);

  if (IsLocalMemory(mipmap_array.data)) {
    mipmap_data_addr = reinterpret_cast<const void*>(
        reinterpret_cast<uintptr_t>(mipmap_array.data) - local_memory_base_address_);
  }

  // Copy the pre-computed SRD words 0-7 from metadata
  mipmap_array.srd[0] = desc_gfx13->word0.u32All;
  mipmap_array.srd[1] = desc_gfx13->word1.u32All;
  mipmap_array.srd[2] = desc_gfx13->word2.u32All;
  mipmap_array.srd[3] = desc_gfx13->word3.u32All;
  mipmap_array.srd[4] = desc_gfx13->word4.u32All;
  mipmap_array.srd[5] = desc_gfx13->word5.u32All;
  mipmap_array.srd[6] = desc_gfx13->word6.u32All;
  mipmap_array.srd[7] = desc_gfx13->word7.u32All;

  if (mipmap_array.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DB) {
    // 1DB uses buffer descriptors
    SQ_BUF_RSRC_WORD0 word0;
    SQ_BUF_RSRC_WORD1 word1;
    SQ_BUF_RSRC_WORD3 word3;
    SQ_BUF_RSRC_WORD4 word4;

    word0.u32All = 0;
    word0.bits.BASE_ADDRESS = PtrLow32(mipmap_data_addr);

    word1.u32All = mipmap_array.srd[1];
    word1.bits.BASE_ADDRESS_HI = PtrHigh32(mipmap_data_addr);

    word3.u32All = mipmap_array.srd[3];
    word3.bits.STRIDE = mipmap_prop.element_size;
    word3.bits.STRIDE_SCALE = mipmap_prop.element_size;

    word4.u32All = mipmap_array.srd[4];
    word4.bits.DST_SEL_X = swizzle.x;
    word4.bits.DST_SEL_Y = swizzle.y;
    word4.bits.DST_SEL_Z = swizzle.z;
    word4.bits.DST_SEL_W = swizzle.w;
    word4.bits.FORMAT = GetCombinedFormat(mipmap_prop.data_format, mipmap_prop.data_type);

    mipmap_array.srd[0] = word0.u32All;
    mipmap_array.srd[1] = word1.u32All;
    mipmap_array.srd[3] = word3.u32All;
    mipmap_array.srd[4] = word4.u32All;

    mipmap_array.row_pitch = mipmap_array.desc.width * mipmap_prop.element_size;
    mipmap_array.slice_pitch = mipmap_array.row_pitch;
  } else {
    // Non-1DB uses image descriptors
    uint32_t hwPixelSize = ImageLut().GetPixelSize(mipmap_prop.data_format, mipmap_prop.data_type);
    if (mipmap_prop.element_size != hwPixelSize) {
      return (hsa_status_t)HSA_EXT_STATUS_ERROR_IMAGE_FORMAT_UNSUPPORTED;
    }

    reinterpret_cast<SQ_IMG_RSRC_WORD0*>(&mipmap_array.srd[0])->bits.BASE_ADDRESS = PtrLow40Shift8(mipmap_data_addr);
    reinterpret_cast<SQ_IMG_RSRC_WORD1*>(&mipmap_array.srd[1])->bits.BASE_ADDRESS_HI = PtrHigh64Shift40(mipmap_data_addr);
    reinterpret_cast<SQ_IMG_RSRC_WORD1*>(&mipmap_array.srd[1])->bits.FORMAT = GetCombinedFormat(mipmap_prop.data_format, mipmap_prop.data_type);
    reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&mipmap_array.srd[3])->bits.DST_SEL_X = swizzle.x;
    reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&mipmap_array.srd[3])->bits.DST_SEL_Y = swizzle.y;
    reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&mipmap_array.srd[3])->bits.DST_SEL_Z = swizzle.z;
    reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&mipmap_array.srd[3])->bits.DST_SEL_W = swizzle.w;

    if (mipmap_array.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DA ||
        mipmap_array.desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1D) {
      reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&mipmap_array.srd[3])->bits.TYPE =
          ImageLut().MapGeometry(mipmap_array.desc.geometry);
    }
  }

  // Looks like this is only used for CPU copies.
  mipmap_array.row_pitch = 0;
  mipmap_array.slice_pitch = 0;

  // Store mipmap-specific metadata
  mipmap_array.srd[8] = mipmap_array.desc.format.channel_type;
  mipmap_array.srd[9] = mipmap_array.desc.format.channel_order;
  mipmap_array.srd[10] = static_cast<uint32_t>(mipmap_array.desc.width);
  mipmap_array.srd[11] = mipmap_array.num_levels;

  // Allocate and populate pMipInfo from metadata mip_offsets
  ADDR3_MIP_INFO* mip_info_storage = new ADDR3_MIP_INFO[mipmap_array.num_levels];
  memset(mip_info_storage, 0, sizeof(ADDR3_MIP_INFO) * mipmap_array.num_levels);

  // Extract per-level information from mip_offsets array
  for (uint32_t level = 0; level < mipmap_array.num_levels; level++) {
    // mip_offsets contains offset bits [39:8], shift left by 8 to get actual byte offset
    mip_info_storage[level].offset = static_cast<uint64_t>(desc_gfx13->mip_offsets[level]) << 8;

    // Calculate dimensions for this level (halve at each level)
    mip_info_storage[level].pixelPitch = std::max(1u, static_cast<uint32_t>(mipmap_array.desc.width >> level));
    mip_info_storage[level].pixelHeight = std::max(1u, static_cast<uint32_t>(mipmap_array.desc.height >> level));
    mip_info_storage[level].depth = std::max(1u, static_cast<uint32_t>(mipmap_array.desc.depth >> level));
  }

  // Store pMipInfo in addr_output for later use by PopulateMipLevelSrd
  mipmap_array.addr_output.addr3.pMipInfo = mip_info_storage;

  // Total size calculation from metadata (estimate from last level)
  uint32_t last_level = mipmap_array.num_levels - 1;
  uint64_t last_level_size = mip_info_storage[last_level].pixelPitch *
                             mip_info_storage[last_level].pixelHeight *
                             mip_info_storage[last_level].depth *
                             mipmap_prop.element_size;
  mipmap_array.size = mip_info_storage[last_level].offset + last_level_size;

  return HSA_STATUS_SUCCESS;
}

hsa_status_t ImageManagerGfx13::PopulateMipLevelSrd(
    MipmappedArray& level_view,
    const MipmappedArray& mipmap_array,
    uint32_t mip_level) const {

  // SRD already copied from parent, just modify BASE_LEVEL/LAST_LEVEL fields
  uint32_t* srd_words = reinterpret_cast<uint32_t*>(level_view.srd);

  // GFX13 SRD WORDs 1 and 3 has BASE_LEVEL and LAST_LEVEL fields
  SQ_IMG_RSRC_WORD1* word1 = reinterpret_cast<SQ_IMG_RSRC_WORD1*>(&srd_words[1]);
  SQ_IMG_RSRC_WORD3* word3 = reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&srd_words[3]);

  // Set both to same value - hardware samples only this level
  word1->bits.BASE_LEVEL = mip_level;
  word3->bits.LAST_LEVEL = mip_level;

  debug_print("Set SRD mip selection: BASE_LEVEL=%u, LAST_LEVEL=%u", mip_level, mip_level);

  return HSA_STATUS_SUCCESS;
}

hsa_status_t ImageManagerGfx13::FillImage(const Image& image, const void* pattern,
                                       const hsa_ext_image_region_t& region) {

  if (BlitQueueInit().queue_ == NULL) {
    return HSA_STATUS_ERROR_OUT_OF_RESOURCES;
  }

  Image* image_view = const_cast<Image*>(&image);

  SQ_BUF_RSRC_WORD4* word4_buff = NULL;
  SQ_IMG_RSRC_WORD3* word3_image = NULL;
  uint32_t dst_sel_w_original = 0;
  if (image_view->desc.format.channel_type ==
      HSA_EXT_IMAGE_CHANNEL_TYPE_UNORM_SHORT_101010) {
    // Force GPU to ignore the last two bits (alpha bits).
    if (image_view->desc.geometry == HSA_EXT_IMAGE_GEOMETRY_1DB) {
      // GFX13: Use WORD4 for buffer descriptors instead of WORD3
      word4_buff = reinterpret_cast<SQ_BUF_RSRC_WORD4*>(&image_view->srd[4]);
      dst_sel_w_original = word4_buff->bits.DST_SEL_W;
      word4_buff->bits.DST_SEL_W = SEL_0;
    } else {
      word3_image = reinterpret_cast<SQ_IMG_RSRC_WORD3*>(&image_view->srd[3]);
      dst_sel_w_original = word3_image->bits.DST_SEL_W;
      word3_image->bits.DST_SEL_W = SEL_0;
    }
  }

  SQ_IMG_RSRC_WORD1* word1 = NULL;
  uint32_t num_format_original = 0;
  const void* new_pattern = pattern;
  float fill_value[4] = {0};
  switch (image_view->desc.format.channel_order) {
    case HSA_EXT_IMAGE_CHANNEL_ORDER_SRGBA:
    case HSA_EXT_IMAGE_CHANNEL_ORDER_SRGB:
    case HSA_EXT_IMAGE_CHANNEL_ORDER_SRGBX:
    case HSA_EXT_IMAGE_CHANNEL_ORDER_SBGRA: {
      // We do not have write support for SRGBA image, so convert pattern
      // to standard form and treat the image as RGBA image.
      const float* pattern_f = reinterpret_cast<const float*>(pattern);
      fill_value[0] = LinearToStandardRGB(pattern_f[0]);
      fill_value[1] = LinearToStandardRGB(pattern_f[1]);
      fill_value[2] = LinearToStandardRGB(pattern_f[2]);
      fill_value[3] = pattern_f[3];
      new_pattern = fill_value;

      ImageProperty image_prop = ImageLut().MapFormat(image.desc.format, image.desc.geometry);

      word1 = reinterpret_cast<SQ_IMG_RSRC_WORD1*>(&image_view->srd[1]);
      num_format_original = word1->bits.FORMAT;
      word1->bits.FORMAT = GetCombinedFormat(image_prop.data_format, TYPE_UNORM);
    } break;
    default:
      break;
  }

  hsa_status_t status = ImageRuntime::instance()->blit_kernel().FillImage(
      blit_queue_, blit_code_catalog_, *image_view, new_pattern, region);

  // Revert back original configuration.
  if (word4_buff != NULL) {
    word4_buff->bits.DST_SEL_W = dst_sel_w_original;
  }

  if (word3_image != NULL) {
    word3_image->bits.DST_SEL_W = dst_sel_w_original;
  }

  if (word1 != NULL) {
    word1->bits.FORMAT = num_format_original;
  }

  return status;
}

}  // namespace image
}  // namespace rocr
