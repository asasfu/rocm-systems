// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT

#pragma once

// Opt-in header: include only in translation units that use fmt formatting.
// WARNING: do NOT also include <fmt/chrono.h> in the same TU — it defines its
// own formatter for std::chrono::duration and the two will collide.

#include <chrono>
#include <ratio>
#include <string_view>
#include <utility>

#include <spdlog/fmt/fmt.h>

#include "common/units/chrono.hpp"
#include "common/units/data_size.hpp"
#include "common/units/frequency.hpp"
#include "common/units/power.hpp"

// ---------------------------------------------------------------------------
// fmt::formatter<rocprofsys::common::units::frequency<Rep, Period>>
//
// Default:   fmt::format("{}", 2_mhz)         -> "2 MHz"
// Autoscale: fmt::format("{:~}", 50000000_hz) -> "50 MHz"
//            Composes with specs: "{:~.2f}"   -> "1.50 kHz"
// ---------------------------------------------------------------------------
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
            const auto [scaled, suffix] = autoscale_freq(freq_hz);
            auto out = m_count_formatter.format(static_cast<Rep>(scaled), ctx);
            return fmt::format_to(out, " {}", suffix);
        }
        using rocprofsys::common::units::frequency_suffix;
        auto out = m_count_formatter.format(value.count(), ctx);
        return fmt::format_to(out, " {}", frequency_suffix<Period>::value);
    }

private:
    static auto autoscale_freq(double freq_hz) -> std::pair<double, std::string_view>
    {
        using rocprofsys::common::units::frequency_suffix;
        constexpr double k_ghz =
            static_cast<double>(std::giga::num) / static_cast<double>(std::giga::den);
        constexpr double k_mhz =
            static_cast<double>(std::mega::num) / static_cast<double>(std::mega::den);
        constexpr double k_khz =
            static_cast<double>(std::kilo::num) / static_cast<double>(std::kilo::den);
        const double mag = freq_hz < 0.0 ? -freq_hz : freq_hz;
        if(mag >= k_ghz) return { freq_hz / k_ghz, frequency_suffix<std::giga>::value };
        if(mag >= k_mhz) return { freq_hz / k_mhz, frequency_suffix<std::mega>::value };
        if(mag >= k_khz) return { freq_hz / k_khz, frequency_suffix<std::kilo>::value };
        return { freq_hz, frequency_suffix<std::ratio<1>>::value };
    }

    fmt::formatter<Rep> m_count_formatter;
    bool                m_autoscale = false;
};

// ---------------------------------------------------------------------------
// fmt::formatter<rocprofsys::common::units::data_size<Rep, Scale>>
//
// Default:   fmt::format("{}", 2_mb)         -> "2 MB"
// Autoscale: fmt::format("{:~}", 5242880_b)  -> "5 MB"
// ---------------------------------------------------------------------------
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
            const auto [scaled, suffix] = autoscale_size(bytes_val);
            auto out = m_count_formatter.format(static_cast<Rep>(scaled), ctx);
            return fmt::format_to(out, " {}", suffix);
        }
        using rocprofsys::common::units::data_size_suffix;
        auto out = m_count_formatter.format(value.count(), ctx);
        return fmt::format_to(out, " {}", data_size_suffix<Scale>::value);
    }

