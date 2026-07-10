/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palVideoEncoder.h
 * @brief Defines the Platform Abstraction Library (PAL) IVideoEncoder interface and related types.
 ***********************************************************************************************************************
 */

#pragma once

#include "pal.h"
#include "palDevice.h"
#include "palGpuMemoryBindable.h"

namespace Pal
{
#if PAL_BUILD_VIDEO

/// Video H.264 profile
enum class VideoEncodeH264Profile : uint32
{
    Baseline = 0x0,     ///< Baseline Profile
    Main     = 0x1,     ///< Main Profile
    High     = 0x2,     ///< High Profile
    Count
};

/// Video H.264 level to use for encoding
enum class VideoEncodeH264Level : uint32
{
    Level_1_0 = 10,     ///< Level 1.0
    Level_1_1 = 11,     ///< Level 1.1
    Level_1_2 = 12,     ///< Level 1.2
    Level_1_3 = 13,     ///< Level 1.3
    Level_2_0 = 20,     ///< Level 2.0
    Level_2_1 = 21,     ///< Level 2.1
    Level_2_2 = 22,     ///< Level 2.2
    Level_3_0 = 30,     ///< Level 3.0
    Level_3_1 = 31,     ///< Level 3.1
    Level_3_2 = 32,     ///< Level 3.2
    Level_4_0 = 40,     ///< Level 4.0
    Level_4_1 = 41,     ///< Level 4.1
    Level_4_2 = 42,     ///< Level 4.2
    Level_5_0 = 50,     ///< Level 5.0
    Level_5_1 = 51,     ///< Level 5.1
    Level_5_2 = 52      ///< Level 5.2
};

/// Encode H.264 Pre-Encode mode.
enum class VideoEncodeH264PreEncodeMode : uint32
{
    None = 0x0,   ///< Disable Pre-encode Mode
    Auto = 0x1,   ///< Enable Pre-encode Mode
    Count
};

/// Encode H.264 GOP type.
enum class VideoEncodeH264GopType : uint32
{
    FixedSize = 0x0,   ///< The GOP is fixed size specified by GOP_SIZE
    MinMax    = 0x1,
    Count
};

/// Encode H.264 Header mode.
enum class VideoEncodeH264HeaderMode : uint32
{
    None       = 0x0,
    GopAligned = 0x1,   ///< GOP aligned to insert headers (VPS, SPS, and PPS) in bitstream
    IdrAligned = 0x2,   ///< IDR aligned to insert headers (VPS, SPS, and PPS) in bitstream
    Count
};

/// Encode H.264 Intra Refresh mode.
enum class VideoEncodeH264IntraRefreshMode : uint32
{
    Disabled   = 0x0,
    GopAligned = 0x1,   ///< GOP aligned to insert Intra Refresh frame
    Continuous = 0x2,
    Count
};

/// Encode H.264 Slice Control mode.
enum class VideoEncodeH264SliceControlMode : uint32
{
    FixedMbs  = 0x0,   ///< Used when encSliceControlMode == ENC__SLICE_CONTROL_MODE__FIXED_MBS
    FixedBits = 0x1,   ///< Used when encSliceControlMode == ENC__SLICE_CONTROL_MODE__FIXED_BITS
    Count
};

/// Encode H.264 Rate Control mode.
enum class VideoEncodeH264RateControlMode : uint32
{
    None                  = 0x0,
    LatencyConstrainedVbr = 0x1,   ///< Latency constrained variable bit rate
    PeakConstrainedVbr    = 0x2,   ///< Peak constrained variable bit rate
    Cbr                   = 0x3,   ///< Constant bit rate
    Count
};

/// Encode H.264 Quality preset.
enum class VideoEncodeH264QualityPreset : uint32
{
    Speed    = 0x0,   ///< HW configuration optimized for encoding speed
    Balanced = 0x1,   ///< Balanced video encoder configuration
    Quality  = 0x2,   ///< Constant bit rate
    Count
};

/// Encode H.264 Vbaq mode.
enum class VideoEncodeH264VbaqMode : uint32
{
    None = 0x0,   ///< Disable VBAQ - Variable Based Adaptive Quantization
    Auto = 0x1,   ///< Enable VBAQ - Variable Based Adaptive Quantization
    Count
};

/// Encode H.264 Picture type.
enum class VideoEncodeH264PictureType : uint32
{
    Unspecified = 0x0,   ///< User is not forcing a specific picture type
    Idr         = 0x1,   ///< Instantaneous decoder refresh frame
    I           = 0x2,   ///< Intra coded frame
    P           = 0x3,   ///< Predictive frame
    P_Skip      = 0x4,   ///< P-Picture with all units coded as skip (match reference)
    Count
};

/// H.264 NAL type.
enum class VideoEncodeH264NalType : uint32
{
    Idr      = 0x0,
    Cra      = 0x1,   ///< clean random access
    Tsa      = 0x2,   ///< temporal sub-layer access
    Trailing = 0x3,   ///< trailing Nal type
    Count
};

/// H.264 POC type.
enum class VideoEncodeH264PocType : uint32
{
    Type0 = 0x0,   ///< Picture order count type 0: Send POC explicitly in each slice header
    Type2 = 0x2,   ///< Picture order count type 2: Display order same as decoding order
    Count
};

/// Encode H264 QP map type.
enum class VideoEncodeH264QpMapType : uint32
{
    None     = 0x0,
    Delta    = 0x1,
    Absolute = 0x2,
    ROI      = 0x3,
    PA       = 0x4,
    Count
};

/// Encode H264 two pass search center map mode
enum class VideoEncodeH264TwoPassSearchCenterMapMode : uint32
{
    None = 0x0,
    Auto = 0x1,
    Count
};

/// Video H.265 profile.
enum class VideoEncodeH265Profile : uint32
{
    Main = 0x0,   ///< Main Profile
    Count
};

/// H.265 tier.
enum class VideoEncodeH265Tier : uint32
{
    Main = 0x0,   ///< H.265 HEVC Main Tier.
    High = 0x1,   ///< H.265 HEVC High Tier.
    Count
};

/// Video H.265 Level.
enum class VideoEncodeH265Level : uint32
{
    Level_1_0 = 30,     ///< Level 1.0
    Level_2_0 = 60,     ///< Level 2.0
    Level_2_1 = 63,     ///< Level 2.1
    Level_3_0 = 90,     ///< Level 3.0
    Level_3_1 = 93,     ///< Level 3.1
    Level_4_0 = 120,    ///< Level 4.0
    Level_4_1 = 123,    ///< Level 4.1
    Level_5_0 = 150,    ///< Level 5.0
    Level_5_1 = 153,    ///< Level 5.1
    Level_5_2 = 156,    ///< Level 5.2
    Level_6_0 = 180,    ///< Level 6.0
    Level_6_1 = 183,    ///< Level 6.1
    Level_6_2 = 186     ///< Level 6.2
};

/// Encode H.265 Pre-Encode mode.
enum class VideoEncodeH265PreEncodeMode : uint32
{
    None = 0x0,   ///< Disable Pre-encode Mode
    Auto = 0x1,   ///< Enable Pre-encode Mode
    Count
};

/// Encode H.265 GOP type.
enum class VideoEncodeH265GopType : uint32
{
    FixedSize = 0x0,   ///< The GOP is fixed size specified by GOP_SIZE
    MinMax    = 0x1,
    Count
};

/// Encode H.265 Header mode.
enum class VideoEncodeH265HeaderMode : uint32
{
    None       = 0x0,
    GopAligned = 0x1,   ///< GOP aligned to insert headers (VPS, SPS, and PPS) in bitstream
    IdrAligned = 0x2,   ///< IDR aligned to insert headers (VPS, SPS, and PPS) in bitstream
    Count
};

/// Encode H.265 Intra Refresh mode.
enum class VideoEncodeH265IntraRefreshMode : uint32
{
    Disabled   = 0x0,
    GopAligned = 0x1,   ///< GOP aligned to insert Intra Refresh frame
    Continuous = 0x2,
    Count
};

/// Encode H.265 Slice Control mode.
enum class VideoEncodeH265SliceControlMode : uint32
{
    FixedCtbs = 0x0,   ///< Encode slices with specified number of CTBs (last slice may be smaller)
    FixedBits = 0x1,   ///< Used when encSliceControlMode == HEVC_ENC__SLICE_CONTROL_MODE__FIXED_BITS
    Count
};

/// Encode H.265 Rate Control mode.
enum class VideoEncodeH265RateControlMode : uint32
{
    None                  = 0x0,
    LatencyConstrainedVbr = 0x1,   ///< Latency constrained variable bit rate
    PeakConstrainedVbr    = 0x2,   ///< Peak constrained variable bit rate
    Cbr                   = 0x3,   ///< Constant bit rate
    Count
};

/// Encode H.265 Quality preset.
enum class VideoEncodeH265QualityPreset : uint32
{
    Speed    = 0x0,   ///< HW configuration optimized for encoding speed
    Balanced = 0x1,   ///< Balanced video encoder configuration
    Quality  = 0x2,   ///< HW configuration optimized for encoding quality
    Count
};

/// Encode H.265 Vbaq mode.
enum class VideoEncodeH265VbaqMode : uint32
{
    None = 0x0,   ///< Disable VBAQ - Variable Based Adaptive Quantization
    Auto = 0x1,   ///< Enable VBAQ - Variable Based Adaptive Quantization
    Count
};

/// Encode H.265 Picture type.
enum class VideoEncodeH265PictureType : uint32
{
    Unspecified = 0x0,   ///< User is not forcing a specific picture type
    Idr         = 0x1,   ///< Instantaneous decoder refresh frame
    I           = 0x2,   ///< Intra coded frame
    P           = 0x3,   ///< Predictive frame
    P_Skip      = 0x4,   ///< P-Picture with all units coded as skip (match reference)
    Count
};

/// H.265 NAL type.
enum class VideoEncodeH265NalType : uint32
{
    Idr        = 0x0,
    Cra        = 0x1,   ///< clean random access
    Tsa_N      = 0x2,   ///< temporal sub-layer access (sublayer non-reference)
    Tsa_R      = 0x3,   ///< temporal sub-layer access (sublayer reference)
    Trailing_N = 0x4,   ///< trailing Nal type (sublayer non-reference)
    Trailing_R = 0x5,   ///< trailing Nal type (sublayer reference)
    Count
};

/// Encode H265 QP map type.
enum class VideoEncodeH265QpMapType : uint32
{
    None     = 0x0,
    Delta    = 0x1,
    Absolute = 0x2,
    ROI      = 0x3,
    PA       = 0x4,
    Count
};

/// Encode H265 two pass search center map mode
enum class VideoEncodeH265TwoPassSearchCenterMapMode : uint32
{
    None = 0x0,
    Auto = 0x1,
    Count
};

/// Specifies properties for creation of a @ref IVideoEncoder object. Input structure to
/// IDevice::CreateVideoEncoder().
struct VideoEncoderCreateInfo
{
    EngineType              engineType;             ///< Engine type to create the encoder for
    SwizzledFormat          inputFormat;            ///< Input image format
    Extent2d                extent;                 ///< Output video stream width and height
    uint32                  maxFeedbacks;           ///< Maximum number of feedbacks to maintain (at least 2)
    VideoEncodeCodec        codecType;              ///< Video encode codec type supported

