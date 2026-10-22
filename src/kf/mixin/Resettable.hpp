// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Resettable.hpp
/// @brief   Adds reset() method to reset object state.

#pragma once

namespace kf::mixin {

struct ResettableTag {};

/// @brief CRTP mixin reset.
/// @tparam Impl The derived class that must implement `reset_impl()`.
template<typename Impl> struct Resettable : ResettableTag {

    /// @brief Reset: full state-reset
    constexpr void reset() noexcept {
        static_cast<Impl *>(this)->reset_impl();
    }
};

}// namespace kf::mixin

#define KF_IMPL_RESETTABLE(...) friend struct ::kf::mixin::Resettable<__VA_ARGS__>
