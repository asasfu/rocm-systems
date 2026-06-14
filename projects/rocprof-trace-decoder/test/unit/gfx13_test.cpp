// MIT License
//
// Copyright (c) 2024-2026 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include "gfx10/gfx10wave.h"
#include "gfx13/gfx13parser.h"
#include "gfx13/gfx13token.h"
#include "gfx13/gfx13wave.h"
#include "mi400/mi400token.h"
#include "stitch/stitch.hpp"

//=============================================================================
// GFX13 Token Type Tests
//=============================================================================

TEST(GFX13TokenTest, DefaultConstruction)
{
    gfx13::Token token{};
    EXPECT_EQ(token.time, 0);
    EXPECT_EQ(token.contents, 0);
}

TEST(GFX13TokenTest, ParameterizedConstruction)
{
    gfx13::Token token{1000, 0xABCD, RdnaType::INST};
    EXPECT_EQ(token.time, 1000);
    EXPECT_EQ(token.contents, 0xABCD);
    EXPECT_EQ(token.type, RdnaType::INST);
}

//=============================================================================
// GFX13 Token Lookup Table Tests
//=============================================================================

TEST(GFX13LookupTableTest, InheritsFromMI400)
{
    gfx13::TokenLookupTable lookup;

    // Test inherited encodings from MI400
    EXPECT_EQ(lookup.lookup(0b0010).type, RdnaType::INST);
    EXPECT_EQ(lookup.lookup(0b011).type, RdnaType::VALU_INST);
    EXPECT_EQ(lookup.lookup(0b1000001).type, RdnaType::WAVE_END);
}

TEST(GFX13LookupTableTest, GetTimeForTimestamp)
{
    gfx13::TokenLookupTable lookup;

    gfx13::longtime_type ts{};
    ts.pl = 0;
    ts.time = 1000;

    bool packetlost = false;
    int64_t realtime = 0;
    int64_t cur_time = 500;

    auto result = lookup.getTime(lookup.lookup(0b00000001), ts.raw, cur_time, packetlost, realtime);
    EXPECT_EQ(result, ts.time + cur_time);
}

//=============================================================================
// GFX13 Wave Instruction Mapping Tests
//=============================================================================

TEST(GFX13WaveTest, InstMapToGfx9UnknownReturnsNone)
{
    // Test unknown instruction numbers return NONE
    auto mapped = gfx13::map_to_common_type(999, 0, 0);
    EXPECT_EQ(mapped.category, WaveInstCategory::NONE);
    EXPECT_EQ(mapped.cycles, 0);
}

//=============================================================================
// GFX13 Token Generator Tests
//=============================================================================

TEST(GFX13TokenGeneratorTest, ConstructorThrowsOnNullBuffer)
{
    EXPECT_THROW(gfx13::TokenGenerator(nullptr, 10, 0, 0), std::exception);
}

TEST(GFX13TokenGeneratorTest, ConstructorThrowsOnZeroSize)
{
    uint8_t dummy[10] = {0};
    EXPECT_THROW(gfx13::TokenGenerator(dummy, 0, 0, 0), std::exception);
}

TEST(GFX13TokenGeneratorTest, EmptyBufferTerminates)
{
    uint8_t buffer[1] = {0};
    gfx13::TokenGenerator gen(buffer, 1, 0, 0);

    // Should terminate without infinite loop
    int count = 0;
    while (gen.nextValid() && count < 100)
    {
        gen.next();
        count++;
    }
    EXPECT_LT(count, 100);
}

// Mock ICodeServicer for testing
class MockCodeServicerGfx13 : public ICodeServicer
{
public:
    MOCK_METHOD(assemblyLine, GetInstruction, (pcinfo_t addr, int gfxip), (override));
};

// Test callback to capture output
struct GFX13TestCallbackData
{
    std::vector<rocprofiler_thread_trace_decoder_record_type_t> record_types;
    int wave_count = 0;
    uint64_t gfxip = 0;

    // Captured wave data
    std::vector<uint8_t> wave_cus;
    std::vector<uint8_t> wave_simds;
    std::vector<uint8_t> wave_ids;
    std::vector<uint64_t> instruction_counts;
};

