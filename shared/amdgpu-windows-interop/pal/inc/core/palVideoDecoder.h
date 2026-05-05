/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palVideoDecoder.h
 * @brief Defines the Platform Abstraction Library (PAL) IVideoDecoder interface and related types.
 ***********************************************************************************************************************
 */

#pragma once

#if PAL_BUILD_VIDEO

#include "pal.h"
#include "palCmdBuffer.h"
#include "palGpuMemoryBindable.h"
#include "palLiterals.h"

#if PAL_CLOSED_SOURCE
#include "drv_uvd_if.h"
#else
#include "drv_uvd_if_open.h"
#endif

namespace Pal
{


/// Decoder hw memory alignment requirement
constexpr gpusize HwMemoryAlignment = 256;
constexpr uint32 MacroblockAlignment = 16;

/// Hevc context buffer size
constexpr uint32 HevcSdbLeftCtxt    = 52 * Util::OneKibibyte;
constexpr uint32 HevcCtxtPerCtb     = 16;

/// Vc1 context buffer size
constexpr uint32 Vc1ItSurfacePerMb  = 64;
constexpr uint32 Vc1DbSurfacePerMb  = 128;
constexpr uint32 Vc1BpSurfacePerMb  = 16;
constexpr uint32 Vc1NumBpSurface    = 7;

// Maximum reference frames per codec
constexpr uint32 H264MaxRefFrames = 16;
constexpr uint32 HevcMaxRefFrames = 15;
constexpr uint32 Vp9MaxRefFrames = 8;
constexpr uint32 Av1MaxRefFrames = 8;
constexpr uint32 maxAv1DpbSize = 9;

// H264 Maximum Number of MB per Level
constexpr uint32 H264NumberOfMBForLevel10 = 99;
constexpr uint32 H264NumberOfMBForLevel1b = 99;
constexpr uint32 H264NumberOfMBForLevel11 = 396;
constexpr uint32 H264NumberOfMBForLevel12 = 396;
constexpr uint32 H264NumberOfMBForLevel13 = 396;
constexpr uint32 H264NumberOfMBForLevel20 = 396;
constexpr uint32 H264NumberOfMBForLevel21 = 792;
constexpr uint32 H264NumberOfMBForLevel22 = 1620;
constexpr uint32 H264NumberOfMBForLevel30 = 1620;
constexpr uint32 H264NumberOfMBForLevel31 = 3600;
constexpr uint32 H264NumberOfMBForLevel32 = 5120;
constexpr uint32 H264NumberOfMBForLevel40 = 8192;
constexpr uint32 H264NumberOfMBForLevel41 = 8192;
constexpr uint32 H264NumberOfMBForLevel42 = 8704;
constexpr uint32 H264NumberOfMBForLevel50 = 22080;
constexpr uint32 H264NumberOfMBForLevel51 = 36864;
constexpr uint32 H264NumberOfMBForLevel52 = 36864;

/// AMD H264 profile level structure
union PicParamsH264ProfileLevel
{
    struct
    {
        uint8   level   :   6;                  ///< Profile
        uint8   profile :   2;                  ///< Level
    };
    uint8       value;
};

/// AMD H264 extension structure
union PicParamsH264GapsSeek
{
    struct
    {
        uint8   gaps                :   1;      ///< Indicate if there is a gap from the previous pic.
        uint8   seek                :   1;      ///< Seeking is performed
        uint8   interlacedPicture   :   1;      ///< Interlaced picture
        uint8   lowLatency          :   1;      ///< Low latency decode
        uint8   reserved            :   3;      ///< Reserved
        uint8   extensionEnabled    :   1;      ///< AMD extension.
    };
    uint8       value;
};

/// AMD VC1 extension structure
union PicParamsVC1Extension
{
    struct
    {
        uint8   profile             :   2;      ///< profile
        uint8   interlacedPicture   :   1;      ///< Interlaced picture
        uint8   reserved            :   4;      ///< Reserved.
        uint8   extensionEnabled    :   1;      ///< AMD extension
    };
    uint8       value;
};

/// PPS Flags for VC1
union PpsFlagInfoVc1
{
    struct
    {
        uint32 vsTransform    : 1;  ///< Variable-sized transform coding is enabled for the sequence or not
        uint32 dquant         : 2;  ///< Indicate whether or not the quantization step size may vary within a frame
        uint32 extendedMv     : 1;  ///< Extended motion vectors are enabled or disabled
        uint32 fastUvMc       : 1;  ///< Control the subpixel interpolation and rounding of color-difference motion vectors
        uint32 loopFilter     : 1;  ///< Loop filtering is enabled for the sequence or not
        uint32 refDist        : 1;  ///< REFDIST syntax element is presented or not
        uint32 panScan        : 1;  ///< Indicate that pan/scan information is present in the
                                    ///  Picture headers within the entry point segment or not
        uint32 extendedDmv    : 1;  ///< Extended differential motion vector range
        uint32 quantizer      : 2;  ///< Quantizer used for the sequence. Implicit/Explicit/Nonunifom/Uniform Quantizer
        uint32 overlap        : 1;  ///< Overlap smoothing flag
        uint32                : 4;
        uint32 maxBframes     : 3;  ///< Maximum Number of consecutive B frames
        uint32 rangeRed       : 1;  ///< Range Reduction Flag
        uint32 syncMarker     : 1;  ///< Synchronization markers are presented in the bitstream or not
        uint32 multiRes       : 1;  ///< Indicate whether the frames may be coded at smaller resolutions than the
                                    ///  specified frame resolution
        uint32                : 2;
        uint32 rangeMapUV     : 3;  ///< Color-difference components of the decoded pictures within the entry point
                                    ///  segment shall be scaled according to the rangeMapUV value
        uint32 rangeMapUVFlag : 1;  ///< Range Mapping Color-Difference Flag
        uint32 rangeMapY      : 3;  ///< Luma components of the decoded pictures within the entry point segment shall
                                    ///  be scaled according to the rangeMapY value
        uint32 rangeMapYFlag  : 1;  ///< Range Mapping Luma Flag
    };

