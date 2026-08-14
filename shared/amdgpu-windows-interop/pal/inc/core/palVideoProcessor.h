/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palVideoProcessor.h
 * @brief Defines the Platform Abstraction Library (PAL) IVideoProcessor interface and related types.
 ***********************************************************************************************************************
 */

#pragma once

#if PAL_BUILD_VPE

#include "pal.h"
#include "palCmdBuffer.h"
#include "palGpuMemoryBindable.h"

namespace Pal
{

/// Video Processor Create Info
struct VideoProcessorCreateInfo
{
    EngineType  engineType;     ///< Engine type to create the decoder for
};

/// Color Space type values
enum class ColorSpace : uint8
{
    Ycbcr = 0,                  ///< YCbCr (e.g. nv12)
    Rgb,                        ///< YCbCr (e.g. nv12)
    Count
};

/// Color range type values
enum class ColorRange : uint8
{
    Full = 0,                   ///< Full range
    Studio,                     ///< Studio range
    Count
};

/// Cositing type values
enum class Cositing : uint8
{
    None = 0,                   ///< No cositing
    Left,                       ///< Left cositing
    TopLeft,                    ///< Right cositing
    Count
};

/// Color space type (standard) values
enum class ColorPrimaries : uint8
{
    Bt601 = 0,                  ///< BT 601 color space
    Bt709,                      ///< BT 709 color space
    Jfif,                       ///< JFIF color space
    Bt2020,                     ///< BT 2020 HDR color space
    Custom,                     ///< Custom / user controlled
    Count
};

/// Gamma type values
enum class GammaType : uint8
{
    Gamma22 = 0,                ///< Gamma 2.2
    Gamma2084,                  ///< Gamma 2084 (HDR)
    Gamma10,                    ///< Gamma 1.0
    GammaHLG,                   ///< Gamma HLG
    Gamma24,                    ///< Gamma 2.4
    Gamma2084Normalized,        ///< Normalized  Gamma 2084 used for ToneMap Input function
    Gamma709,                   ///< Gamma BT709
    GammaSRGB,                  ///< Gamma sRGB
    GammaCustom,                ///< Custom Gamma
    Count
};

/// Alpha Fill mode values
enum class AlphaFillMode :uint8
{
    Opaque = 0,                 ///< Opaque mode
    Background,                 ///< Background alpha mode
    Destination,                ///< Destination surface alpha mode
    SourceStream,               ///< Source Stream alpha
    Count
};

/**
 ***********************************************************************************************************************
 * @interface IVideoProcessor
 * @brief     Object containing video processor state. Separate concrete implementations will support various
 *            HW implementations.
 *
 * @see IDevice::CreateVideoProcessor()
 ***********************************************************************************************************************
 */
class IVideoProcessor : public IGpuMemoryBindable
{
public:
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

    /// Query video processor capability
    ///
    /// @param [in] pFrameInfo A pointer to video processor frame info.
    ///
    /// @returns Results whether video processor can be used or not.
    virtual Result QueryVideoProcessorCapability(
        const VideoProcessorFrameInfo* pFrameInfo) = 0;

protected:
    /// @internal Constructor.
    ///
    /// @param [in] createInfo App-specified parameters describing the desired video decoder properties.
    IVideoProcessor() : m_pClientData(nullptr) { }

    /// @internal Destructor.  Prevent use of delete operator on this interface.  Client must destroy objects by
    /// explicitly calling IDestroyable::Destroy() and is responsible for freeing the system memory allocated for the
    /// object on their own.
    virtual ~IVideoProcessor() { }

private:
    /// @internal Client data pointer. This can have an arbitrary value and can be returned by calling GetClientData()
    /// and set via SetClientData().
    /// For non-top-layer objects, this will point to the layer above the current object.
    void* m_pClientData;
};

} // Pal

#endif
