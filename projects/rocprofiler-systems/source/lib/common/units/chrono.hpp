// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <ratio>
#include <string_view>

namespace rocprofsys::inline common::units
{

namespace detail
{

inline constexpr std::intmax_t SECONDS_PER_MINUTE = 60;
inline constexpr std::intmax_t SECONDS_PER_HOUR   = 3600;

}  // namespace detail

/**
 * Printable suffix for a std::chrono::duration @p Period (e.g. "ms" for
 * std::milli).
 *
 * Left undefined for unknown periods so that printing an unregistered unit is a
 * compile error; specialise it to add a new unit's suffix.
 */
template <typename Period>
struct duration_suffix;

template <>
struct duration_suffix<std::nano>
{
    static constexpr std::string_view VALUE = "ns";
};
template <>
struct duration_suffix<std::micro>
{
    static constexpr std::string_view VALUE = "us";
};
template <>
struct duration_suffix<std::milli>
{
    static constexpr std::string_view VALUE = "ms";
};
template <>
struct duration_suffix<std::ratio<1>>
{
    static constexpr std::string_view VALUE = "s";
};
template <>
struct duration_suffix<std::ratio<detail::SECONDS_PER_MINUTE>>
{
    static constexpr std::string_view VALUE = "min";
};
template <>
struct duration_suffix<std::ratio<detail::SECONDS_PER_HOUR>>
{
    static constexpr std::string_view VALUE = "h";
};

}  // namespace rocprofsys::inline common::units
