// MIT License
//
// Copyright (c) 2017-2025 Advanced Micro Devices, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "ip_discovery.h"
#include "linux/registers/soc_v1_0_ip_offset.h"

/* soc v1_0 model used by MI450 emulator */
const reg_base_offset_table* soc_v1_0_reg_base_init() {
  static_assert(HWIP_MAX_INSTANCE >= MAX_INSTANCE,
                "HWIP_MAX_INSTANCE must be greater than MAX_INSTANCE");
  static_assert(HWIP_MAX_SEGMENT >= MAX_SEGMENT,
                "HWIP_MAX_SEGMENT must be greater than MAX_SEGMENT");

  static const auto* soc_v1_0_reg_table = []() {
    auto* reg_table = new reg_base_offset_table();

    // helper lambda to initialize blocks
    auto init_hwip = [&](amd_hw_ip_block_type hwip, const auto& base) {
      for (uint32_t i = 0; i < MAX_INSTANCE; ++i) {
        std::copy(std::begin(base.instance[i].segment), std::end(base.instance[i].segment),
                  std::begin(reg_table->reg_offset[hwip][i]));
      }
    };

    init_hwip(ATHUB_HWIP, ATHUB_MID_BASE);
    init_hwip(MMHUB_HWIP, MMHUB_MID_BASE);
    init_hwip(NBIO_HWIP, NBIO_MID_BASE);
    init_hwip(MP0_HWIP, ASP_MID_BASE);
    init_hwip(MP1_HWIP, MP1_MID_BASE);
    init_hwip(VCN_HWIP, VCN_MID_BASE);
    init_hwip(SMUIO_HWIP, SMUIO_MID_BASE);
    init_hwip(OSSSYS_HWIP, OSSSYS_MID_BASE);
    init_hwip(PCIE_HWIP, PCIE_MID_BASE);
    init_hwip(LSDMA_HWIP, LSDMA_MID_BASE);
    init_hwip(GC_HWIP, GC_XCD_BASE);
    init_hwip(SDMA0_HWIP, GC_XCD_BASE);
    init_hwip(SDMA1_HWIP, GC_XCD_BASE);
    for (uint32_t i = 0; i < 2; i++)
      std::copy(std::begin(GC_AID_BASE.instance[i].segment),
                std::end(GC_AID_BASE.instance[i].segment),
                std::begin(reg_table->reg_offset[GC_HWIP][8+i]));

    return reg_table;
  }();

  return soc_v1_0_reg_table;
}
