// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    ui/Style.hpp
/// @brief   Widget style with foreground and background semantic colors.

#pragma once

#include "kf/ui/Color.hpp"

namespace kf::ui {

/// @brief Widget style
struct Style {
    Color foreground_color;///< Text color
    Color background_color;///< Background color

    /// @brief Create default style
    [[nodiscard]] static constexpr Style defaults() noexcept {
        return Style{
            .foreground_color = Color::Normal,
            .background_color = Color::Normal,
        };
    }
};

}// namespace kf::ui