rocprofiler_thread_trace_decoder_status_t gfx13_test_callback(
    rocprofiler_thread_trace_decoder_record_type_t type, void* data, uint64_t size, void* userdata
)
{
    auto* cbdata = static_cast<GFX13TestCallbackData*>(userdata);
    cbdata->record_types.push_back(type);

    if (type == ROCPROFILER_THREAD_TRACE_DECODER_RECORD_GFXIP)
        cbdata->gfxip = reinterpret_cast<uint64_t>(data);
    else if (type == ROCPROFILER_THREAD_TRACE_DECODER_RECORD_WAVE)
    {
        cbdata->wave_count++;
        auto* wave = static_cast<rocprofiler_thread_trace_decoder_wave_t*>(data);
        cbdata->wave_cus.push_back(wave->cu);
        cbdata->wave_simds.push_back(wave->simd);
        cbdata->wave_ids.push_back(wave->wave_id);
        cbdata->instruction_counts.push_back(wave->instructions_size);
    }
    return ROCPROFILER_THREAD_TRACE_DECODER_STATUS_SUCCESS;
}

TEST(GFX13TokenGeneratorTest, ParsesHeaderWaveStartWaveEnd)
{
    // Build a buffer with HEADER, WAVE_START, INST, and WAVE_END tokens
    std::vector<uint8_t> buffer(32, 0);

    // Token 1: HEADER (64 bits)
    mi400::header_type header{};
    header.header = 0b0010001; // HEADER encoding
    header.version = 6;        // tt_version for gfx13
    header.DPRate = 1;
    header.DWGP = 3;
    header.DSIMD = 2;
    header.DSA = 0;
    memcpy(&buffer[0], &header.raw, 8);

    // Token 2: WAVE_START (32 bits, padded to 64)
    gfx12::wstart_type ws{};
    ws.header = 0b01100; // WAVE_START encoding
    ws.tm = 1;
    ws.sa = 0;
    ws.simd = 2;
    ws.wgp = 3;
    ws.wid = 5;
    memcpy(&buffer[8], &ws.raw, 8);

    // Token 3: WAVE_END (24 bits, padded to 64)
    mi400::wend_type we{};
    we.header = 0b1000001; // WAVE_END encoding
    we.tm = 1;
    we.sa = 0;
    we.simd = 2;
    we.wgp = 3;
    we.wid = 5;
    memcpy(&buffer[16], &we.raw, 8);

    // Set up mock and stitcher
    auto mock_service = std::make_shared<MockCodeServicerGfx13>();
    GFX13TestCallbackData cbdata{};
    Stitcher stitcher(mock_service, gfx13_test_callback, &cbdata);
    stitcher.setgfxip(13);

    CppReturnInfo info{};
    gfx13::TokenGenerator gen(buffer.data(), buffer.size(), 0, 0);
    RDNASQTParser parser;

    parser.sqtt_simd_analysis(info, gen, stitcher);

    // Verify GFXIP callback was received
    EXPECT_EQ(cbdata.gfxip, 13);

    // Verify record types received (should include GFXIP and WAVE)
    EXPECT_FALSE(cbdata.record_types.empty());

    // Verify a wave was parsed
    EXPECT_EQ(cbdata.wave_count, 1);

    // Verify wave properties match the input tokens
    ASSERT_EQ(cbdata.wave_cus.size(), 1u);
    EXPECT_EQ(cbdata.wave_cus[0], 3);   // wgp from header DWGP
    EXPECT_EQ(cbdata.wave_simds[0], 2); // simd from header DSIMD
    EXPECT_EQ(cbdata.wave_ids[0], 5);   // wid from WAVE_START
}

