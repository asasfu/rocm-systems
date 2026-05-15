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
#ifndef _GFX13_BLOCKTABLE_UMC_H_
#define _GFX13_BLOCKTABLE_UMC_H_

#define REG_INFO_PERFMON(INST, UMC_CH, INDEX)                         \
 {REG_32B_ADDR(UMC, INST, regUMCCH##UMC_CH##_PerfMonCtl##INDEX), REG_32B_ADDR(UMC, INST, regUMCCH##UMC_CH##_PerfMonCtlClk),   \
  REG_32B_ADDR(UMC, INST, regUMCCH##UMC_CH##_PerfMonCtr##INDEX##_Lo), REG_32B_ADDR(UMC, INST, regUMCCH##UMC_CH##_PerfMonCtr##INDEX##_Hi), REG_32B_NULL}
#define REG_INFO_PERFMON_CLK(INST, UMC_CH)                         \
 {REG_32B_ADDR(UMC, INST, regUMCCH##UMC_CH##_PerfMonCtlClk), REG_32B_ADDR(UMC, INST, regUMCCH##UMC_CH##_PerfMonCtlClk),   \
  REG_32B_ADDR(UMC, INST, regUMCCH##UMC_CH##_PerfMonCtrClk_Lo), REG_32B_ADDR(UMC, INST, regUMCCH##UMC_CH##_PerfMonCtrClk_Hi), REG_32B_NULL}

namespace gfxip {
namespace gfx13 {

  // UMC
  static const CounterRegInfo UmcCounterRegAddr[] = {
    REG_INFO_PERFMON(0, 0, 1),
    REG_INFO_PERFMON(0, 0, 2),
    REG_INFO_PERFMON(0, 0, 3),
    REG_INFO_PERFMON(0, 0, 4),
    REG_INFO_PERFMON(0, 0, 5),
    REG_INFO_PERFMON(0, 0, 6),
    REG_INFO_PERFMON(0, 0, 7),
    REG_INFO_PERFMON(0, 0, 8),
    REG_INFO_PERFMON(0, 0, 9),
    REG_INFO_PERFMON(0, 0, 10),
    REG_INFO_PERFMON(0, 0, 11),
    REG_INFO_PERFMON(0, 0, 12),
    REG_INFO_PERFMON_CLK(0, 0),

    REG_INFO_PERFMON(0, 1, 1),
    REG_INFO_PERFMON(0, 1, 2),
    REG_INFO_PERFMON(0, 1, 3),
    REG_INFO_PERFMON(0, 1, 4),
    REG_INFO_PERFMON(0, 1, 5),
    REG_INFO_PERFMON(0, 1, 6),
    REG_INFO_PERFMON(0, 1, 7),
    REG_INFO_PERFMON(0, 1, 8),
    REG_INFO_PERFMON(0, 1, 9),
    REG_INFO_PERFMON(0, 1, 10),
    REG_INFO_PERFMON(0, 1, 11),
    REG_INFO_PERFMON(0, 1, 12),
    REG_INFO_PERFMON_CLK(0, 1),

    REG_INFO_PERFMON(1, 0, 1),
    REG_INFO_PERFMON(1, 0, 2),
    REG_INFO_PERFMON(1, 0, 3),
    REG_INFO_PERFMON(1, 0, 4),
    REG_INFO_PERFMON(1, 0, 5),
    REG_INFO_PERFMON(1, 0, 6),
    REG_INFO_PERFMON(1, 0, 7),
    REG_INFO_PERFMON(1, 0, 8),
    REG_INFO_PERFMON(1, 0, 9),
    REG_INFO_PERFMON(1, 0, 10),
    REG_INFO_PERFMON(1, 0, 11),
    REG_INFO_PERFMON(1, 0, 12),
    REG_INFO_PERFMON_CLK(1, 0),

    REG_INFO_PERFMON(1, 1, 1),
    REG_INFO_PERFMON(1, 1, 2),
    REG_INFO_PERFMON(1, 1, 3),
    REG_INFO_PERFMON(1, 1, 4),
    REG_INFO_PERFMON(1, 1, 5),
    REG_INFO_PERFMON(1, 1, 6),
    REG_INFO_PERFMON(1, 1, 7),
    REG_INFO_PERFMON(1, 1, 8),
    REG_INFO_PERFMON(1, 1, 9),
    REG_INFO_PERFMON(1, 1, 10),
    REG_INFO_PERFMON(1, 1, 11),
    REG_INFO_PERFMON(1, 1, 12),
    REG_INFO_PERFMON_CLK(1, 1),

    REG_INFO_PERFMON(2, 0, 1),
    REG_INFO_PERFMON(2, 0, 2),
    REG_INFO_PERFMON(2, 0, 3),
    REG_INFO_PERFMON(2, 0, 4),
    REG_INFO_PERFMON(2, 0, 5),
    REG_INFO_PERFMON(2, 0, 6),
    REG_INFO_PERFMON(2, 0, 7),
    REG_INFO_PERFMON(2, 0, 8),
    REG_INFO_PERFMON(2, 0, 9),
    REG_INFO_PERFMON(2, 0, 10),
    REG_INFO_PERFMON(2, 0, 11),
    REG_INFO_PERFMON(2, 0, 12),
    REG_INFO_PERFMON_CLK(2, 0),

    REG_INFO_PERFMON(2, 1, 1),
    REG_INFO_PERFMON(2, 1, 2),
    REG_INFO_PERFMON(2, 1, 3),
    REG_INFO_PERFMON(2, 1, 4),
    REG_INFO_PERFMON(2, 1, 5),
    REG_INFO_PERFMON(2, 1, 6),
    REG_INFO_PERFMON(2, 1, 7),
    REG_INFO_PERFMON(2, 1, 8),
    REG_INFO_PERFMON(2, 1, 9),
    REG_INFO_PERFMON(2, 1, 10),
    REG_INFO_PERFMON(2, 1, 11),
    REG_INFO_PERFMON(2, 1, 12),
    REG_INFO_PERFMON_CLK(2, 1),

    REG_INFO_PERFMON(3, 0, 1),
    REG_INFO_PERFMON(3, 0, 2),
    REG_INFO_PERFMON(3, 0, 3),
    REG_INFO_PERFMON(3, 0, 4),
    REG_INFO_PERFMON(3, 0, 5),
    REG_INFO_PERFMON(3, 0, 6),
    REG_INFO_PERFMON(3, 0, 7),
    REG_INFO_PERFMON(3, 0, 8),
    REG_INFO_PERFMON(3, 0, 9),
    REG_INFO_PERFMON(3, 0, 10),
    REG_INFO_PERFMON(3, 0, 11),
    REG_INFO_PERFMON(3, 0, 12),
    REG_INFO_PERFMON_CLK(3, 0),

    REG_INFO_PERFMON(3, 1, 1),
    REG_INFO_PERFMON(3, 1, 2),
    REG_INFO_PERFMON(3, 1, 3),
    REG_INFO_PERFMON(3, 1, 4),
    REG_INFO_PERFMON(3, 1, 5),
    REG_INFO_PERFMON(3, 1, 6),
    REG_INFO_PERFMON(3, 1, 7),
    REG_INFO_PERFMON(3, 1, 8),
    REG_INFO_PERFMON(3, 1, 9),
    REG_INFO_PERFMON(3, 1, 10),
    REG_INFO_PERFMON(3, 1, 11),
    REG_INFO_PERFMON(3, 1, 12),
    REG_INFO_PERFMON_CLK(3, 1),

    REG_INFO_PERFMON(4, 0, 1),
    REG_INFO_PERFMON(4, 0, 2),
    REG_INFO_PERFMON(4, 0, 3),
    REG_INFO_PERFMON(4, 0, 4),
    REG_INFO_PERFMON(4, 0, 5),
    REG_INFO_PERFMON(4, 0, 6),
    REG_INFO_PERFMON(4, 0, 7),
    REG_INFO_PERFMON(4, 0, 8),
    REG_INFO_PERFMON(4, 0, 9),
    REG_INFO_PERFMON(4, 0, 10),
    REG_INFO_PERFMON(4, 0, 11),
    REG_INFO_PERFMON(4, 0, 12),
    REG_INFO_PERFMON_CLK(4, 0),

    REG_INFO_PERFMON(4, 1, 1),
    REG_INFO_PERFMON(4, 1, 2),
    REG_INFO_PERFMON(4, 1, 3),
    REG_INFO_PERFMON(4, 1, 4),
    REG_INFO_PERFMON(4, 1, 5),
    REG_INFO_PERFMON(4, 1, 6),
    REG_INFO_PERFMON(4, 1, 7),
    REG_INFO_PERFMON(4, 1, 8),
    REG_INFO_PERFMON(4, 1, 9),
    REG_INFO_PERFMON(4, 1, 10),
    REG_INFO_PERFMON(4, 1, 11),
    REG_INFO_PERFMON(4, 1, 12),
    REG_INFO_PERFMON_CLK(4, 1),

    REG_INFO_PERFMON(5, 0, 1),
    REG_INFO_PERFMON(5, 0, 2),
    REG_INFO_PERFMON(5, 0, 3),
    REG_INFO_PERFMON(5, 0, 4),
    REG_INFO_PERFMON(5, 0, 5),
    REG_INFO_PERFMON(5, 0, 6),
    REG_INFO_PERFMON(5, 0, 7),
    REG_INFO_PERFMON(5, 0, 8),
    REG_INFO_PERFMON(5, 0, 9),
    REG_INFO_PERFMON(5, 0, 10),
    REG_INFO_PERFMON(5, 0, 11),
    REG_INFO_PERFMON(5, 0, 12),
    REG_INFO_PERFMON_CLK(5, 0),

    REG_INFO_PERFMON(5, 1, 1),
    REG_INFO_PERFMON(5, 1, 2),
    REG_INFO_PERFMON(5, 1, 3),
    REG_INFO_PERFMON(5, 1, 4),
    REG_INFO_PERFMON(5, 1, 5),
    REG_INFO_PERFMON(5, 1, 6),
    REG_INFO_PERFMON(5, 1, 7),
    REG_INFO_PERFMON(5, 1, 8),
    REG_INFO_PERFMON(5, 1, 9),
    REG_INFO_PERFMON(5, 1, 10),
    REG_INFO_PERFMON(5, 1, 11),
    REG_INFO_PERFMON(5, 1, 12),
    REG_INFO_PERFMON_CLK(5, 1),

    REG_INFO_PERFMON(6, 0, 1),
    REG_INFO_PERFMON(6, 0, 2),
    REG_INFO_PERFMON(6, 0, 3),
    REG_INFO_PERFMON(6, 0, 4),
    REG_INFO_PERFMON(6, 0, 5),
    REG_INFO_PERFMON(6, 0, 6),
    REG_INFO_PERFMON(6, 0, 7),
    REG_INFO_PERFMON(6, 0, 8),
    REG_INFO_PERFMON(6, 0, 9),
    REG_INFO_PERFMON(6, 0, 10),
    REG_INFO_PERFMON(6, 0, 11),
    REG_INFO_PERFMON(6, 0, 12),
    REG_INFO_PERFMON_CLK(6, 0),

    REG_INFO_PERFMON(6, 1, 1),
    REG_INFO_PERFMON(6, 1, 2),
    REG_INFO_PERFMON(6, 1, 3),
    REG_INFO_PERFMON(6, 1, 4),
    REG_INFO_PERFMON(6, 1, 5),
    REG_INFO_PERFMON(6, 1, 6),
    REG_INFO_PERFMON(6, 1, 7),
    REG_INFO_PERFMON(6, 1, 8),
    REG_INFO_PERFMON(6, 1, 9),
    REG_INFO_PERFMON(6, 1, 10),
    REG_INFO_PERFMON(6, 1, 11),
    REG_INFO_PERFMON(6, 1, 12),
    REG_INFO_PERFMON_CLK(6, 1),

    REG_INFO_PERFMON(7, 0, 1),
    REG_INFO_PERFMON(7, 0, 2),
    REG_INFO_PERFMON(7, 0, 3),
    REG_INFO_PERFMON(7, 0, 4),
    REG_INFO_PERFMON(7, 0, 5),
    REG_INFO_PERFMON(7, 0, 6),
    REG_INFO_PERFMON(7, 0, 7),
    REG_INFO_PERFMON(7, 0, 8),
    REG_INFO_PERFMON(7, 0, 9),
    REG_INFO_PERFMON(7, 0, 10),
    REG_INFO_PERFMON(7, 0, 11),
    REG_INFO_PERFMON(7, 0, 12),
    REG_INFO_PERFMON_CLK(7, 0),

    REG_INFO_PERFMON(7, 1, 1),
    REG_INFO_PERFMON(7, 1, 2),
    REG_INFO_PERFMON(7, 1, 3),
    REG_INFO_PERFMON(7, 1, 4),
    REG_INFO_PERFMON(7, 1, 5),
    REG_INFO_PERFMON(7, 1, 6),
    REG_INFO_PERFMON(7, 1, 7),
    REG_INFO_PERFMON(7, 1, 8),
    REG_INFO_PERFMON(7, 1, 9),
    REG_INFO_PERFMON(7, 1, 10),
    REG_INFO_PERFMON(7, 1, 11),
    REG_INFO_PERFMON(7, 1, 12),
    REG_INFO_PERFMON_CLK(7, 1),
  };

  // Counter block UMC
  static const GpuBlockInfo UmcCounterBlockInfo = {
    "UMC",
    __BLOCK_ID_HSA(UMC),
    1,
    UmcCounterBlockMaxEvent,
    sizeof(UmcCounterRegAddr)/sizeof(CounterRegInfo),//UmcCounterBlockNumCounters,
    UmcCounterRegAddr,
    gfx13_cntx_prim::umc_select_value,
    CounterBlockUmcAttr | CounterBlockAidAttr | CounterBlockExplInstAttr,
    REG_32B_NULL};

}  // namespace gfx13

}  // namespace gfxip

#undef REG_INFO_PERFMON
#undef REG_INFO_PERFMON_CLK

#endif  // _GFX13_BLOCKTABLE_UMC_H_
