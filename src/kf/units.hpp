// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    units.hpp
/// @brief   Defines common physical units (degrees, pixels, milliseconds, etc.) used across the library.

#pragma once

#include "kf/core.hpp"

namespace kf::units {

/// @brief Angle in degrees
using Degrees = u16;

/// @brief Pixel position coordinate
using Pixels = i16;

/// @brief Physical distance in millimeters
using Millimeters = f64;

/// @brief Time duration in seconds
using Seconds = f32;

/// @brief Frequency in Hertz (Hz = 1/s)
using Hertz = u16;

/// @brief Time duration in milliseconds
using Milliseconds = u32;

/// @brief Time duration in microseconds
using Microseconds = u32;

}// namespace kf::units