    uint32 u32All;
};

/// SPS Flags for VC1
union SpsFlagInfoVc1
{
    struct
    {
        uint32                  : 1;
        uint32 psf              : 1;  ///< Progressive Segmented Frame
        uint32 reserved         : 1;
        uint32 finterpFlag      : 1;  ///< Frame Interpolation Flag
        uint32 tfcntrFlag       : 1;  ///< Frame Counter Flag
        uint32 interlace        : 1;  ///< 0:pictures shall be coded as single frames using the progressive syntax
                                      ///  1:individual frames may be coded using the progressive or interlace syntax
        uint32 pulldown         : 1;  ///< Indicates the presence of syntax elements RPTFRM, or TFF and RFF
        uint32 postProc         : 1;  ///< Postprocessing Flag
        uint32 extensionSupport : 1;  ///< AMD extension support
    };

    uint32 u32All;
};

/// Deblock Parameters for VC1
union PicDeblockConfinedVc1
{
    struct
    {
        uint8 extendedDmv : 1;  ///< Extended motion vectors are enabled or disabled
        uint8 psf         : 1;  ///< Progressive Segmented Frame
        uint8 reserved    : 1;
        uint8 finterpflag : 1;  ///< Frame Interpolation Flag
        uint8 tfcntrflag  : 1;  ///< Frame Counter Flag
        uint8 interlace   : 1;  ///< 0:pictures shall be coded as single frames using the progressive syntax
                                ///  1:individual frames may be coded using the progressive or interlace syntax
        uint8 pulldown    : 1;  ///< Indicates the presence of syntax elements RPTFRM, or TFF and RFF
        uint8 postproc    : 1;  ///< Postprocessing Flag
    };

    uint8 u8All;
};

/// PicSpatialResid8 for VC1
union PicSpatialResid8Vc1
{
    struct
    {
        uint8 vsTransform : 1;  ///< Variable-sized transform coding is enabled for the sequence or not
        uint8 dquant      : 2;  ///< Indicate whether or not the quantization step size may vary within a frame
        uint8 extendedMv  : 1;  ///< Extended motion vectors are enabled or disabled
        uint8 fastUvMc    : 1;  ///< Control the subpixel interpolation and rounding of color-difference motion vectors
        uint8 loopFilter  : 1;  ///< Loop filtering is enabled for the sequence or not
        uint8 refDist     : 1;  ///< REFDIST syntax element is presented or not
        uint8 panScan     : 1;  ///< Indicate that pan/scan information is present in the
                                ///  picture headers within the entry point segment or not
    };

    uint8 u8All;
};

/// Range Mapping Parameters for VC1
union PicObmcVc1
{
    struct
    {
        uint8 rangeMapUV     : 3;  ///< Color-difference components of the decoded pictures within the entry point
                                   ///  segment shall be scaled according to the rangeMapUV value
        uint8 rangeMapUVFlag : 1;  ///< Range Mapping Color-Difference Flag
        uint8 rangeMapY      : 3;  ///< Luma components of the decoded pictures within the entry point segment shall
                                   ///  be scaled according to the rangeMapY value
        uint8 rangeMapYFlag  : 1;  ///< Range Mapping Luma Flag
    };

