// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Initable.hpp
/// @brief   Adds init() forwarding to init_impl().

#pragma once

#include <utility>// std::forward

namespace kf::mixin {

struct InitableTag {};

/// @brief CRTP mixin that adds an `init` method forwarding to `init_impl`
/// @tparam Impl       Implementation class (must implement `init_impl`)
/// @tparam R          return type
template<typename Impl, typename R> struct Initable : InitableTag {

    /// @brief Initializes the object
    /// @return Value returned by `init_impl`
    [[nodiscard]] R init() noexcept {
        return static_cast<Impl *>(this)->init_impl();
    }
};

template<typename Impl> struct Initable<Impl, void> : InitableTag {

    /// @brief Initializes the object
    void init() noexcept {
        static_cast<Impl *>(this)->init_impl();
    }
};

}// namespace kf::mixin

#define KF_IMPL_INITABLE(...) friend struct ::kf::mixin::Initable<__VA_ARGS__>
