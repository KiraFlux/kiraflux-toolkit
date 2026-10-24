// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Initable.hpp
/// @brief   Adds init() forwarding to init_impl().

#pragma once

#include <utility>// std::forward

namespace kf {

namespace mixin {

struct InitableTag {};

}// namespace mixin

namespace internal {

template<typename Impl> struct InitableBase : mixin::InitableTag {

    /// @brief Uninitializes the object
    void deinit() noexcept {
        return static_cast<Impl *>(this)->deinit_impl();
    }
};

}// namespace internal

namespace mixin {

/// @brief CRTP mixin that adds an `init`, 'deinit' method forwarding to `init_impl`
/// @tparam Impl       Implementation class (must implement `init_impl`)
/// @tparam R          return type
template<typename Impl, typename R> struct Initable : internal::InitableBase<Impl> {

    /// @brief Initializes the object
    /// @return Value returned by `init_impl`
    [[nodiscard]] R init() noexcept {
        return static_cast<Impl *>(this)->init_impl();
    }
};

template<typename Impl> struct Initable<Impl, void> : internal::InitableBase<Impl> {

    /// @brief Initializes the object
    void init() noexcept {
        static_cast<Impl *>(this)->init_impl();
    }
};

}// namespace mixin

}// namespace kf

#define KF_IMPL_INITABLE(__impl__, ...) \
    friend struct ::kf::mixin::Initable<__impl__, __VA_ARGS__>;\
    friend struct ::kf::internal::InitableBase<__impl__>