    uint8 u8All;
};

/// PicOverflowBlocks for VC1
union PicOverflowBlocksVc1
{
    struct
    {
        uint8 maxBframes : 3;  ///< Maximum Number of consecutive B frames
        uint8 rangeRed   : 1;  ///< Range Reduction Flag
        uint8 syncMarker : 1;  ///< Synchronization markers are presented in the bitstream or not
        uint8 multiRes   : 1;  ///< Indicate whether the frames may be coded at smaller resolutions than the
                               ///  specified frame resolution
        uint8 quantizer  : 2;  ///< Quantizer used for the sequence. Implicit/Explicit/Nonunifom/Uniform Quantizer
    };

    uint8 u8All;
};

/// PicDeblocked for VC1
union PicDeblockedVc1
{
    struct
    {
        uint8         : 6;
        uint8 overlap : 1;  ///< Overlap smoothing flag
        uint8         : 1;
    };

    uint8 u8All;
};

/// PPS Flags for H264
union PpsFlagInfoH264
{
    struct
    {
        uint32 transform8x8Mode               : 1;  ///< 8x8 transform decoding process may be in use or not
        uint32 redundantPicCntPresent         : 1;  ///< redundant_pic_cnt syntax element is present in all
                                                    ///  slice headers or not
        uint32 constrainedIntraPred           : 1;  ///< Equal to 1 means prediction of macroblocks coded using Intra
                                                    ///  macroblock prediction modes only uses residual data and
                                                    ///< decoded samples from I or SI macroblock types.
        uint32 deblockingFilterControlPresent : 1;  ///< Equal to 1 specifies that a set of syntax elements controlling
                                                    ///  the characteristics of the deblocking filter is present in
                                                    ///  the slice header
        uint32 weightedBipredIdc              : 2;  ///< 0:the default weighted prediction, 1:explicit weighted
                                                    ///  prediction, 2:implicit weighted prediction shall be applied
                                                    ///  to B slices
        uint32 weightedPred                   : 1;  ///< 0/1:default/explicit weighted prediction shall be applied to
                                                    ///  P slices
        uint32 picOrderPresent                : 1;  ///< Picture order present flag
        uint32 entropyCodingMode              : 1;  ///< entropy decoding mode flag (i.e. CAVLC/CABAC)
    };

    uint32 u32All;
};

/// SPS Flags for H264
union SpsFlagInfoH264
{
    struct
    {
        uint32 direct8x8Inference         : 1;  ///< motion vector for 4x4/8x8 region
        uint32 mbAdaptiveFrameField       : 1;  ///< current macroblock pair is a frame/field macroblock pair
        uint32 frameMbsOnly               : 1;  ///< flag indicating whether the macroblock was coded in frame mode
        uint32 delataPicOrderAlwaysZero   : 1;  ///< Specifies delta_pic_order_cnt[0] and delta_pic_order_cnt[1]
                                                ///  are presented in the slice headers or not
        uint32 residualColourTransform    : 1;  ///< Deprecated, so must equal to 0
        uint32 gapsInFrameNumValueAllowed : 1;  ///< Any gap from the previous pic
        uint32 firstPictureAfterSeek      : 1;  ///< Seeking is performed
        uint32 extensionSupport           : 1;  ///< AMD extension support
    };

    uint32 u32All;
};

///PPS Flags for HEVC
union PpsFlagInfoHEVC
{
    struct
    {
        uint32 dependentSliceSegmentsEnabled      : 1;  ///< dependent_slice_segments_enabled_flag
        uint32 outputFlagPresent                  : 1;  ///< output_flag_present_flag
        uint32 signDataHidingEnable               : 1;  ///< sign bit hiding is disabled/enabled
        uint32 cabacInitPresent                   : 1;  ///< cabac_init_present_flag
        uint32 constrainedIntraPred               : 1;  ///< Constrained intra prediction mode
        uint32 transformSkipEnabled               : 1;  ///< transform_skip_flag presented or not
        uint32 cuQpDeltaEnabled                   : 1;  ///< cu_qp_delta_enabled_flag
        uint32 ppsSliceChromaQpOffsetsPresent     : 1;  ///< pps_slice_chroma_qp_offsets_present_flag
        uint32 weightedPred                       : 1;  ///< weighted prediction applied to P slices or not
        uint32 weightedBiPred                     : 1;  ///< default/weighted prediction for B slices
        uint32 transQuantBypassEnabled            : 1;  ///< transquant_bypass_enabled_flag
        uint32 tilesEnabled                       : 1;  ///< tiles_enabled_flag
        uint32 entropyCodingSyncEnabled           : 1;  ///< entropy_coding_sync_enabled_flag
        uint32 uniformSpacing                     : 1;  ///< uniform_spacing_flag
        uint32 loopFilterAcrossTilesEnabled       : 1;  ///< loop_filter_across_tiles_enabled_flag
        uint32 ppsLoopFilterAcrossSlicesEnabled   : 1;  ///< pps_loop_filter_across_slices_enabled_flag
        uint32 deblockingFilterOverrideEnabled    : 1;  ///< deblocking_filter_override_enabled_flag
        uint32 ppsDeblockingFilterDisabled        : 1;  ///< pps_deblocking_filter_disabled_flag
        uint32 listsModificationPresent           : 1;  ///< lists_modification_present_flag
        uint32 sliceSegmentHeaderExtensionPresent : 1;  ///< slice_segment_header_extension_present_flag
    };

