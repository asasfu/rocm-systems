// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT

#pragma once

#include <compare>
#include <cstdint>
#include <ratio>
#include <string_view>
#include <type_traits>

namespace rocprofsys::inline common::units
{

/**
 * A frequency value, modelled on std::chrono::duration.
 *
 * Stores a count of @p Period hertz, where @p Period is a std::ratio relative
 * to one hertz. `frequency<double, std::kilo>{3}` therefore represents 3 kHz.
 *
 * @tparam Rep    underlying arithmetic representation (defaults to double so
 *                that scaling never silently truncates).
 * @tparam Period std::ratio giving this unit's size in hertz.
 */
template <typename Rep, typename Period = std::ratio<1>>
class frequency
{
public:
    using rep    = Rep;
    using period = Period;

    constexpr frequency() = default;

    constexpr explicit frequency(const Rep& value) noexcept
    : m_count{ value }
    {}

    /** @return the raw count in units of @p Period (3 for `3_khz`). */
    [[nodiscard]] constexpr Rep count() const noexcept { return m_count; }

private:
    Rep m_count{};
};

using hertz     = frequency<double, std::ratio<1>>;
using kilohertz = frequency<double, std::kilo>;
using megahertz = frequency<double, std::mega>;
using gigahertz = frequency<double, std::giga>;

namespace detail
{

template <typename T>
struct is_frequency : std::false_type
{};

template <typename Rep, typename Period>
struct is_frequency<frequency<Rep, Period>> : std::true_type
{};

}  // namespace detail

/** Satisfied by any instantiation of @ref frequency. */
template <typename T>
concept frequency_like = detail::is_frequency<std::remove_cvref_t<T>>::value;

/**
 * Printable suffix for a frequency @p Period (e.g. "MHz" for std::mega).
 *
 * Left undefined for unknown periods so that printing an unregistered unit is a
 * compile error; specialise it to add a new unit's suffix.
 */
template <typename Period>
struct frequency_suffix;

template <>
struct frequency_suffix<std::ratio<1>>
{
    static constexpr std::string_view VALUE = "Hz";
};
template <>
struct frequency_suffix<std::kilo>
{
    static constexpr std::string_view VALUE = "kHz";
};
template <>
struct frequency_suffix<std::mega>
{
    static constexpr std::string_view VALUE = "MHz";
};
template <>
struct frequency_suffix<std::giga>
{
    static constexpr std::string_view VALUE = "GHz";
};

/**
 * Convert a frequency to another frequency unit at compile time.
 *
 * Mirrors std::chrono::duration_cast: the factor is the exact rational
 * @c From::period / @c To::period, applied in @c To::rep.
 *
 * @tparam To   target frequency type.
 * @tparam From source frequency type (deduced).
 * @param  from value to convert.
 * @return @p from expressed in @p To's unit.
 */
template <frequency_like To, frequency_like From>
[[nodiscard]] constexpr To
frequency_cast(const From& from) noexcept
{
    using from_t     = std::remove_cvref_t<From>;
    using factor     = std::ratio_divide<typename from_t::period, typename To::period>;
    using rep        = typename To::rep;
    using calc       = std::common_type_t<rep, typename from_t::rep, std::intmax_t>;
    const auto count = static_cast<calc>(from.count()) * static_cast<calc>(factor::num) /
                       static_cast<calc>(factor::den);
    return To{ static_cast<rep>(count) };
}

/**
 * Equality across any two frequency units.
 *
 * Both operands are converted to hertz before comparing, so `1000_hz == 1_khz`.
 */
template <frequency_like LHS, frequency_like RHS>
[[nodiscard]] constexpr bool
operator==(const LHS& lhs, const RHS& rhs) noexcept
{
    using rep  = std::common_type_t<typename LHS::rep, typename RHS::rep>;
    using base = frequency<rep, std::ratio<1>>;
    return frequency_cast<base>(lhs).count() == frequency_cast<base>(rhs).count();
}

/**
 * Ordering across any two frequency units.
 *
 * Both operands are normalised to hertz before comparing, so `1_khz < 2_mhz`
 * is well-defined. Returns std::partial_ordering because the underlying rep is
 * floating-point.
 */
template <frequency_like LHS, frequency_like RHS>
[[nodiscard]] constexpr auto
operator<=>(const LHS& lhs, const RHS& rhs) noexcept
{
    using rep  = std::common_type_t<typename LHS::rep, typename RHS::rep>;
    using base = frequency<rep, std::ratio<1>>;
    return frequency_cast<base>(lhs).count() <=> frequency_cast<base>(rhs).count();
}

namespace literals
{

// clang-format off
constexpr hertz     operator""_hz (long double value)        noexcept { return hertz{static_cast<double>(value)}; }
constexpr hertz     operator""_hz (unsigned long long value) noexcept { return hertz{static_cast<double>(value)}; }
constexpr kilohertz operator""_khz(long double value)        noexcept { return kilohertz{static_cast<double>(value)}; }
constexpr kilohertz operator""_khz(unsigned long long value) noexcept { return kilohertz{static_cast<double>(value)}; }
constexpr megahertz operator""_mhz(long double value)        noexcept { return megahertz{static_cast<double>(value)}; }
constexpr megahertz operator""_mhz(unsigned long long value) noexcept { return megahertz{static_cast<double>(value)}; }
constexpr gigahertz operator""_ghz(long double value)        noexcept { return gigahertz{static_cast<double>(value)}; }
constexpr gigahertz operator""_ghz(unsigned long long value) noexcept { return gigahertz{static_cast<double>(value)}; }
// clang-format on

}  // namespace literals

}  // namespace rocprofsys::inline common::units
