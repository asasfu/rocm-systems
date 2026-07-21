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

#pragma once
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>
#include "mi400/mi400parser.h"
#include "mi400/mi400token.h"

namespace gfx13
{

union longtime_type
{
    struct
    {
        uint64_t header : 8;
        uint64_t pl     : 1;
        uint64_t tl     : 1;
        uint64_t unused : 2;
        uint64_t time   : 36;
    };
    uint64_t raw;
};

union realtime_type
{
    struct
    {
        uint64_t header : 8;
        uint64_t unused : 4;
        uint64_t time   : 52;
    };
    uint64_t raw;
};

class Token : public mi400::Token
{
public:
    Token() = default;
    Token(int64_t globaltime, uint64_t _contents, RdnaType _type) : mi400::Token(globaltime, _contents, _type) {}
};

} // namespace gfx13
