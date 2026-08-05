/*
 * Copyright © Advanced Micro Devices, Inc., or its affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef EXT_IMAGE_IMAGE_MANAGER_GFX13_H_
#define EXT_IMAGE_IMAGE_MANAGER_GFX13_H_

#include "addrlib/inc/addrinterface.h"
#include "image_manager_gfx12.h"

namespace rocr {
namespace image {

class ImageManagerGfx13 : public ImageManagerGfx12 {
 public:
  ImageManagerGfx13();
  virtual ~ImageManagerGfx13();

  /// @brief Fill image structure with device specific image object.
  virtual hsa_status_t PopulateImageSrd(Image& image) const override;

  /// @brief Fill image structure with device specific image object using the given format.
  virtual hsa_status_t PopulateImageSrd(Image& image, const metadata_amd_t* desc) const override;

  /// @brief Modify device specific image object according to the specified
  /// new format.
  virtual hsa_status_t ModifyImageSrd(Image& image,
                                      hsa_ext_image_format_t& new_format) const override;

  /// @brief Fill sampler structure with device specific sampler object.
  virtual hsa_status_t PopulateSamplerSrd(Sampler& sampler) const override;

  /// @brief Fill mipmap structure with device specific mipmapped array object.
  virtual hsa_status_t PopulateMipmapSrd(MipmappedArray& mipmap_array) const override;

  /// @brief Fill mipmap structure with pre-computed AMD metadata descriptor.
  virtual hsa_status_t PopulateMipmapSrd(MipmappedArray& mipmap_array, const metadata_amd_t* desc) const override;

  /// @brief Create mip level view using SRD BASE_LEVEL/LAST_LEVEL fields
  virtual hsa_status_t PopulateMipLevelSrd(MipmappedArray& level_view,
        const MipmappedArray& mipmap_array, uint32_t mip_level) const override;

  /// @brief Fill image backing storage using agent copy.
  virtual hsa_status_t FillImage(const Image& image, const void* pattern,
                                 const hsa_ext_image_region_t& region) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ImageManagerGfx13);
};

}  // namespace image
}  // namespace rocr
#endif  // EXT_IMAGE_IMAGE_MANAGER_GFX13_H_
