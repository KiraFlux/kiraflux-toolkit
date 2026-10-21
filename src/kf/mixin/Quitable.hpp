// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Quitable.hpp
/// @brief   Adds quit() for resource release.

#pragma once

namespace kf::mixin {

struct QuitableTag {};

/// @brief CRTP mixin quit
/// @tparam Impl The derived class that must implement `quit_impl()`
template<typename Impl> struct Quitable : QuitableTag {

    /// @brief Releases resources and shuts down the object
    void quit() noexcept {
        static_cast<Impl *>(this)->quit_impl();
    }
};

}// namespace kf::mixin

#define KF_IMPL_QUITABLE(...) friend struct ::kf::mixin::Quitable<__VA_ARGS__>
