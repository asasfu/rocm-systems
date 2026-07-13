// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT

#pragma once

#include <compare>
#include <concepts>
#include <cstdint>
#include <ratio>
#include <string_view>
#include <type_traits>
#include <utility>

namespace rocprofsys::inline common::units
{

/**
 * A data-size value, modelled on std::chrono::duration.
 *
 * Stores a count of @p Scale bytes, where @p Scale is a std::ratio relative to
 * one byte. Units are binary (1 KB = 1024 B), so
 * `data_size<double, std::ratio<1024>>{1}` represents 1 KB = 1024 bytes.
 *
 * @tparam Rep   underlying arithmetic representation (defaults to double so that
 *               scaling never silently truncates).
 * @tparam Scale std::ratio giving this unit's size in bytes.
 */
template <typename Rep, typename Scale = std::ratio<1>>
class data_size
{
public:
    using rep   = Rep;
    using scale = Scale;

    constexpr data_size() = default;

    /**
     * Converting from `long long`/`unsigned long long` must be explicit at
     * the call site: those types can hold values a @p Rep like `double`
     * can't represent exactly, so silently narrowing them here would lose
     * precision. Other implicit conversions (e.g. `int`) are still allowed.
     */
    template <typename U>
        requires std::convertible_to<const U&, Rep> &&
                 (!std::same_as<std::remove_cvref_t<U>, long long>) &&
                 (!std::same_as<std::remove_cvref_t<U>, unsigned long long>)
    constexpr explicit data_size(U&& value) noexcept
    : m_count{ static_cast<Rep>(std::forward<U>(value)) }
    {}

    /** @return the raw count in units of @p Scale (3 for `3_kb`). */
    [[nodiscard]] constexpr Rep count() const noexcept { return m_count; }

