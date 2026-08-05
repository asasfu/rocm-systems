/* Copyright (c) Advanced Micro Devices, Inc., or its affiliates. All rights reserved. */
/**
 ***********************************************************************************************************************
 * @file  palTime.h
 * @brief PAL time-related utility collection.
 ***********************************************************************************************************************
 */

#pragma once

#include <chrono>
#include <time.h>

namespace Util
{

/// Specifies a class that implements a timestamp.
class Timestamp
{
public:
    /// Creates a new timestamp object that records the time it was created.
    Timestamp();

    /// Returns the timestamp as a C-string.
    const char* CStr() const { return m_data; }

private:
    char m_data[64];
};

/// Returns the current time in the local timezone.
struct tm GetCurrentLocalTime();

/// Seconds stored as a float instead of an integer.
using fseconds      = std::chrono::duration<float>;
/// Milliseconds stored as a float instead of an integer.
using fmilliseconds = std::chrono::duration<float, std::milli>;
/// Microseconds stored as a float instead of an integer.
using fmicroseconds = std::chrono::duration<float, std::micro>;
/// Nanoseconds stored as a float instead of an integer.
using fnanoseconds  = std::chrono::duration<float, std::nano>;

/// A time_point who's epoch is January 1st 1970 and uses seconds for the duration.
/// C++20 guarantees us that system_clock's epoch is always January 1st 1970 on all platforms.
/// system_clock's internal duration is still implementation defined.
/// On Windows it's hundreds-of-nanoseconds and on Linux it's seconds.
/// However time_point has it's own duration type.
/// As long as we go through the time_point to interpret the duration then everything should be in terms of seconds.
using SecondsSinceEpoch = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

/// Like std::chrono::duration_cast, but it preserves the special 'infinite' value used in timeouts.
template<class DestDuration, class Rep, class Period>
constexpr DestDuration TimeoutCast(
    const std::chrono::duration<Rep, Period>& d)
{
    if (d == (std::chrono::duration<Rep, Period>::max)())
    {
        return (DestDuration::max)();
    }
    else
    {
        return std::chrono::duration_cast<DestDuration, Rep, Period>(d);
    }
}

} // Util