    union
    {
        /// H.264 specific parameters.
        struct
        {
            VideoEncodeH264Profile       profile;               ///< H.264 profile
            VideoEncodeH264Level         level;                 ///< H.264 level
            uint32                       maxNumReferenceFrames; ///< Maximum number of reference pictures kept in the DPB
            VideoEncodeH264PreEncodeMode preEncodeMode;         ///< Pre Encode Mode
            VideoEncodeH264TwoPassSearchCenterMapMode twoPassSearchCenterMapMode; ///< Two Pass Search Center Map Mode
        } h264;

        /// H.265 specific parameters.
        struct
        {
            VideoEncodeH265Tier          tier;                   ///< H.265 tier
            VideoEncodeH265Profile       profile;                ///< H.265 profile
            VideoEncodeH265Level         level;                  ///< H.265 level
            uint32                       maxNumReferenceFrames;  ///< Maximum number of reference pictures kept in the DPB
            VideoEncodeH265PreEncodeMode preEncodeMode;          ///< Pre Encode Mode
            VideoEncodeH265TwoPassSearchCenterMapMode twoPassSearchCenterMapMode; ///< Two Pass Search Center Map Mode
        } h265;
    };
};

/// Defines the content of video encoder feedback entries
struct VideoEncodeFeedback
{
    gpusize                 bitstreamOffset;        ///< Offset of the bitstream in the output buffer
    gpusize                 bitstreamSize;          ///< Size of the bitstream (including filler data and padding)
    gpusize                 fillerDataSize;         ///< Filler data size included in the bitstream
    gpusize                 paddingSize;            ///< Size of the padding at the end of the bitstream
    uint64                  timestampFrequency;     ///< Timestamp frequency in Hz
    uint64                  startTimestamp;         ///< Start timestamp of frame encoding
    uint64                  endTimestamp;           ///< End timestamp of frame encoding
    uint32                  hdcpEncrypted;          ///< 0 - bitstream is not encrypted; 1 - bitstream is encrypted
    uint32                  hdcpInputCounterHi;     ///< higher order 32-bits of the 64-bit HDCP input counter
    uint32                  hdcpInputCounterLo;     ///< lower order 32-bits of the 64-bit HDCP input counter

