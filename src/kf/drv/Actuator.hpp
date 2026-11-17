// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/Actuator.hpp
/// @brief   CRTP base for actuators (set, stop, init).

#pragma once

#include "kf/core.hpp"

#include "kf/drv/Driver.hpp"

namespace kf::drv {

struct ActuatorTraitsTag {};

template<typename U, typename... Args> struct ActuatorTraits : ActuatorTraitsTag, DriverTraits<Args...> {
    using Unit = U;
};

struct ActuatorTag {};

/// @brief Actuator Driver CRTP interface
/// @tparam Impl Actuator Driver implementation
template<typename Impl, implements<ActuatorTraitsTag> D> struct Actuator :

    ActuatorTag,
    Driver<Impl, D>

{

    using Driver<Impl, D>::Driver;

    /// @brief Set Control value
    void set(typename D::Unit value) noexcept {
        static_cast<Impl *>(this)->set_impl(value);
    }

    /// @brief Release actuator (disable output)
    void stop() noexcept {
        static_cast<Impl *>(this)->stop_impl();
    }
};

}// namespace kf::drv

#define KF_IMPL_ACTUATOR(...)                                           \
    friend struct ::kf::drv::Actuator<__VA_ARGS__, Traits::TraitsBase>; \
    KF_IMPL_DRIVER(__VA_ARGS__)
