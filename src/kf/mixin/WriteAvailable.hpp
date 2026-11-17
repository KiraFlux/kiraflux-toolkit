// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/WriteAvailable.hpp
/// @brief   Adds available_for_write() to query free space.

#pragma once

#include "kf/core.hpp"

namespace kf::mixin {

struct WriteAvailableTag {};

/// @brief Adds `available_for_write()` to query how many bytes/items can be written without blocking.
/// @tparam Impl Implementation class with `usize available_for_write_impl() const noexcept` method.
/// @note The value is advisory; actual write may still block if buffer is shared.
template<typename Impl> struct WriteAvailable : WriteAvailableTag {

    /// @brief Get the amount of free space available for writing.
    [[nodiscard]] usize available_for_write() const noexcept {
        return static_cast<Impl const *>(this)->available_for_write_impl();
    }
};

}// namespace kf::mixin

#define KF_IMPL_WRITE_AVAILABLE(...) friend struct ::kf::mixin::WriteAvailable<__VA_ARGS__>