    union
    {
        /// H.264 specific parameters.
        struct
        {
            uint32    firstMbQp;                    ///< QP of the first encoded MB in a picture (RC related)
            uint32    minMbQp;                      ///< Min QP of all encoded MBs in a picture (RC related)
            uint32    maxMbQp;                      ///< Max QP of all encoded MBs in a picture (RC related)
            uint32    averageMbQp;                  ///< Average QP of all encoded MBs in a picture (RC related)

            int32     markedLtrIndex;               ///< Marked LTR index (if >= 0)
            uint16    referencedLtrIndices;         ///< Bitfield of referenced LTR indices
            VideoEncodeH264PictureType outputPictureType;    ///< Output picture type
        } h264;

        /// H.265 specific parameters.
        struct
        {
            uint32    firstCtbQp;                   ///< QP of the first encoded CTB in a picture (RC related)
            uint32    minCtbQp;                     ///< Min QP of all encoded CTBs in a picture (RC related)
            uint32    maxCtbQp;                     ///< Max QP of all encoded CTBs in a picture (RC related)
            uint32    averageCtbQp;                 ///< Average QP of all encoded CTBs in a picture (RC related)
            int32     markedLtrIndex;               ///< Marked LTR index (if >= 0)
            uint16    referencedLtrIndices;         ///< Bitfield of referenced LTR indices
            VideoEncodeH265PictureType outputPictureType;    ///< Output picture type
            VideoEncodeH265NalType     outputNalType;        ///< OUtput nal type
       } h265;
    };
};

/// Defines to support EFC
enum class VideoEncodeH264PictureFormat : uint32
{
    UveH264PictureFormat_NV12            = 0x0,                        ///< NV12 Picture format of efc input or output surfaces
    UveH264PictureFormat_RGB             = 0x1,                        ///< RGB Picture format of efc input or output surfaces
    UveH264PictureFormat_B8G8R8A8        = UveH264PictureFormat_RGB,   ///< B8G8R8A8 Picture format of efc input or output surfaces
    UveH264PictureFormat_B10G10R10A2     = 0x2,                        ///< B10G10R10A2 Picture format of efc input or output surfaces
    UveH264PictureFormat_R16G16B16A16F   = 0x3,                        ///< R16G16B16A16F Picture format of efc input or output surfaces
    UveH264PictureFormat_P010            = 0x4,                        ///< P010 Picture format of efc input or output surfaces
    UveH264PictureFormat_AYUV            = 0x5,                        ///< AYUV Picture format of efc input or output surfaces
    UveH264PictureFormat_Y410            = 0x6,                        ///< Y410 Picture format of efc input or output surfaces
    UveH264PictureFormat_R8G8B8A8        = 0x7,                        ///< R8G8B8A8 Picture format of efc input or output surfaces
    UveH264PictureFormat_R10G10B10A2     = 0x8,                        ///< R10G10B10A2 Picture format of efc input or output surfaces
    Count
};

enum class VideoEncodeH264ColorVolume : uint32
{
    UveH264ColorVolume_G22_BT709    = 0x0,                        ///< G22_BT709 color volume of efc input or output surfaces
    UveH264ColorVolume_G10_SCRGB    = 0x1,                        ///< G10_SCRGB color volume of efc input or output surfaces
    UveH264ColorVolume_G10_BT709    = 0x2,                        ///< G10_BT709 color volume of efc input or output surfaces
    UveH264ColorVolume_G10_BT2020   = 0x3,                        ///< G10_BT2020 color volume of efc input or output surfaces
    UveH264ColorVolume_G2084_BT2020 = 0x4,                        ///< G2084_BT2020 color volume of efc input or output surfaces
    Count
};

enum class VideoEncodeH264ColorSpace : uint32
{
    UveH264ColorSpace_YUV = 0x0,                                ///< YUV color volume of efc input or output surfaces
    UveH264ColorSpace_RGB = 0x1,                                ///< RGB color volume of efc input or output surfaces
    Count
};

enum class VideoEncodeH264ColorIntegerRange : uint32
{
    UveH264ColorIntegerRange_Full    = 0x0,                     ///< Full color integer range of efc input or output surfaces
    UveH264ColorIntegerRange_Studio  = 0x1,                     ///< Studio color integer range efc input or output surfaces
    Count
};

enum class VideoEncodeH264ChromaSubSampling : uint32
{
    UveH264ChromaSubSampling_4_2_0 = 0x0,                       ///< 420 chroma subsampling of efc input or output surfaces
    UveH264ChromaSubSampling_4_4_4 = 0x1,                       ///< 444 chroma subsampling of efc input or output surfaces
    Count
};

enum class VideoEncodeH264ChromaLocation : uint32
{
    UveH264ChromaLocation_Interstitial = 0x0,                   ///< Interstitial chroma location of efc input or output surfaces
    UveH264ChromaLocation_CoSite       = 0x1,                   ///< Cosite chroma location of efc input or output surfaces
    Count
};

enum class VideoEncodeH264ColorBitDepth : uint32
{
    UveH264ColorBitDepth_8_BIT   = 0x0,                         ///< 8bit bitdepth of efc input or output surfaces
    UveH264ColorBitDepth_10_BIT  = 0x1,                         ///< 10bit bitdepth of efc input or output surfaces
    UveH264ColorBitDepth_FP16    = 0x2,                         ///< 16bit Floating point bitdepth of efc input or output surfaces
    Count
};

enum class VideoEncodeH264ColorPackingFormat : uint32
{
    UveH264ColorPackingFormat_NV12           = 0x0,             ///< NV12 color packing format of efc input or output surfaces
    UveH264ColorPackingFormat_P010           = 0x1,             ///< P010 color packing format of efc input or output surfaces
    UveH264ColorPackingFormat_AYUV           = 0x2,             ///< AYUV color packing format of efc input or output surfaces
    UveH264ColorPackingFormat_Y410           = 0x3,             ///< Y410 color packing format of efc input or output surfaces
    UveH264ColorPackingFormat_B8G8R8A8       = 0x4,             ///< B8G8R8A8 color packing format of efc input or output surfaces
    UveH264ColorPackingFormat_B10G10R10A2    = 0x5,             ///< B10G10R10A2 color packing format of efc input or output surfaces
    UveH264ColorPackingFormat_R16G16B16A16F  = 0x6,             ///< R16G16B16A16F color packing format of efc input or output surfaces
    UveH264ColorPackingFormat_R8G8B8A8       = 0x7,             ///< R8G8B8A8 color packing format of efc input or output surfaces
    UveH264ColorPackingFormat_R10G10B10A2    = 0x8,             ///< R10G10B10A2 color packing format of efc input or output surfaces
    Count
};
/// H265 EFC Parameters
enum class VideoEncodeH265PictureFormat : uint32
{
    UveH265PictureFormat_NV12          = 0x0,                        ///< NV12 Picture format of efc input or output surfaces
    UveH265PictureFormat_RGB           = 0x1,                        ///< RGB Picture format of efc input or output surfaces
    UveH265PictureFormat_B8G8R8A8      = UveH265PictureFormat_RGB,   ///< B8G8R8A8 Picture format of efc input or output surfaces
    UveH265PictureFormat_B10G10R10A2   = 0x2,                        ///< B10G10R10A2 Picture format of efc input or output surfaces
    UveH265PictureFormat_R16G16B16A16F = 0x3,                        ///< R16G16B16A16F Picture format of efc input or output surfaces
    UveH265PictureFormat_P010          = 0x4,                        ///< P010 Picture format of efc input or output surfaces
    UveH265PictureFormat_AYUV          = 0x5,                        ///< AYUV Picture format of efc input or output surfaces
    UveH265PictureFormat_Y410          = 0x6,                        ///< Y410 Picture format of efc input or output surfaces
    UveH265PictureFormat_R8G8B8A8      = 0x7,                        ///< R8G8B8A8 Picture format of efc input or output surfaces
    UveH265PictureFormat_R10G10B10A2   = 0x8,                        ///< R10G10B10A2 Picture format of efc input or output surfaces
    Count
};

enum class VideoEncodeH265ColorVolume : uint32
{
    UveH265ColorVolume_G22_BT709    = 0x0,                        ///< G22_BT709 color volume of efc input or output surfaces
    UveH265ColorVolume_G10_SCRGB    = 0x1,                        ///< G10_SCRGB color volume of efc input or output surfaces
    UveH265ColorVolume_G10_BT709    = 0x2,                        ///< G10_BT709 color volume of efc input or output surfaces
    UveH265ColorVolume_G10_BT2020   = 0x3,                        ///< G10_BT2020 color volume of efc input or output surfaces
    UveH265ColorVolume_G2084_BT2020 = 0x4,                        ///< G2084_BT2020 color volume of efc input or output surfaces
    Count
};

enum class VideoEncodeH265ColorSpace : uint32
{
    UveH265ColorSpace_YUV = 0x0,                                ///< YUV color space of efc input or output surfaces
    UveH265ColorSpace_RGB = 0x1,                                ///< RGB color space of efc input or output surfaces
    Count
};

enum class VideoEncodeH265ColorIntegerRange : uint32
{
    UveH265ColorIntegerRange_Full   = 0x0,                     ///< Full color integer range of efc input or output surfaces
    UveH265ColorIntegerRange_Studio = 0x1,                     ///< studio color integer range of efc input or output surfaces
    Count
};

enum class VideoEncodeH265ChromaSubSampling : uint32
{
    UveH265ChromaSubSampling_4_2_0 = 0x0,                       ///< 420 chroma subsampling of efc input or output surfaces
    UveH265ChromaSubSampling_4_4_4 = 0x1,                       ///< 420 chroma subsampling of efc input or output surfaces
    Count
};

enum class VideoEncodeH265ChromaLocation : uint32
{
    UveH265ChromaLocation_Interstitial = 0x0,                   ///< Interstitial chroma location of efc input or output surfaces
    UveH265ChromaLocation_CoSite       = 0x1,                   ///< Cosite chroma location of efc input or output surfaces
    Count
};

enum class VideoEncodeH265ColorBitDepth : uint32
{
    UveH265ColorBitDepth_8_BIT  = 0x0,                         ///< 8bit bitdepth of efc input or output surfaces
    UveH265ColorBitDepth_10_BIT = 0x1,                         ///< 10bit bitdepth of efc input or output surfaces
    UveH265ColorBitDepth_FP16   = 0x2,                         ///< 16bit floating point bitdepth of efc input or output surfaces
    Count
};

enum class VideoEncodeH265ColorPackingFormat : uint32
{
    UveH265ColorPackingFormat_NV12            = 0x0,             ///< NV12 color packing format of efc input or output surfaces
    UveH265ColorPackingFormat_P010            = 0x1,             ///< P010 color packing format of efc input or output surfaces
    UveH265ColorPackingFormat_AYUV            = 0x2,             ///< AYUV color packing format of efc input or output surfaces
    UveH265ColorPackingFormat_Y410            = 0x3,             ///< Y410 color packing format of efc input or output surfaces
    UveH265ColorPackingFormat_B8G8R8A8        = 0x4,             ///< B8G8R8A8 color packing format of efc input or output surfaces
    UveH265ColorPackingFormat_B10G10R10A2     = 0x5,             ///< B10G10R10A2 color packing format of efc input or output surfaces
    UveH265ColorPackingFormat_R16G16B16A16F   = 0x6,             ///< R16G16B16A16F color packing format of efc input or output surfaces
    UveH265ColorPackingFormat_R8G8B8A8        = 0x7,             ///< R8G8B8A8 color packing format of efc input or output surfaces
    UveH265ColorPackingFormat_R10G10B10A2     = 0x8,             ///< R10G10B10A2 color packing format of efc input or output surfaces
    Count
};

/// Defines the set of flags that can be used with feedback retrieval
enum VideoEncodeFeedbackFlags : uint32
{
    VideoEncodeFeedbackDefault  = 0x00000000,   ///< Default with no waiting
    VideoEncodeFeedbackWait     = 0x00000001,   ///< Indicates that the specified number of feedbacks should be waited
                                                ///  on to be retrievable
    VideoEncodeFeedbackAllFlags = 0x00000001    ///< Clients should NOT use it, for internal static_assert purpose only.
};

/// This enum gives details of Sei Insertion mode for next picture or next idr.
enum class VideoEncodeH265SeiInsertionMode : uint32
{

