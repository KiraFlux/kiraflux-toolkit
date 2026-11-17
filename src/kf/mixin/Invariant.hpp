// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Invariant.hpp
/// @brief   Adds is_some()/is_none() for optional‑like types.

#pragma once

namespace kf::mixin {

/// @brief CRTP mixin that provides state checking (Some/None) for optional-like types
/// @tparam Impl The derived class (must implement `bool is_some_impl() const noexcept`).
/// @note Adds `is_some()` and `is_none()` to the derived class.
template<typename Impl> struct Invariant {

    /// @brief Checks whether the object contains a Some
    /// @return `true` if a value is present, `false` otherwise.
    [[nodiscard]] constexpr bool is_some() const noexcept {
        return static_cast<Impl const *>(this)->is_some_impl();
    }

    /// @brief Checks whether the object is None
    /// @return `true` if empty, `false` otherwise.
    [[nodiscard]] constexpr bool is_none() const noexcept {
        return not is_some();
    }
};

}// namespace kf::mixin

#define KF_IMPL_INVARIANT(...) friend struct ::kf::mixin::Invariant<__VA_ARGS__>
