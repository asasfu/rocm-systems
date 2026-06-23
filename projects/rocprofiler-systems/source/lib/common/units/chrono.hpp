// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT

#pragma once

// Opt-in header: include only in translation units that use fmt formatting.
// WARNING: do NOT include <fmt/chrono.h> in the same TU - the two formatters
// for std::chrono::duration will collide.

#include <chrono>
#include <ratio>
#include <string_view>
#include <utility>

#include <spdlog/fmt/fmt.h>

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
    static constexpr std::string_view value = "ns";
};
template <>
struct duration_suffix<std::micro>
{
    static constexpr std::string_view value = "us";
};
template <>
struct duration_suffix<std::milli>
{
    static constexpr std::string_view value = "ms";
};
template <>
struct duration_suffix<std::ratio<1>>
{
    static constexpr std::string_view value = "s";
};
template <>
struct duration_suffix<std::ratio<detail::SECONDS_PER_MINUTE>>
{
    static constexpr std::string_view value = "min";
};
template <>
struct duration_suffix<std::ratio<detail::SECONDS_PER_HOUR>>
{
    static constexpr std::string_view value = "h";
};

}  // namespace rocprofsys::inline common::units

/**
 * fmt formatter for std::chrono::duration, matching the units-library style:
 * "<count> <suffix>" with a space, and a `~` autoscale flag that picks among
 * ns/us/ms/s to keep the magnitude readable.
 *
 * `fmt::format("{:~}", std::chrono::milliseconds{1500})` -> "1.5 s".
 */
template <typename Rep, typename Period>
struct fmt::formatter<std::chrono::duration<Rep, Period>>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        const auto* pos = ctx.begin();
        if(pos != ctx.end() && *pos == '~')
        {
            m_autoscale = true;
            ++pos;
            ctx.advance_to(pos);
        }
        return m_count_formatter.parse(ctx);
    }

    template <typename FormatContext>
    auto format(const std::chrono::duration<Rep, Period>& value, FormatContext& ctx) const
    {
        if(m_autoscale)
        {
            const auto [scaled, suffix] = autoscale(value);
            auto out                    = m_count_formatter.format(scaled, ctx);
            return fmt::format_to(out, " {}", suffix);
        }
        auto out = m_count_formatter.format(static_cast<double>(value.count()), ctx);
        using rocprofsys::common::units::duration_suffix;
        return fmt::format_to(out, " {}", duration_suffix<Period>::value);
    }

private:
    /// Choose the unit keeping @p d in a readable scale.
    ///
    /// Converts to the target unit via duration<double, TargetPeriod> rather than
    /// dividing the seconds double by a tiny constant (e.g. 1e-6), which avoids the
    /// FP rounding error that turns "250 us" into "250.00000000000003 us".
    static auto autoscale(const std::chrono::duration<Rep, Period>& d)
        -> std::pair<double, std::string_view>
    {
        using rocprofsys::common::units::duration_suffix;
        const double secs = std::chrono::duration<double>(d).count();
        const double mag  = secs < 0.0 ? -secs : secs;
        if(mag >= 1.0) return { secs, duration_suffix<std::ratio<1>>::value };
        if(mag >= 1e-3)
            return { std::chrono::duration<double, std::milli>(d).count(),
                     duration_suffix<std::milli>::value };
        if(mag >= 1e-6)
            return { std::chrono::duration<double, std::micro>(d).count(),
                     duration_suffix<std::micro>::value };
        return { std::chrono::duration<double, std::nano>(d).count(),
                 duration_suffix<std::nano>::value };
    }

    fmt::formatter<double> m_count_formatter;
    bool                   m_autoscale = false;
};