    None = 0,       ///< None
    NextPicture,    ///< Mode for next picture
    NextIdr,        ///< Mode for idr
    Count
};
/**
 ***********************************************************************************************************************
 * @interface IVideoEncoder
 * @brief     Object containing video encoder state. Separate concrete implementations will support various
 *            HW implementations.
 *
 * @see IDevice::CreateVideoEncoder()
 ***********************************************************************************************************************
 */
class IVideoEncoder : public IGpuMemoryBindable
{
public:
    /// Returns the number of available feedbacks.
    virtual uint32 GetFeedbackCount() = 0;

    /// Returns a feedback entry from the encoder.
    ///
    /// @param [out] pFeedback  Pointer to write the feedback entry to.
    /// @param [in]  flags      Flags determining the behavior of the retrieval (@see VideoEncodeFeedbackFlag).
    ///
    /// @returns Success if the feedback entry was successfully returned in pFeedback, or NotReady if there is no
    ///          available feedback.
    virtual Result GetFeedback(
        VideoEncodeFeedback*     pFeedback,
        VideoEncodeFeedbackFlags flags) = 0;

    /// Returns H264/H265 SPS header content from the encoder.
    ///
    /// @param [out]       pBuffer Pointer to write the SPS header content to.
    ///                            When this pointer is NULL, this function is used to query proper SPS size in byte.
    /// @param [in/output] pSize   [in] This is the buffer size (in byte) pointed by pBuffer.
    ///                            [out]The queried proper SPS size (in byte) when pBuffer is NULL
    ///                                 Returned SPS header content size (in byte) when pBuffer is valid.
    ///
    /// @returns Success           If SPS content was successfully returned in pBuffer when pBuffer is not NULL,
    ///                            in this case, pSize contains real content size.
    ///                            Or when pBuffer is NULL (query SPS header size).
    ///          NotReady          If this function was called before BeginEncode is called.
    ///          Unsupported       Function call is not supported.
    ///          ErrorInvalidMemorySize  Specified pBuffer is not big enough for SPS header content,
    ///                                  in this case, pSize will return the required buffer size
    virtual Result GetSps(
        uint32* pBuffer,
        uint32* pSize) = 0;

