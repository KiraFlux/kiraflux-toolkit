// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/Sensor.hpp
/// @brief   CRTP base for sensors (read, init).

#pragma once

#include "kf/core.hpp"

#include "kf/drv/Driver.hpp"

namespace kf::drv {

struct SensorTraitsTag {};

template<typename M, typename... Args> struct SensorTraits : SensorTraitsTag, DriverTraits<Args...> {
    using Measurement = M;
};

struct SensorTag {};

/// @brief Sensor CRTP interface
/// @tparam Impl Sensor implementation
template<typename Impl, implements<SensorTraitsTag> D> struct Sensor :

    SensorTag,
    Driver<Impl, D>

{

    using Driver<Impl, D>::Driver;

    /// @brief Read sensor value
    [[nodiscard]] auto read() noexcept -> typename D::Measurement {
        return static_cast<Impl *>(this)->read_impl();
    }
};

}// namespace kf::drv

#define KF_IMPL_SENSOR(...)                                                    \
    friend struct ::kf::drv::Sensor<__VA_ARGS__, typename Traits::TraitsBase>; \
    KF_IMPL_DRIVER(__VA_ARGS__)