private:
    static auto autoscale_size(double bytes_val) -> std::pair<double, std::string_view>
    {
        using rocprofsys::common::units::data_size_suffix;
        using rocprofsys::common::units::gigabytes;
        using rocprofsys::common::units::kilobytes;
        using rocprofsys::common::units::megabytes;
        using rocprofsys::common::units::terabytes;
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

// ---------------------------------------------------------------------------
// fmt::formatter<rocprofsys::common::units::power<Rep, Period>>
//
// Default:   fmt::format("{}", 250_mw)  -> "250 mW"
// Autoscale: fmt::format("{:~}", 0.5_w) -> "500 mW"
// ---------------------------------------------------------------------------
template <typename Rep, typename Period>
struct fmt::formatter<rocprofsys::common::units::power<Rep, Period>>
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
    auto format(const rocprofsys::common::units::power<Rep, Period>& value,
                FormatContext&                                       ctx) const
    {
        if(m_autoscale)
        {
            using rocprofsys::common::units::power_cast;
            using rocprofsys::common::units::watt;
            const auto watts_val = static_cast<double>(power_cast<watt>(value).count());
            const auto [scaled, suffix] = autoscale_power(watts_val);
            auto out = m_count_formatter.format(static_cast<Rep>(scaled), ctx);
            return fmt::format_to(out, " {}", suffix);
        }
        using rocprofsys::common::units::power_suffix;
        auto out = m_count_formatter.format(value.count(), ctx);
        return fmt::format_to(out, " {}", power_suffix<Period>::value);
    }

private:
    static auto autoscale_power(double watts_val) -> std::pair<double, std::string_view>
    {
        using rocprofsys::common::units::power_suffix;
        constexpr double k_kw =
            static_cast<double>(std::kilo::num) / static_cast<double>(std::kilo::den);
        constexpr double k_mw =
            static_cast<double>(std::milli::num) / static_cast<double>(std::milli::den);
        constexpr double k_uw =
            static_cast<double>(std::micro::num) / static_cast<double>(std::micro::den);
        constexpr double k_nw =
            static_cast<double>(std::nano::num) / static_cast<double>(std::nano::den);
        const double mag = watts_val < 0.0 ? -watts_val : watts_val;
        if(mag >= k_kw) return { watts_val / k_kw, power_suffix<std::kilo>::value };
        if(mag >= 1.0) return { watts_val, power_suffix<std::ratio<1>>::value };
        if(mag >= k_mw) return { watts_val / k_mw, power_suffix<std::milli>::value };
        if(mag >= k_uw) return { watts_val / k_uw, power_suffix<std::micro>::value };
        return { watts_val / k_nw, power_suffix<std::nano>::value };
    }

    fmt::formatter<Rep> m_count_formatter;
    bool                m_autoscale = false;
};

// ---------------------------------------------------------------------------
// fmt::formatter<std::chrono::duration<Rep, Period>>
//
// Default:   fmt::format("{}", 1500ms)   -> "1500 ms"
// Autoscale: fmt::format("{:~}", 1500ms) -> "1.5 s"
// ---------------------------------------------------------------------------
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
            const auto [scaled, suffix] = autoscale_duration(value);
            auto out                    = m_count_formatter.format(scaled, ctx);
            return fmt::format_to(out, " {}", suffix);
        }
        auto out = m_count_formatter.format(static_cast<double>(value.count()), ctx);
        using rocprofsys::common::units::duration_suffix;
        return fmt::format_to(out, " {}", duration_suffix<Period>::value);
    }

private:
    /// Uses duration<double, TargetPeriod> cast rather than dividing by a tiny
    /// constant to avoid FP rounding errors (e.g. 250000ns -> "250 us" not "250.000...03
    /// us").
    static auto autoscale_duration(const std::chrono::duration<Rep, Period>& dur)
        -> std::pair<double, std::string_view>
    {
        using rocprofsys::common::units::duration_suffix;
        const double secs = std::chrono::duration<double>(dur).count();
        const double mag  = secs < 0.0 ? -secs : secs;
        if(mag >= 1.0)
        {
            return { secs, duration_suffix<std::ratio<1>>::value };
        }
        if(mag >= 1e-3)
        {
            return { std::chrono::duration<double, std::milli>(dur).count(),
                     duration_suffix<std::milli>::value };
        }
        if(mag >= 1e-6)
        {
            return { std::chrono::duration<double, std::micro>(dur).count(),
                     duration_suffix<std::micro>::value };
        }
        return { std::chrono::duration<double, std::nano>(dur).count(),
                 duration_suffix<std::nano>::value };
    }

    fmt::formatter<double> m_count_formatter;
    bool                   m_autoscale = false;
};
