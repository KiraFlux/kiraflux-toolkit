// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Flush.hpp
/// @brief   Adds flush() to force output of buffered data.

#pragma once

namespace kf::mixin {

struct FlushTag {};

/// @brief Adds `flush()` to force transmission of buffered data.
/// @tparam Impl Flush implementation class with `void flush_impl() noexcept` method
template<typename Impl> struct Flush : FlushTag {

    /// @brief Flush
    void flush() noexcept {
        static_cast<Impl *>(this)->flush_impl();
    }
};

}// namespace kf::mixin

#define KF_IMPL_FLUSH(...) friend struct ::kf::mixin::Flush<__VA_ARGS__>