    uint32 u32All;
};

/// SPS Flags for HEVC
union SpsFlagInfoHEVC
{
    struct
    {
        uint32 scalingListEnabled          : 1;  ///< scaling_list_enabled_flag
        uint32 ampEnabled                  : 1;  ///< amp_enabled_flag
        uint32 sampleAdaptiveOffsetEnabled : 1;  ///< sample_adaptive_offset_enabled_flag
        uint32 pcmEnabled                  : 1;  ///< pcm_enabled_flag
        uint32 pcmLoopFilterDisabled       : 1;  ///< pcm_loop_filter_disabled_flag
        uint32 longTermRefPicsPresent      : 1;  ///< long_term_ref_pics_present_flag
        uint32 spsTemporalMvpEnabled       : 1;  ///< sps_temporal_mvp_enabled_flag
        uint32 strongIntraSmoothingEnabled : 1;  ///< strong_intra_smoothing_enabled_flag
        uint32 separateColourPlane         : 1;  ///< separate_colour_plane_flag
        uint32 naRefCzWorkaround           : 1;  ///< cz workaround flag
        uint32 vaapiDirectReflist          : 1;  ///< vaapi direct reflist flag
    };

    uint32 u32All;
};

/// VP9 frame header flags
union Vp9FrameHeaderFlags
{
    struct
    {
        uint32 showExistingFrame          : 1;  ///< Whether the current frame is intended to be output and displayed
                                                ///  after its decoding is completed
        uint32 frameType                  : 1;  ///< Frame type of the current frame (i.e. key frame or non key frame)
        uint32 errorResilientMode         : 1;  ///< Frame level error resilient mode flag
        uint32 intraOnly                  : 1;  ///< Indicates the frame is an intra-only frame or inter frame
        uint32 allowHighPrecisionMv       : 1;  ///< Motion vectors are specified to quarter/eighth pel precision
        uint32 refreshFrameContext        : 1;  ///< Probabilities computed for this frame should be stored/discarded
        uint32 frameParallelDecodingMode  : 1;  ///< Parallel decoding mode is enabled/disabled
        uint32 segmentationEnabled        : 1;  ///< Current frame makes use of the segmentation tool or not
        uint32 segmentationUpdateMap      : 1;  ///< Equal to 1 indicates that the segmentation map should be updated
                                                ///  during the decoding of this frame.Equal to 0 means that the
                                                ///  segmentation map from the previous frame is used.
        uint32 segmentationTemporalUpdate : 1;  ///< Equal to 1 indicates that the updates to the segmentation map are
                                                ///  coded relative to the existing segmentation map.Equal to 0
                                                ///  indicates that the new segmentation map is coded without reference
                                                ///  to the existing segmentation map.
        uint32 segmentationUpdateData     : 1;  ///< Equal to 1 indicates that new parameters are about to be specified
                                                ///  for each segment.Equal to 0 indicates that the segmentation
                                                ///  parameters should keep their existing values.
        uint32 modeRefDeltaEnabled        : 1;  ///< Equal to 1 means that the filter level depends on the mode and
                                                ///  reference frame used to predict a block.Equal to 0  means that
                                                ///  the filter level does not depend on the mode and reference frame
        uint32 modeRefDeltaUpdate         : 1;  ///< Equal to 1 means that the bitstream contains additional syntax
                                                ///  elements that specify which mode and reference frame deltas are
                                                ///  to be updated.Equal to 0 means that these syntax elements are
                                                ///  not present.
        uint32 usePrevInFindMvRefs        : 1;  ///< Indicates whether the previous mode information context from the
                                                ///  last decoded frame can be used or not
    };

