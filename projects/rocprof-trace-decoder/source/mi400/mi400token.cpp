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

#include <sys/stat.h>
#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include "mi400parser.h"
#include "mi400token.h"
#include "trace_parser.hpp"

typedef mi400::Token Token;

namespace mi400
{

TokenLookupTable::TokenLookupTable()
{
    AddEncoding({
        RdnaType::INST, {0, 1, 0, 0}
    });
    AddEncoding({
        RdnaType::VALU_INST, {1, 1, 0}
    });
    AddEncoding({
        RdnaType::NOP, {0, 0, 0, 0}
    });
    AddEncoding({
        RdnaType::IMM_ONE, {1, 0, 1, 1}
    });
    AddEncoding({
        RdnaType::WAVE_END, {1, 0, 0, 0, 0, 0, 1}
    });
    AddEncoding({
        RdnaType::LDS_CONFIG, {0, 1, 1, 0, 0, 1, 0, 0}
    });
    AddEncoding({
        RdnaType::MISC_GFX10, {1, 0, 0, 0, 1, 0, 1}
    });
    AddEncoding({
        RdnaType::TIME, {0, 1, 1, 1}
    });
    AddEncoding({
        RdnaType::MEDIUM_TIME, {0, 1, 1, 0, 0, 1, 0, 1}
    });

    time_bits[UNKNOWN] = {0, 0};
    time_bits[INST] = {4, 6};
    time_bits[VALU_INST] = {4, 8};
    time_bits[WAVE_READY] = {5, 8};
    time_bits[IMMEDIATE] = {5, 8};
    time_bits[IMM_ONE] = {7, 10};
    time_bits[NEW_PC_GFX12] = {8, 11};
    time_bits[EXEC_POPCOUNT1] = {7, 10};
    time_bits[EXEC_POPCOUNT3] = {6, 9};
    time_bits[WAVE_START] = {5, 7};
    time_bits[WAVE_START_EXT] = {5, 7};
    time_bits[WAVE_ALLOC] = {5, 8};
    time_bits[WAVE_END] = {7, 10};
    time_bits[SHADER_DATA] = {7, 10};
    time_bits[SHADER_DATA_SHORT] = {7, 10};
    time_bits[LDS_CONFIG] = {8, 10};
    time_bits[UTIL_COUNTER_GFX11] = {7, 9};
    time_bits[TIME] = {4, 8};
    time_bits[MISC_GFX10] = {7, 10};
    time_bits[EVENT] = {8, 11};
    time_bits[EVENT_SYNC] = {8, 11};
    time_bits[REG] = {4, 7};
    time_bits[REG_INIT] = {7, 10};
    time_bits[TIMESTAMP] = {12, 64};
    time_bits[HEADER] = {0, 0};
    time_bits[MEDIUM_TIME] = {8, 16};
    time_bits[NOP] = {0, 0};

    // Unused
    time_bits[NEW_PC_GFX10] = {0, 0};
    time_bits[UTIL_COUNTER_GFX10] = {0, 0};
    time_bits[RAYTRACE] = {0, 0};
    time_bits[REALTIME] = {0, 0};
}

int64_t TokenGenerator::getTime(RdnaType type, bool& PL, int64_t& rt)
{
    if (type == RdnaType::TIMESTAMP)
    {
        gfx12::timestamp_type stamp{.raw = current};
        PL |= bool(stamp.pl && !stamp.rt);
        if (stamp.rt == 0) return stamp.time + globaltime;

        if (stamp.pl == 0) rt = stamp.time;
        return globaltime;
    }
    else if (type == RdnaType::TIME) { globaltime += 1; }
    return lookupbits.getDelta(type, current) + globaltime;
};

std::array<int, 16> TM_DELTA_TABLE = {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 1, 2};

gfx10::Token TokenGenerator::next()
{
    while (byte_ptr + 9 < BUFFER_SIZE)
    {
        readOne_unsafe400();

        if (bIsExt && (current & 1)) // Handle wave_start_ext
        {
            bits_toread = 8; // LDS is 48 bits, WG is 40 bits
            continue;
        }

        RdnaType type = (RdnaType) lookupbits.lookup(current);
        if (type == RdnaType::NOP)
        {
            bits_toread = 8;
            continue;
        }

        bits_toread = TOKEN_LEN.at(type);
        bIsExt = type == WAVE_START_EXT;

        int64_t real = 0;
        if (type == VALU_INST)
            globaltime += TM_DELTA_TABLE[valu_inst_type{.raw = current}.wavetm];
        else if (type == TIME || type == TIMESTAMP || type == REALTIME)
            globaltime = getTime(type, packetlost, real);
        else
            globaltime += lookupbits.getDelta(type, current);

        if (type == RdnaType::TIMESTAMP || type == RdnaType::TIME)
        {
            if (real != 0) addRealtime(real);
            continue;
        }

        auto token = Token{globaltime, current, type};

        if (type == VALU_INST)
        {
            ::valu_inst_type valu12 = {};
            valu12.wid = get_valu_inst_mi400();
            token.contents = valu12.raw;
        }
        else if (type == INST)
        {
            auto inst = gfx12::inst_type{.raw = current};
            if (inst.inst >= 10 && inst.inst <= 14) update_fifo(inst.wid);
        }

        // Read last 8 bits of INST_PC
        if (bits_toread > 64 && byte_ptr < BUFFER_SIZE)
        {
            advanceByte(getBuffer400());
            token.contents = current;
        }

        bit_ptr = byte_ptr * 8;
        return token;
    }

    // Duplicated for performance reasons. Avoiding duplication leads to worse performance.
    while (byte_ptr < BUFFER_SIZE || current)
    {
        readOne_safe400();

        if (bIsExt && (current & 1)) // Handle wave_start_ext
        {
            bits_toread = 8; // LDS is 48 bits, WG is 40 bits
            continue;
        }

        RdnaType type = (RdnaType) lookupbits.lookup(current);
        if (type == RdnaType::NOP)
        {
            bits_toread = 8;
            continue;
        }

        bits_toread = TOKEN_LEN.at(type);
        bIsExt = type == WAVE_START_EXT;

        int64_t real = 0;
        if (type == VALU_INST)
            globaltime += TM_DELTA_TABLE[valu_inst_type{.raw = current}.wavetm];
        else if (type == TIME || type == TIMESTAMP || type == REALTIME)
            globaltime = getTime(type, packetlost, real);
        else
            globaltime += lookupbits.getDelta(type, current);

        if (type == RdnaType::TIMESTAMP || type == RdnaType::TIME)
        {
            if (real != 0) addRealtime(real);
            continue;
        }

        auto token = Token{globaltime, current, type};

        if (type == VALU_INST)
        {
            ::valu_inst_type valu12 = {};
            valu12.wid = get_valu_inst_mi400();
            token.contents = valu12.raw;
        }
        else if (type == INST)
        {
            auto inst = gfx12::inst_type{.raw = current};
            if (inst.inst >= 10 && inst.inst <= 14) update_fifo(inst.wid);
        }

        // Read last 8 bits of INST_PC
        if (bits_toread > 64 && byte_ptr < BUFFER_SIZE)
        {
            advanceByte(getBuffer400());
            token.contents = current;
        }

        bit_ptr = byte_ptr * 8;
        return token;
    }

    bit_ptr = byte_ptr * 8;
    return Token{0, 0, RdnaType::TIMESTAMP};
}

TokenGenerator::TokenGenerator(const uint8_t* _buffer, size_t size, int64_t _globaltime, int64_t _base_time) :
NaviTokenGenerator(_buffer, size, _globaltime, _base_time)
{
    if (!_buffer || !size) throw std::exception();

    static_assert(NAVI_TYPE_LAST <= 64);
    for (auto& v : TOKEN_LEN) v = 8;

    TOKEN_LEN[UNKNOWN] = 8;
    TOKEN_LEN[INST] = 24;
    TOKEN_LEN[VALU_INST] = 8;
    TOKEN_LEN[IMM_ONE] = 16;
    TOKEN_LEN[IMMEDIATE] = 24;
    TOKEN_LEN[WAVE_READY] = 24;
    TOKEN_LEN[NEW_PC_GFX12] = 72;
    TOKEN_LEN[EXEC_POPCOUNT1] = 24;
    TOKEN_LEN[EXEC_POPCOUNT3] = 48;
    TOKEN_LEN[WAVE_START] = 32;
    TOKEN_LEN[WAVE_START_EXT] = 40;
    TOKEN_LEN[WAVE_ALLOC] = 24;
    TOKEN_LEN[WAVE_END] = 24;
    TOKEN_LEN[SHADER_DATA] = 56;
    TOKEN_LEN[SHADER_DATA_SHORT] = 32;
    TOKEN_LEN[UTIL_COUNTER_GFX11] = 48;
    TOKEN_LEN[LDS_CONFIG] = 24;
    TOKEN_LEN[MISC_GFX10] = 24;
    TOKEN_LEN[EVENT] = 24;
    TOKEN_LEN[EVENT_SYNC] = 32;
    TOKEN_LEN[REG] = 64;
    TOKEN_LEN[REG_INIT] = 64;
    TOKEN_LEN[TIME] = 8;
    TOKEN_LEN[MEDIUM_TIME] = 16;
    TOKEN_LEN[TIMESTAMP] = 64;
    TOKEN_LEN[HEADER] = 64;
    TOKEN_LEN[NOP] = 8;

    // Unused
    TOKEN_LEN[NEW_PC_GFX10] = 8;
    TOKEN_LEN[UTIL_COUNTER_GFX10] = 8;
    TOKEN_LEN[RAYTRACE] = 8;
    TOKEN_LEN[REALTIME] = 8;
}

void TokenGenerator::update_fifo(int wave)
{
    if (FIFO[0] == wave) return;

    int pos = 5;
    for (int i = 1; i < 5; i++)
        if (FIFO[i] == wave) pos = i;

    for (int j = pos; j >= 1; j--) FIFO[j] = FIFO[j - 1];

    FIFO[0] = wave;
}

std::array<int, 16> WAVE_ID_TM_TABLE = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4};

int TokenGenerator::get_valu_inst_mi400()
{
    int wave = FIFO.at(WAVE_ID_TM_TABLE.at(valu_inst_type{.raw = current}.wavetm));
    update_fifo(wave);
    return wave;
}

} // namespace mi400
