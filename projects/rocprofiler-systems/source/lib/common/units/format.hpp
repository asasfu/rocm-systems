// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT

#pragma once

// Opt-in header: include only in translation units that use fmt formatting.
// Do NOT include <fmt/chrono.h> in the same TU as common/units/chrono.hpp.

#include <ratio>
#include <string_view>

#include <spdlog/fmt/fmt.h>

#include "common/units/data_size.hpp"
#include "common/units/frequency.hpp"

/**
 * fmt formatter for @ref rocprofsys::common::units::frequency.
 *
 * Default: prints the count in the value's own unit, e.g.
 * `fmt::format("{}", 2_mhz)` -> "2 MHz".
 *
 * Autoscale: prefix the spec with `~` to pick the unit that keeps the magnitude
 * in [1, 1000), e.g. `fmt::format("{:~}", 50000000_hz)` -> "50 MHz". The flag
 * composes with numeric specs, so `fmt::format("{:~.2f}", 1500_hz)` -> "1.50 kHz".
 */
template <typename Rep, typename Period>
struct fmt::formatter<rocprofsys::common::units::frequency<Rep, Period>>
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
    auto format(const rocprofsys::common::units::frequency<Rep, Period>& value,
                FormatContext&                                           ctx) const
    {
        if(m_autoscale)
        {
            using rocprofsys::common::units::frequency_cast;
            using rocprofsys::common::units::hertz;
            const auto freq_hz =
                static_cast<double>(frequency_cast<hertz>(value).count());
            const auto [scaled, suffix] = autoscale(freq_hz);
            auto out = m_count_formatter.format(static_cast<Rep>(scaled), ctx);
            return fmt::format_to(out, " {}", suffix);
        }
        using rocprofsys::common::units::frequency_suffix;
        auto out = m_count_formatter.format(value.count(), ctx);
        return fmt::format_to(out, " {}", frequency_suffix<Period>::value);
    }

private:
    /// Choose the unit keeping @p freq_hz in [1, 1000); returns the scaled value and
    /// suffix.
    static auto autoscale(double freq_hz) -> std::pair<double, std::string_view>
    {
        using rocprofsys::common::units::frequency_suffix;
        // Derive thresholds from std::ratio to avoid magic number literals.
        constexpr double k_mhz =
            static_cast<double>(std::mega::num) / static_cast<double>(std::mega::den);
        constexpr double k_khz =
            static_cast<double>(std::kilo::num) / static_cast<double>(std::kilo::den);
        const double mag = freq_hz < 0.0 ? -freq_hz : freq_hz;
        if(mag >= k_mhz) return { freq_hz / k_mhz, frequency_suffix<std::mega>::value };
        if(mag >= k_khz) return { freq_hz / k_khz, frequency_suffix<std::kilo>::value };
        return { freq_hz, frequency_suffix<std::ratio<1>>::value };
    }

    fmt::formatter<Rep> m_count_formatter;
    bool                m_autoscale = false;
};

/**
 * fmt formatter for @ref rocprofsys::common::units::data_size.
 *
 * Default: `fmt::format("{}", 2_mb)` -> "2 MB".
 * Autoscale with `~`: `fmt::format("{:~}", 5242880_b)` -> "5 MB".
 */
template <typename Rep, typename Scale>
struct fmt::formatter<rocprofsys::common::units::data_size<Rep, Scale>>
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
    auto format(const rocprofsys::common::units::data_size<Rep, Scale>& value,
                FormatContext&                                          ctx) const
    {
        if(m_autoscale)
        {
            using rocprofsys::common::units::bytes;
            using rocprofsys::common::units::data_size_cast;
            const auto bytes_val =
                static_cast<double>(data_size_cast<bytes>(value).count());
            const auto [scaled, suffix] = autoscale(bytes_val);
            auto out = m_count_formatter.format(static_cast<Rep>(scaled), ctx);
            return fmt::format_to(out, " {}", suffix);
        }
        using rocprofsys::common::units::data_size_suffix;
        auto out = m_count_formatter.format(value.count(), ctx);
        return fmt::format_to(out, " {}", data_size_suffix<Scale>::value);
    }

private:
    /// Choose the binary unit keeping @p bytes_val in [1, 1024); returns the scaled value
    /// and suffix.
    static auto autoscale(double bytes_val) -> std::pair<double, std::string_view>
    {
        using rocprofsys::common::units::data_size_suffix;
        using rocprofsys::common::units::gigabytes;
        using rocprofsys::common::units::kilobytes;
        using rocprofsys::common::units::megabytes;
        using rocprofsys::common::units::terabytes;
        // Derive thresholds from each type's own scale ratio to avoid magic literals.
        constexpr double k_kb = static_cast<double>(kilobytes::scale::num) /
                                static_cast<double>(kilobytes::scale::den);
        constexpr double k_mb = static_cast<double>(megabytes::scale::num) /
                                static_cast<double>(megabytes::scale::den);
        constexpr double k_gb = static_cast<double>(gigabytes::scale::num) /
                                static_cast<double>(gigabytes::scale::den);
        constexpr double k_tb = static_cast<double>(terabytes::scale::num) /
                                static_cast<double>(terabytes::scale::den);
        const double mag = bytes_val < 0.0 ? -bytes_val : bytes_val;
        if(mag >= k_tb)
            return { bytes_val / k_tb,
                     data_size_suffix<typename terabytes::scale>::value };
        if(mag >= k_gb)
            return { bytes_val / k_gb,
                     data_size_suffix<typename gigabytes::scale>::value };
        if(mag >= k_mb)
            return { bytes_val / k_mb,
                     data_size_suffix<typename megabytes::scale>::value };
        if(mag >= k_kb)
            return { bytes_val / k_kb,
                     data_size_suffix<typename kilobytes::scale>::value };
        return { bytes_val, data_size_suffix<std::ratio<1>>::value };
    }

    fmt::formatter<Rep> m_count_formatter;
    bool                m_autoscale = false;
};