    /**
     * This size as a raw count of bytes (35840 for `35_kb`).
     *
     * Explicit on purpose: a typed size never decays into a number implicitly
     * so the unit cannot be dropped by accident.
     *
     * @warning The result has type @p Rep (double for the built-in aliases).
     *          Storing it in a narrower integer truncates, and a value outside
     *          that integer's range is undefined behaviour - prefer a 64-bit or
     *          floating-point target for large sizes. Because the conversion is
     *          explicit, that narrowing happens at the call site where
     *          `-Wconversion` can see it.
     * @return the size in bytes.
     */
    [[nodiscard]] constexpr Rep to_bytes() const noexcept
    {
        using calc = std::common_type_t<Rep, std::intmax_t>;
        return static_cast<Rep>(static_cast<calc>(m_count) *
                                static_cast<calc>(Scale::num) /
                                static_cast<calc>(Scale::den));
    }

private:
    Rep m_count{};
};

namespace detail
{
inline constexpr std::intmax_t BYTES_PER_KIB = 1024LL;
}  // namespace detail

using bytes     = data_size<double, std::ratio<1>>;
using kilobytes = data_size<double, std::ratio<detail::BYTES_PER_KIB>>;
using megabytes =
    data_size<double, std::ratio<detail::BYTES_PER_KIB * detail::BYTES_PER_KIB>>;
using gigabytes =
    data_size<double, std::ratio<detail::BYTES_PER_KIB * detail::BYTES_PER_KIB *
                                 detail::BYTES_PER_KIB>>;
using terabytes =
    data_size<double, std::ratio<detail::BYTES_PER_KIB * detail::BYTES_PER_KIB *
                                 detail::BYTES_PER_KIB * detail::BYTES_PER_KIB>>;

namespace detail
{

template <typename T>
struct is_data_size : std::false_type
{};

template <typename Rep, typename Scale>
struct is_data_size<data_size<Rep, Scale>> : std::true_type
{};

}  // namespace detail

/** Satisfied by any instantiation of @ref data_size. */
template <typename T>
concept data_size_like = detail::is_data_size<std::remove_cvref_t<T>>::value;

/**
 * Printable suffix for a data-size @p Scale (e.g. "MB" for 1024*1024 bytes).
 *
 * Left undefined for unknown scales so that printing an unregistered unit is a
 * compile error; specialise it to add a new unit's suffix.
 */
template <typename Scale>
struct data_size_suffix;

template <>
struct data_size_suffix<std::ratio<1>>
{
    static constexpr std::string_view VALUE = "B";
};
template <>
struct data_size_suffix<std::ratio<detail::BYTES_PER_KIB>>
{
    static constexpr std::string_view VALUE = "KB";
};
template <>
struct data_size_suffix<std::ratio<detail::BYTES_PER_KIB * detail::BYTES_PER_KIB>>
{
    static constexpr std::string_view VALUE = "MB";
};
template <>
struct data_size_suffix<
    std::ratio<detail::BYTES_PER_KIB * detail::BYTES_PER_KIB * detail::BYTES_PER_KIB>>
{
    static constexpr std::string_view VALUE = "GB";
};
template <>
struct data_size_suffix<std::ratio<detail::BYTES_PER_KIB * detail::BYTES_PER_KIB *
                                   detail::BYTES_PER_KIB * detail::BYTES_PER_KIB>>
{
    static constexpr std::string_view VALUE = "TB";
};

/**
 * Convert a data-size to another data-size unit at compile time.
 *
 * Mirrors std::chrono::duration_cast: the factor is the exact rational
 * @c From::scale / @c To::scale, applied in @c To::rep.
 *
 * @tparam To   target data-size type.
 * @tparam From source data-size type (deduced).
 * @param  from value to convert.
 * @return @p from expressed in @p To's unit.
 */
template <data_size_like To, data_size_like From>
[[nodiscard]] constexpr To
data_size_cast(const From& from) noexcept
{
    using from_t     = std::remove_cvref_t<From>;
    using factor     = std::ratio_divide<typename from_t::scale, typename To::scale>;
    using rep        = typename To::rep;
    using calc       = std::common_type_t<rep, typename from_t::rep, std::intmax_t>;
    const auto count = static_cast<calc>(from.count()) * static_cast<calc>(factor::num) /
                       static_cast<calc>(factor::den);
    return To{ static_cast<rep>(count) };
}

/**
 * Equality across any two data-size units.
 *
 * Both operands are converted to bytes before comparing, so `1024_b == 1_kb`.
 */
template <data_size_like LHS, data_size_like RHS>
[[nodiscard]] constexpr bool
operator==(const LHS& lhs, const RHS& rhs) noexcept
{
    using rep  = std::common_type_t<typename LHS::rep, typename RHS::rep>;
    using base = data_size<rep, std::ratio<1>>;
    return data_size_cast<base>(lhs).count() == data_size_cast<base>(rhs).count();
}

/**
 * Ordering across any two data-size units.
 *
 * Both operands are normalised to bytes before comparing, so `1_kb < 2048_b`
 * is well-defined. Returns std::partial_ordering because the underlying rep is
 * floating-point.
 */
template <data_size_like LHS, data_size_like RHS>
[[nodiscard]] constexpr auto
operator<=>(const LHS& lhs, const RHS& rhs) noexcept
{
    using rep  = std::common_type_t<typename LHS::rep, typename RHS::rep>;
    using base = data_size<rep, std::ratio<1>>;
    return data_size_cast<base>(lhs).count() <=> data_size_cast<base>(rhs).count();
}

namespace literals
{

// clang-format off
constexpr bytes     operator""_b (long double value)        noexcept { return bytes{static_cast<double>(value)}; }
constexpr bytes     operator""_b (unsigned long long value) noexcept { return bytes{static_cast<double>(value)}; }
constexpr kilobytes operator""_kb(long double value)        noexcept { return kilobytes{static_cast<double>(value)}; }
constexpr kilobytes operator""_kb(unsigned long long value) noexcept { return kilobytes{static_cast<double>(value)}; }
constexpr megabytes operator""_mb(long double value)        noexcept { return megabytes{static_cast<double>(value)}; }
constexpr megabytes operator""_mb(unsigned long long value) noexcept { return megabytes{static_cast<double>(value)}; }
constexpr gigabytes operator""_gb(long double value)        noexcept { return gigabytes{static_cast<double>(value)}; }
constexpr gigabytes operator""_gb(unsigned long long value) noexcept { return gigabytes{static_cast<double>(value)}; }
constexpr terabytes operator""_tb(long double value)        noexcept { return terabytes{static_cast<double>(value)}; }
constexpr terabytes operator""_tb(unsigned long long value) noexcept { return terabytes{static_cast<double>(value)}; }
// clang-format on

}  // namespace literals

}  // namespace rocprofsys::inline common::units
