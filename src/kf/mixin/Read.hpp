// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Read.hpp
/// @brief   Adds read() -> Option<T> for consuming items.

#pragma once

#include <utility>

#include "kf/Option.hpp"

namespace kf::mixin {

struct ReadTag {};

/// @brief Add `read() -> Option<T>`
/// @tparam Impl Read mixin implementation class
/// @tparam T Value type
template<typename Impl, typename T> struct Read : ReadTag {

    /// @brief Read Value
    /// @return Option with moved value if read success, `none` otherwise
    [[nodiscard]] constexpr auto read() -> Option<T> {
        return static_cast<Impl *>(this)->read_impl();
    }
};

}// namespace kf::mixin

#define KF_IMPL_READ(...) friend struct ::kf::mixin::Read<__VA_ARGS__>
