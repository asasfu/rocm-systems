// Copyright (c) Advanced Micro Devices, Inc.
// SPDX-License-Identifier: MIT

#pragma once

// Aggregate include for all type-safe unit types.
// Does NOT pull in fmt formatters; include format.hpp explicitly in
// translation units that need fmt output.

#include "common/units/chrono.hpp"
#include "common/units/data_size.hpp"
#include "common/units/frequency.hpp"
#include "common/units/power.hpp"
