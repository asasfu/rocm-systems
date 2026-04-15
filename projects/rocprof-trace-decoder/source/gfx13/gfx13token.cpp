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

#include "gfx13parser.h"
#include "gfx13token.h"
#include "trace_parser.hpp"

typedef gfx13::Token Token;

namespace gfx13
{

TokenLookupTable::TokenLookupTable() : mi400::TokenLookupTable()
{
    AddEncoding({
        RdnaType::RAYTRACE, {1, 0, 1, 1, 0, 0, 1}
    });
    AddEncoding({
        RdnaType::TIMESTAMP, {1, 0, 0, 0, 0, 0, 0, 0}
    });
    AddEncoding({
        RdnaType::REALTIME, {1, 0, 0, 0, 0, 0, 0, 1}
    });
    time_bits[RAYTRACE] = {7, 10};
    time_bits[REALTIME] = {1, 1};
    time_bits[TIMESTAMP] = {12, 36};
}

int64_t TokenGenerator::getTime(RdnaType type, bool& PL, int64_t& rt)
{
    if (type == RdnaType::TIMESTAMP)
    {
        longtime_type stamp{.raw = current};
        PL |= bool(stamp.pl);
        return stamp.time + globaltime;
    }
    else if (type == RdnaType::REALTIME)
    {
        rt = realtime_type{.raw = current}.time;
        return globaltime;
    }
    else if (type == RdnaType::TIME) { globaltime += 1; }
    return lookupbits.getDelta(type, current) + globaltime;
};

TokenGenerator::TokenGenerator(const uint8_t* _buffer, size_t size, int64_t _globaltime, int64_t _base_time) :
mi400::TokenGenerator(_buffer, size, _globaltime, _base_time)
{
    lookupbits = gfx13::TokenLookupTable{};

    TOKEN_LEN[RAYTRACE] = 32;
    TOKEN_LEN[TIMESTAMP] = 48;
    TOKEN_LEN[REALTIME] = 64;
}

} // namespace gfx13