    /// Returns H264/H265 PPS header content from the encoder.
    ///
    /// @param [out]       pBuffer Pointer to write the PPS header content to.
    ///                            When this pointer is NULL, this function is used to query proper PPS size in byte.
    /// @param [in/output] pSize   [in] This is the buffer size (in byte) pointed by pBuffer.
    ///                            [out] The queried proper PPS size (in byte) when pBuffer is NULL
    ///                                  Returned PPS header content size (in byte) when pBuffer is valid.
    ///
    /// @returns Success           If PPS content was successfully returned in pBuffer when pBuffer is not NULL,
    ///                            in this case, pSize contains real content size.
    ///                            Or when pBuffer is NULL (query PPS header size).
    ///          NotReady          If this function was called before BeginEncode is called.
    ///          Unsupported       Function call is not supported.
    ///          ErrorInvalidMemorySize  Specified pBuffer is not big enough for PPS header content,
    ///                                  in this case, pSize will return the required buffer size
    virtual Result GetPps(
        uint32* pBuffer,
        uint32* pSize) = 0;

    /// Returns H265 VPS header content from the encoder.
    ///
    /// @param [out]       pBuffer Pointer to write the VPS header content to.
    ///                            When this pointer is NULL, this function is used to query proper VPS size in byte.
    /// @param [in/output] pSize   [in] This is the buffer size (in byte) pointed by pBuffer.
    ///                            [out] The queried proper VPS size (in byte) when pBuffer is NULL
    ///                                  Returned VPS header content size (in byte) when pBuffer is valid.
    ///
    /// @returns Success           If VPS content was successfully returned in pBuffer when pBuffer is not NULL,
    ///                            in this case, pSize contains real content size.
    ///                            Or when pBuffer is NULL (query VPS header size).
    ///          NotReady          If this function was called before BeginEncode is called.
    ///          Unsupported       Function call is not supported.
    ///          ErrorInvalidMemorySize  Specified pBuffer is not big enough for VPS header content,
    ///                                  in this case, pSize will return the required buffer size
    virtual Result GetVps(
        uint32* pBuffer,
        uint32* pSize) = 0;

