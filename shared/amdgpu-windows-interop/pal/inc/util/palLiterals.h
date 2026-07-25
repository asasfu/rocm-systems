/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */

#pragma once

#include "palUtil.h"

namespace Util
{
/// byte-unit for a thousand bytes
constexpr uint64 OneKibibyte = 1024;

/// byte-unit for a million bytes
constexpr uint64 OneMebibyte = 1024 * OneKibibyte;

/// byte-unit for a billion bytes
constexpr uint64 OneGibibyte = 1024 * OneMebibyte;

/// byte-unit for a trillion bytes
constexpr uint64 OneTebibyte = 1024 * OneGibibyte;

namespace Literals
{
/// Literal operator for kibibytes.
/// Ex: 5.5_KiB -> 5632 bytes
///
/// @param n floating point representing how many kibibytes
/// @returns n converted to bytes
consteval uint64 operator ""_KiB(long double n)
{
    return uint64(n * OneKibibyte);
}

/// Literal operator for kibibytes.
/// Ex: 5_KiB -> 5120 bytes
///
/// @param n integer representing how many kibibytes
/// @returns n converted to bytes
consteval uint64 operator ""_KiB(unsigned long long n)
{
    return uint64(n * OneKibibyte);
}

/// Literal operator for mebibytes.
/// Ex: 5.5_MiB -> 5767168 bytes
///
/// @param n floating point representing how many mebibytes
/// @returns n converted to bytes
consteval uint64 operator ""_MiB(long double n)
{
    return uint64(n * OneMebibyte);
}

/// Literal operator for mebibytes.
/// Ex: 5_MiB -> 5242880 bytes
///
/// @param n integer representing how many mebibytes
/// @returns n converted to bytes
consteval uint64 operator ""_MiB(unsigned long long n)
{
    return uint64(n * OneMebibyte);
}

/// Literal operator for gibibytes.
/// Ex: 5.5_GiB -> 5905580032 bytes
///
/// @param n floating point representing how many gibibytes
/// @returns n converted to bytes
consteval uint64 operator ""_GiB(long double n)
{
    return uint64(n * OneGibibyte);
}

/// Literal operator for gibibytes.
/// Ex: 5_GiB -> 5368709120 bytes
///
/// @param n integer representing how many gibibytes
/// @returns n converted to bytes
consteval uint64 operator ""_GiB(unsigned long long n)
{
    return uint64(n * OneGibibyte);
}

/// Literal operator for tebibytes.
/// Ex: 5.5_TiB -> 6047313952768 bytes
///
/// @param n floating point representing how many tebibytes
/// @returns n converted to bytes
consteval uint64 operator ""_TiB(long double n)
{
    return uint64(n * OneTebibyte);
}

/// Literal operator for tebibytes.
/// Ex: 5_TiB -> 5497558138880 bytes
///
/// @param n integer representing how many tebibytes
/// @returns n converted to bytes
consteval uint64 operator ""_TiB(unsigned long long n)
{
    return uint64(n * OneTebibyte);
}
} // Literals
} // Util
