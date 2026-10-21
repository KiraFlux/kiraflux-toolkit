// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/DefaultResettable.hpp
/// @brief   Adds reset() that restores object to default state.

#pragma once

#include <kf/mixin/Resettable.hpp>

namespace kf::mixin {

struct DefaultResettableTag {};

/// @brief CRTP mixin that implements reset as `*this = Impl{}`.
/// @tparam Impl The derived class (must be default-constructible and copy-assignable).
template<typename Impl> struct DefaultResettable :

    DefaultResettableTag,
    Resettable<DefaultResettable<Impl>>

{
private:
    KF_IMPL_RESETTABLE(DefaultResettable<Impl>);
    constexpr void reset_impl() noexcept {
        *static_cast<Impl *>(this) = Impl{};
    }
};

}// namespace kf::mixin