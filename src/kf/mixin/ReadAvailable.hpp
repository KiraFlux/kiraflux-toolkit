// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/ReadAvailable.hpp
/// @brief   Adds available_for_read() to query available data.

#pragma once

#include "kf/core.hpp"

namespace kf::mixin {

struct ReadAvailableTag {};

/// @brief Adds `available_for_read()` to query how many bytes/items can be read without blocking.
/// @tparam Impl Implementation class with `usize available_for_read_impl() const noexcept` method.
/// @note The value may change between calls; intended for non‑blocking polling.
template<typename Impl> struct ReadAvailable : ReadAvailableTag {

    /// @brief Get the number of bytes/items currently available to read.
    [[nodiscard]] usize available_for_read() const noexcept {
        return static_cast<Impl const *>(this)->available_for_read_impl();
    }
};

}// namespace kf::mixin

#define KF_IMPL_READ_AVAILABLE(...) friend struct ::kf::mixin::ReadAvailable<__VA_ARGS__>
