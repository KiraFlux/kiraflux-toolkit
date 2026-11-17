// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    rtos/Task.hpp
/// @brief   Task sleep function (delay) in milliseconds.

#pragma once

#include "kf/units.hpp"

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <unistd.h>
#endif

namespace kf::rtos {

struct Task {

    static void sleep(units::Milliseconds duration) noexcept {
#ifdef ARDUINO
        ::delay(duration);
#else
        (void) ::usleep(static_cast<useconds_t>(duration) * 1000);
#endif
    }
};

}// namespace kf::rtos