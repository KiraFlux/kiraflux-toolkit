// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Capacity.hpp
/// @brief   Adds capacity() to a container.

#pragma once

#include "kf/core.hpp"

namespace kf::mixin {

struct CapacityTag {};

/// @brief Adds `capacity()`.
/// @tparam Impl Implementation class with `constexpr T capacity_impl() const noexcept`.
template<typename Impl> struct Capacity : CapacityTag {

    /// @brief Maximum number of items the container can hold.
    [[nodiscard]] constexpr usize capacity() const noexcept {
        return static_cast<Impl const *>(this)->capacity_impl();
    }
};

}// namespace kf::mixin

#define KF_IMPL_CAPACITY(...) friend struct ::kf::mixin::Capacity<__VA_ARGS__>
