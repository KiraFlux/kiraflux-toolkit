// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Equatable.hpp
/// @brief   Adds == and != operators via CRTP.

#pragma once

namespace kf::mixin {

/// @brief Adds `==` and `!=`
/// @tparam Impl Equatable Implementation class with `constexpr bool is_equals_impl(const Impl &) const noexcept`
template<typename Impl> struct Equatable {

    [[nodiscard]] constexpr bool operator==(Impl const &other) const noexcept {
        return static_cast<Impl const *>(this)->is_equals_impl(other);
    }

    [[nodiscard]] constexpr bool operator!=(Impl const &other) const noexcept {
        return not(*this == other);
    }
};

}// namespace kf::mixin

#define KF_IMPL_EQUATABLE(...) friend struct ::kf::mixin::Equatable<__VA_ARGS__>
