// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

#pragma once

#include "kf/mixin/NonCopyable.hpp"
#include "kf/mixin/Poll.hpp"
#include "kf/mixin/Resettable.hpp"

namespace kf::tuner {

struct TunerTag {};

/// @brief CRTP base class for all tuners.
/// @tparam Impl Derived class implementing the actual tuning logic.
/// @note Derived classes must provide:
///
///       - `bool running_impl() const noexcept`
///
///       - `void reset_impl() noexcept` (from mixin::Resettable)
///
///       - `void poll_impl(kf::units::Milliseconds now) noexcept` (from mixin::Poll)
template<typename Impl> struct Tuner :

    TunerTag,
    mixin::NonCopyable,
    mixin::Resettable<Impl>,
    mixin::Poll<Impl>

{

    /// @brief Check if the tuner is still running (collecting or calculating).
    [[nodiscard]] bool running() const noexcept {
        return static_cast<Impl const *>(this)->running_impl();
    }
};

}// namespace kf::tuner

#define KF_IMPL_TUNER(...)                       \
    friend struct kf::tuner::Tuner<__VA_ARGS__>; \
    KF_IMPL_RESETTABLE(__VA_ARGS__);             \
    KF_IMPL_POLL(__VA_ARGS__)