    /// Reports the create info of the video encoder.
    ///
    /// @returns the reference to VideoEncoderCreateInfo
    const VideoEncoderCreateInfo& GetVideoEncoderCreateInfo() const { return m_createInfo; }

    /// Returns the value of the associated arbitrary client data pointer.
    /// Can be used to associate arbitrary data with a particular PAL object.
    ///
    /// @returns Pointer to client data.
    void* GetClientData() const
    {
        return m_pClientData;
    }

    /// Sets the value of the associated arbitrary client data pointer.
    /// Can be used to associate arbitrary data with a particular PAL object.
    ///
    /// @param  [in]    pClientData     A pointer to arbitrary client data.
    void SetClientData(
        void* pClientData)
    {
        m_pClientData = pClientData;
    }

protected:
    /// @internal Constructor.
    ///
    /// @param [in] createInfo App-specified parameters describing the desired video encoder properties.
    IVideoEncoder(const VideoEncoderCreateInfo& createInfo) : m_createInfo(createInfo), m_pClientData(nullptr) { }

    /// @internal Destructor.  Prevent use of delete operator on this interface.  Client must destroy objects by
    /// explicitly calling IDestroyable::Destroy() and is responsible for freeing the system memory allocated for the
    /// object on their own.
    virtual ~IVideoEncoder() { }

    /// Retained VideoEncoder create info.
    const VideoEncoderCreateInfo m_createInfo;

private:
    /// @internal Client data pointer. This can have an arbitrary value and can be returned by calling GetClientData()
    /// and set via SetClientData().
    /// For non-top-layer objects, this will point to the layer above the current object.
    void* m_pClientData;
};
#endif

} // Pal