TEST(GFX13TokenGeneratorTest, ParsesInstThenValuInst)
{
    // Build a buffer with HEADER, WAVE_START, INST, VALU_INST, WAVE_END
    std::vector<uint8_t> buffer(40, 0);

    // Token 1: HEADER (64 bits)
    mi400::header_type header{};
    header.header = 0b0010001;
    header.version = 6;
    header.DPRate = 1;
    header.DWGP = 3;
    header.DSIMD = 2;
    header.DSA = 0;
    memcpy(&buffer[0], &header.raw, 8);

    // Token 2: WAVE_START (32 bits, padded to 64)
    gfx12::wstart_type ws{};
    ws.header = 0b01100;
    ws.tm = 1;
    ws.sa = 0;
    ws.simd = 2;
    ws.wgp = 3;
    ws.wid = 7;
    memcpy(&buffer[8], &ws.raw, 8);

    // Token 3: INST (24 bits, padded to 64)
    mi400::inst_type inst{};
    inst.header = 0b010;
    inst.tm = 1;
    inst.tp = 0;
    inst.wid = 7;
    inst.inst = 10; // valu_1 instruction
    memcpy(&buffer[16], &inst.raw, 8);

    // Token 4: VALU_INST (8 bits, padded to 64)
    mi400::valu_inst_type valu{};
    valu.header = 0b011;
    valu.tp = 0;
    valu.wavetm = 0;
    memcpy(&buffer[24], &valu.raw, 8);

    // Token 5: WAVE_END (24 bits, padded to 64)
    mi400::wend_type we{};
    we.header = 0b1000001;
    we.tm = 1;
    we.sa = 0;
    we.simd = 2;
    we.wgp = 3;
    we.wid = 7;
    memcpy(&buffer[32], &we.raw, 8);

    // Set up mock and stitcher
    auto mock_service = std::make_shared<MockCodeServicerGfx13>();
    GFX13TestCallbackData cbdata{};
    Stitcher stitcher(mock_service, gfx13_test_callback, &cbdata);
    stitcher.setgfxip(13);

    CppReturnInfo info{};
    gfx13::TokenGenerator gen(buffer.data(), buffer.size(), 0, 0);
    RDNASQTParser parser;

    parser.sqtt_simd_analysis(info, gen, stitcher);

    // Verify GFXIP callback was received
    EXPECT_EQ(cbdata.gfxip, 13);

    // Verify record types received
    EXPECT_FALSE(cbdata.record_types.empty());

    // Verify a wave was parsed
    EXPECT_EQ(cbdata.wave_count, 1);

    // Verify wave properties match the input tokens
    ASSERT_EQ(cbdata.wave_cus.size(), 1u);
    EXPECT_EQ(cbdata.wave_cus[0], 3);   // wgp from header DWGP
    EXPECT_EQ(cbdata.wave_simds[0], 2); // simd from header DSIMD
    EXPECT_EQ(cbdata.wave_ids[0], 7);   // wid from WAVE_START

    // Verify wave has instructions (INST + VALU_INST = 2 instructions)
    EXPECT_GE(cbdata.instruction_counts[0], 1u);
}

//=============================================================================
// GFX13 Wave Lifecycle Tests
//=============================================================================

TEST(GFX13WaveTest, CompleteWaveLifecycle)
{
    // EINST values from gfx13wave.cpp enum
    constexpr int SALU = 0;
    constexpr int SMEM_RD = 1;
    constexpr int VALU_1 = 10;
    constexpr int BARRIER_WAIT = 19;

    // 1. Create wave (simulates HEADER + WAVE_START)
    gfx10::Token startToken{};
    startToken.time = 100;
    startToken.type = RdnaType::WAVE_START;
    pcinfo_t addr{1, 0x1000};
    gfx10::wave_t wave(0, 0, 0, addr, startToken, false);
    wave.trap_status = WaveTrapStatus::TRAP_RESTORED;

    EXPECT_EQ(wave.cur_state, WaveslotState::WS_IDLE);
    EXPECT_EQ(wave.begin_time, 100);

    // 2. Wave ready at time 110
    wave.apply_wave_rdy(110);
    EXPECT_EQ(wave.cur_state, WaveslotState::WS_STALL);

    // 3. A few regular instructions (SALU, SMEM)
    auto mapped_salu = gfx13::map_to_common_type(SALU, 1, 0);
    wave.apply_inst(120, SALU, mapped_salu, 6);

    auto mapped_smem = gfx13::map_to_common_type(SMEM_RD, 1, 0);
    wave.apply_inst(130, SMEM_RD, mapped_smem, 6);

    wave.apply_inst(140, SALU, mapped_salu, 6);

    EXPECT_EQ(wave.instructions.size(), 3u);
    EXPECT_EQ(wave.cur_state, WaveslotState::WS_EXEC);

    // 4. One VALU instruction at time 150
    auto mapped_valu = gfx13::map_to_common_type(VALU_1, 1, 0);
    wave.apply_inst(150, VALU_1, mapped_valu, 6);

    EXPECT_EQ(wave.instructions.size(), 4u);
    EXPECT_EQ(wave.instructions.back().category, (uint32_t) WaveInstCategory::VALU);

    // 5. One immediate (barrier_wait) at time 160
    auto mapped_barrier = gfx13::map_to_common_type(BARRIER_WAIT, 1, 0);
    wave.apply_inst(160, BARRIER_WAIT, mapped_barrier, 6);

    EXPECT_EQ(wave.instructions.size(), 5u);
    EXPECT_EQ(wave.instructions.back().category, (uint32_t) WaveInstCategory::IMMED);

    // 6. Wave end at time 170
    gfx10::Token endToken{};
    endToken.time = 170;
    endToken.type = RdnaType::WAVE_END;
    wave.complete_wave(endToken);

    EXPECT_TRUE(wave.bIsComplete);
    EXPECT_EQ(wave.end_time, 170);
    EXPECT_EQ(wave.cur_state, WaveslotState::WS_EMPTY);

    // Verify timeline has state transitions
    EXPECT_GE(wave.timeline.size(), 1u);
}
