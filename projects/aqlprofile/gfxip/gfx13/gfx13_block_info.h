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

#ifndef _GFX13_BLOCKINFO_H_
#define _GFX13_BLOCKINFO_H_

namespace gfxip {
namespace gfx13 {
#define __BLOCK_ID_HSA(block) HSA_VEN_AMD_AQLPROFILE_BLOCK_NAME_##block
#define __BLOCK_ID(block) AQLPROFILE_BLOCK_NAME_##block
enum CounterBlockId {
  // Counters retrieved by KFD
  IommuV2CounterBlockId = AQLPROFILE_BLOCKS_NUMBER,
  KernelDriverCounterBlockId,

  CpPipeStatsCounterBlockId,
  HwInfoCounterBlockId,

  LastCounterBlockId = HwInfoCounterBlockId,
};

// Define SPM Counter BlockId
enum SpmGlobalBlockId {
  SPM_GLOBAL_BLOCK_NAME_FIRST = 0,
  SPM_GLOBAL_BLOCK_NAME_CPG = SPM_GLOBAL_BLOCK_NAME_FIRST,
  SPM_GLOBAL_BLOCK_NAME_CPC,
  SPM_GLOBAL_BLOCK_NAME_CPF,
  SPM_GLOBAL_BLOCK_NAME_GDS,
  SPM_GLOBAL_BLOCK_NAME_GCR,
  SPM_GLOBAL_BLOCK_NAME_PH,
  SPM_GLOBAL_BLOCK_NAME_GE1,
  SPM_GLOBAL_BLOCK_NAME_GL2A,
  SPM_GLOBAL_BLOCK_NAME_GL2C,
  SPM_GLOBAL_BLOCK_NAME_SDMA,
  SPM_GLOBAL_BLOCK_NAME_GUS,
  SPM_GLOBAL_BLOCK_NAME_EA,
  SPM_GLOBAL_BLOCK_NAME_CHA,
  SPM_GLOBAL_BLOCK_NAME_CHC,
  SPM_GLOBAL_BLOCK_NAME_CHCG,
  SPM_GLOBAL_BLOCK_NAME_ATCL2,
  SPM_GLOBAL_BLOCK_NAME_VML2,
  SPM_GLOBAL_BLOCK_NAME_GE2_SE,
  SPM_GLOBAL_BLOCK_NAME_GE2_DIST,
  SPM_GLOBAL_BLOCK_NAME_FFBM,
  SPM_GLOBAL_BLOCK_NAME_CANE,
  SPM_GLOBAL_BLOCK_NAME_LAST = SPM_GLOBAL_BLOCK_NAME_CANE,
};

enum SpmSeBlockId {
  SPM_SE_BLOCK_NAME_FIRST = 0,
  SPM_SE_BLOCK_NAME_CB = SPM_SE_BLOCK_NAME_FIRST,
  SPM_SE_BLOCK_NAME_DB,
  SPM_SE_BLOCK_NAME_PA,
  SPM_SE_BLOCK_NAME_SX,
  SPM_SE_BLOCK_NAME_SC,
  SPM_SE_BLOCK_NAME_TA,
  SPM_SE_BLOCK_NAME_TD,
  SPM_SE_BLOCK_NAME_TCP,
  SPM_SE_BLOCK_NAME_SPI,
  SPM_SE_BLOCK_NAME_SQG,
  SPM_SE_BLOCK_NAME_GL1A,
  SPM_SE_BLOCK_NAME_RMI,
  SPM_SE_BLOCK_NAME_GL1C,
  SPM_SE_BLOCK_NAME_GL1CG,
  SPM_SE_BLOCK_NAME_CBR,
  SPM_SE_BLOCK_NAME_DBR,
  SPM_SE_BLOCK_NAME_GL1H,
  SPM_SE_BLOCK_NAME_SQC,
  SPM_SE_BLOCK_NAME_PC,
  SPM_SE_BLOCK_NAME_EA,
  SPM_SE_BLOCK_NAME_GE,
  SPM_SE_BLOCK_NAME_GL2A,
  SPM_SE_BLOCK_NAME_GL2C,
  SPM_SE_BLOCK_NAME_WGS,
  SPM_SE_BLOCK_NAME_GL1XA,
  SPM_SE_BLOCK_NAME_GL1XC,
  SPM_SE_BLOCK_NAME_UTCL1,
  SPM_SE_BLOCK_NAME_LAST = SPM_SE_BLOCK_NAME_UTCL1,
};

// ip_block : athub_4_1_0
// ip_block : gc_12_0_0
// ip_block : sdma_7_0_0

// Number of block instances
// Reference: global_features.h (from gfxip header file package)
static const uint32_t ChaCounterBlockNumInstances      = 1;
static const uint32_t ChcCounterBlockNumInstances      = 4;
static const uint32_t CpcCounterBlockNumInstances      = 1;
static const uint32_t CpfCounterBlockNumInstances      = 1;
static const uint32_t CpgCounterBlockNumInstances      = 1;
static const uint32_t GcmcVmL2CounterBlockNumInstances = 1;
static const uint32_t GcrCounterBlockNumInstances      = 1;
static const uint32_t Gcutcl2CounterBlockNumInstances  = 1;
static const uint32_t Gcvml2CounterBlockNumInstances   = 1;
static const uint32_t GcCaneCounterBlockNumInstances   = 1;
static const uint32_t GcEaCpwdCounterBlockNumInstances = 18;
static const uint32_t GcEaSeCounterBlockNumInstances   = 24;
static const uint32_t Gl1aCounterBlockNumInstances     = 1;
static const uint32_t Gl1cCounterBlockNumInstances     = 4;
static const uint32_t Gl2aCounterBlockNumInstances     = 4;
static const uint32_t Gl2cCounterBlockNumInstances     = 24;
static const uint32_t GrbmCounterBlockNumInstances     = 1;
static const uint32_t GrbmhCounterBlockNumInstances    = 1;
static const uint32_t VcaCounterBlockNumInstances      = 2;
static const uint32_t VcdCounterBlockNumInstances      = 2;
static const uint32_t VmwCounterBlockNumInstances      = 1;
static const uint32_t VtfCounterBlockNumInstances      = 2;
static const uint32_t VtsCounterBlockNumInstances      = 2;
static const uint32_t RlcCounterBlockNumInstances      = 1;
static const uint32_t RpbCounterBlockNumInstances      = 1;
static const uint32_t SdmaCounterBlockNumInstances     = 2;
static const uint32_t SpiCounterBlockNumInstances      = 1;
static const uint32_t SqcCounterBlockNumInstances      = 1;
static const uint32_t SqgCounterBlockNumInstances      = 1;
static const uint32_t TaCounterBlockNumInstances       = 2;
static const uint32_t TcpCounterBlockNumInstances      = 2;
static const uint32_t TdCounterBlockNumInstances       = 2;
static const uint32_t Utcl1CounterBlockNumInstances    = 2;
static const uint32_t SpCounterBlockNumInstances       = 1;

// Number of block counter registers - Auto-generated from chip_offset_byte.h, edit with extra caution
// Reference: chip_offset_byte.h (from gfxip header file package)
static const uint32_t ChaCounterBlockNumCounters      = 4;
static const uint32_t ChcCounterBlockNumCounters      = 4;
static const uint32_t CpcCounterBlockNumCounters      = 2;
static const uint32_t CpfCounterBlockNumCounters      = 2;
static const uint32_t CpgCounterBlockNumCounters      = 2;
static const uint32_t GcmcVmL2CounterBlockNumCounters = 8;
static const uint32_t GcrCounterBlockNumCounters      = 2;
static const uint32_t Gcutcl2CounterBlockNumCounters  = 4;
static const uint32_t Gcvml2CounterBlockNumCounters   = 2;
static const uint32_t GcEaCpwdCounterBlockNumCounters = 2;
static const uint32_t GcEaSeCounterBlockNumCounters   = 1;
static const uint32_t GcCaneCounterBlockNumCounters   = 1;
static const uint32_t Gl1aCounterBlockNumCounters     = 4;
static const uint32_t Gl1cCounterBlockNumCounters     = 4;
static const uint32_t Gl2aCounterBlockNumCounters     = 4;
static const uint32_t Gl2cCounterBlockNumCounters     = 4;
static const uint32_t GrbmCounterBlockNumCounters     = 2;
static const uint32_t GrbmhCounterBlockNumCounters    = 2;
static const uint32_t VcaCounterBlockNumCounters      = 2;
static const uint32_t VcdCounterBlockNumCounters      = 2;
static const uint32_t VmwCounterBlockNumCounters      = 8;
static const uint32_t VtfCounterBlockNumCounters      = 2;
static const uint32_t VtsCounterBlockNumCounters      = 2;
static const uint32_t RlcCounterBlockNumCounters      = 2;
static const uint32_t RpbCounterBlockNumCounters      = 4;
static const uint32_t SdmaCounterBlockNumCounters     = 2;
static const uint32_t SpiCounterBlockNumCounters      = 6;
static const uint32_t SqcCounterBlockNumCounters      = 8;
static const uint32_t SqgCounterBlockNumCounters      = 8;
static const uint32_t TaCounterBlockNumCounters       = 2;
static const uint32_t TcpCounterBlockNumCounters      = 4;
static const uint32_t TdCounterBlockNumCounters       = 2;
static const uint32_t Utcl1CounterBlockNumCounters    = 4;
static const uint32_t SpCounterBlockNumCounters       = 4;

// Block counters max event value - Auto-generated from chip_enum.h, edit with extra caution
// Reference: chip_enum.h (from gfxip header file package)
static const uint32_t ChaCounterBlockMaxEvent      = 25;
static const uint32_t ChcCounterBlockMaxEvent      = 94;
static const uint32_t CpcCounterBlockMaxEvent      = 55;
static const uint32_t CpfCounterBlockMaxEvent      = 4;
static const uint32_t CpgCounterBlockMaxEvent      = 30;
static const uint32_t GcmcVmL2CounterBlockMaxEvent = 90;
static const uint32_t GcrCounterBlockMaxEvent      = 151;
static const uint32_t Gcutcl2CounterBlockMaxEvent  = 36;
static const uint32_t Gcvml2CounterBlockMaxEvent   = 90;
static const uint32_t GcEaCpwdCounterBlockMaxEvent = 32;
static const uint32_t GcEaSeCounterBlockMaxEvent   = 37;
static const uint32_t GcCaneCounterBlockMaxEvent   = 35;
static const uint32_t Gl1aCounterBlockMaxEvent     = 21;
static const uint32_t Gl1cCounterBlockMaxEvent     = 121;
static const uint32_t Gl2aCounterBlockMaxEvent     = 114;
static const uint32_t Gl2cCounterBlockMaxEvent     = 249;
static const uint32_t GrbmCounterBlockMaxEvent     = 51;
static const uint32_t GrbmhCounterBlockMaxEvent    = 25;
static const uint32_t VcaCounterBlockMaxEvent      = 69;
static const uint32_t VcdCounterBlockMaxEvent      = 34;
static const uint32_t VmwCounterBlockMaxEvent      = 390;
static const uint32_t VtfCounterBlockMaxEvent      = 69;
static const uint32_t VtsCounterBlockMaxEvent      = 224;
static const uint32_t RlcCounterBlockMaxEvent      = 6;
static const uint32_t RpbCounterBlockMaxEvent      = 29;
static const uint32_t SdmaCounterBlockMaxEvent     = 125;
static const uint32_t SpiCounterBlockMaxEvent      = 318;
static const uint32_t SqcCounterBlockMaxEvent      = 1023;
static const uint32_t SqgCounterBlockMaxEvent      = 45;
static const uint32_t TaCounterBlockMaxEvent       = 254;
static const uint32_t TcpCounterBlockMaxEvent      = 99;
static const uint32_t TdCounterBlockMaxEvent       = 271;
static const uint32_t Utcl1CounterBlockMaxEvent    = 71;
static const uint32_t SpCounterBlockMaxEvent       = 1023;

static const uint32_t SdmaCounterBlockMaxInstances = 8;
static const uint32_t UmcCounterBlockMaxInstances = 32;

}  // namespace gfx13
}  // namespace gfxip

#endif  //  _GFX13_BLOCKINFO_H_