    uint32 u32All;
};

/// VP9 segment level features.
enum SegLevelFeatures : uint32
{
    SegLevelAltQuant    = 0x0,      ///< Use alternate Quantizer.
    SegLevelAltLf       = 0x1,      ///< Use alternate loop filter value.
    SegLevelRefFrame    = 0x2,      ///< Optional Segment reference frame.
    SegLevelSkip        = 0x3,      ///< Optional Segment (0,0) + skip mode.
    SegLevelCount       = 0x4
};

/// UVD video codec-specific parameters controlling frame decode.  The data structures for each codec are currently
/// defined in drv_uvd_if.h.
struct VideoCodecInfo
{
    union
    {
        avc_t               avcCodecData;           ///< H264 codec info structure
        vc1_t               vc1CodecData;           ///< Vc1 codec info structure
        wmv_t               wmvCodecData;           ///< Wmv9 codec info structure
        mpeg2_idct_t        mpeg2IdctCodecData;     ///< Mpeg2 IDCT codec info structure
        mpeg2_vld_t         mpeg2VldCodecData;      ///< Mpeg2 VLD codec info structure
        mpeg4_asp_vld_t     mpeg4CodecData;         ///< Mpeg4 VLD codec info structure
        hevc_t              hevcCodecData;          ///< Hevc codec info structure
        vp9_t               vp9CodecData;           ///< Vp9 codec info structure
        av1_t               av1CodecData;           ///< Av1 codec info structure
    };
};

/// UVD video codec auxiliary parameters required by specific codec.  The data structures for each codec are currently
/// defined in drv_uvd_if.h.
struct VideoCodecAuxInfo
{
    union
    {
        uvd_its_t           h264AuxData;            ///< H264 quantization matrix buffer
        uvd_its_hevc_t      hevcAuxData;            ///< Hevc quantization matrix buffer
        vp9_probs_segment_t vp9AuxData;             ///< Vp9 probability and segmentation buffer
        av1_segment_fg_t    av1AuxData;             ///< Av1 segmentation buffer
    };
};

/// Specifies video usage type
enum class VideoUsage : uint32
{
    Normal                      = 0x0,              ///< Normal decoding
    Power                       = 0x1,              ///< Minimizing V/D clock
    Quality                     = 0x2,              ///< Maximizing V/D clock
    Count
};

/// Specifies properties for creation of a @ref IVideoDecoder object. Input structure to
/// IDevice::CreateVideoDecoder().
struct VideoDecoderCreateInfo
{
    EngineType              engineType;             ///< Engine type to create the decoder for
    VideoDecodeType         decodeType;             ///< Video Codec type supported by UVD
};

struct VideoDecoderGpuMemInfo
{
    EngineType              engineType;             ///< Engine type to create the decoder for
    VideoDecodeType         decodeType;             ///< Video Codec type supported by UVD
    Extent2d                decodeExtent;           ///< Decode resoution set by the application.
    uint32                  maxDpbFrames;           ///< Maximum number of decode picture buffer(DPB) frames. It is
                                                    ///  used to calculate DPB size
    float                   frameRate;              ///< Frames per second
    uint32                  bitRate;                ///< Bitrate of the stream
};

/**
 ***********************************************************************************************************************
 * @interface IVideoDecoder
 * @brief     Object containing video decoder state. Separate concrete implementations will support various
 *            HW implementations.
 *
 * @see IDevice::CreateVideoDecoder()
 ***********************************************************************************************************************
 */
class IVideoDecoder : public IGpuMemoryBindable
{
public:
    /// Reports the create info of the video decoder.
    ///
    /// @returns the reference to VideoDecoderCreateInfo
    const VideoDecoderCreateInfo& GetVideoDecoderCreateInfo() const { return m_createInfo; }

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
    /// @param [in] createInfo App-specified parameters describing the desired video decoder properties.
    IVideoDecoder(const VideoDecoderCreateInfo& createInfo) : m_createInfo(createInfo), m_pClientData(nullptr) { }

    /// @internal Destructor.  Prevent use of delete operator on this interface.  Client must destroy objects by
    /// explicitly calling IDestroyable::Destroy() and is responsible for freeing the system memory allocated for the
    /// object on their own.
    virtual ~IVideoDecoder() { }

    /// Retained VideoDecoder create info.
    const VideoDecoderCreateInfo m_createInfo;

private:
    /// @internal Client data pointer. This can have an arbitrary value and can be returned by calling GetClientData()
    /// and set via SetClientData().
    /// For non-top-layer objects, this will point to the layer above the current object.
    void* m_pClientData;
};

} // Pal

#endif
