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

#ifndef _GFX13_DEF_H_
#define _GFX13_DEF_H_

#include "linux/soc24_enum.h"
#include "util/soc15_common.h"
#include "util/reg_offsets.h"
#include "linux/registers/gc/gc_13_0_1_offset.h"
#include "linux/registers/gc/gc_13_0_1_sh_mask.h"
#include "linux/registers/athub/athub_5_0_1_offset.h"
#include "linux/registers/athub/athub_5_0_1_sh_mask.h"
#undef regDFLL_DFLL_REG_ADDR
#undef regDFLL_DFLL_WR_DATA
#undef regXVMIN_XVMIN_REG_ADDR
#undef regXVMIN_XVMIN_WR_DATA
#undef regDBGU_PORT_A_INDEX
#undef regDBGU_PORT_A_DATA_LO
#undef regDBGU_PORT_A_DATA_HI
#undef regDBGU_PORT_B_INDEX
#undef regDBGU_PORT_B_DATA_LO
#undef regDBGU_PORT_B_DATA_HI
#undef regDBGU_PORT_C_INDEX
#undef regDBGU_PORT_C_DATA_LO
#undef regDBGU_PORT_C_DATA_HI
#undef regDBGU_PORT_D_INDEX
#undef regDBGU_PORT_D_DATA_LO
#undef regDBGU_PORT_D_DATA_HI
#undef regMSFLL_MSFLL_REG_ADDR
#undef regMSFLL_MSFLL_WR_DATA
#undef XVMIN_REG5__RESERVED__SHIFT
#undef XVMIN_REG5__RESERVED_MASK
#undef GFX_BP_OBSERVE__Reserved__SHIFT
#undef GFX_BP_OBSERVE__Reserved_MASK

#include "linux/packets/nvd.h"
#include "gfxip/gfx13/gfx13_block_info.h"
using namespace gfxip::gfx13;
#include "gfxip/gfx13/gfx13_primitives.h"
#include "gfxip/gfx13/gfx13_block_table.h"

#endif  // _GFX13_DEF_H_
