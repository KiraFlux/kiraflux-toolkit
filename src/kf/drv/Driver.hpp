// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/Driver.hpp
/// @brief   CRTP base for drivers.

#pragma once

#include "kf/Result.hpp"
#include "kf/core.hpp"

#include "kf/mixin/Initable.hpp"
#include "kf/mixin/NonCopyable.hpp"

namespace kf::drv {

/// @brief Traits marker.
struct DriverTraitsTag {};

// TODO: docs
template<typename C, typename S> struct CommonSpecific {
    using Common = C;
    using Specific = S;

    Common const &common;
    Specific specific;
};

// TODO: docs
template<

    typename C,
    typename E,
    typename... Args

    >
struct DriverTraits : DriverTraitsTag, mixin::InitableTraits<Args...> {
    using Config = C;
    using Extra = E;
};

/// @brief Driver marker.
struct DriverTag {};

/// @brief CRTP base for drivers.
/// @tparam Impl Concrete driver type.
/// @tparam D    Driver traits.
template<typename Impl, implements<DriverTraitsTag> D> struct Driver :

    DriverTag,
    mixin::NonCopyable,
    mixin::Initable<Impl, D>

{

    /// @brief Dependencies captured at construction.
    struct Dependencies {
        typename D::Config config;
        [[no_unique_address]] typename D::Extra extra;
    };

    // TODO: docs
    explicit constexpr Driver(Dependencies const &deps) noexcept :
        _config{deps.config} {}

protected:
    /// @brief Captured dependencies, by value.
    typename D::Config _config;
};

}// namespace kf::drv

#define KF_IMPL_DRIVER(...) KF_IMPL_INITABLE(__VA_ARGS__)
