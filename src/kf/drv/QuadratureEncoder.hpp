// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/QuadratureEncoder.hpp
/// @brief   Quadrature encoder with 4X decoding and units conversion.

#pragma once

#include "kf/Pool.hpp"
#include "kf/Result.hpp"
#include "kf/core.hpp"
#include "kf/gpio.hpp"

#include "kf/mixin/Reset.hpp"

#include "kf/drv/Sensor.hpp"

namespace kf::internal {

/// @brief Configuration for a QuadratureEncoder
/// @tparam T Physical unit type representing linear distance per tick
template<typename T> struct QuadratureEncoderCommonConfig final {

    using TickType = kf::isize;    ///< Integral tick counter type
    using UnitType = T;            ///< Physical unit type
    using PhaseStateType = kf::u32;///< Packed two-bit phase state (AB)

    UnitType units_per_tick;      ///< Conversion factor: physical units per encoder tick
    gpio::DigitalInput::Pull pull;///< Phase Pull type

    /// @brief Converts ticks to physical units
    [[nodiscard]] constexpr UnitType units_from_ticks(TickType value) const noexcept {
        return static_cast<UnitType>(value) * units_per_tick;
    }

    /// @brief Converts physical units to ticks (truncated)
    /// @note For integral unit types, precision loss may occur due to integer division
    [[nodiscard]] constexpr TickType ticks_from_units(UnitType value) const noexcept {
        return static_cast<TickType>(value / units_per_tick);
    }
};

struct QuadratureEncoderSpecificConfig {
    gpio::GpioNumber gpio_num_phase_a, gpio_num_phase_b;
};

enum struct QuadratureEncoderError {
    NoSlot,
    GpioInitFail,
};

template<typename Impl, typename T> using QuadratureEncoderBase = drv::Sensor<
    Impl,
    drv::SensorTraits<
        typename QuadratureEncoderCommonConfig<T>::TickType,
        drv::CommonSpecific<QuadratureEncoderCommonConfig<T>, QuadratureEncoderSpecificConfig>,
        NoneType,
        QuadratureEncoderError,
        Pool<gpio::DigitalInput> &>>;

}// namespace kf::internal

namespace kf::drv {

/// @brief Quadrature encoder sensor with 4X decoding
/// @note Positive rotation direction is determined by the order of phase A and B pins.
///       Swapping A and B reverses the count direction.
/// @tparam T Physical linear unit
template<typename T> struct QuadratureEncoder :

    internal::QuadratureEncoderBase<QuadratureEncoder<T>, T>,
    mixin::Reset<QuadratureEncoder<T>>

{

    using Self = QuadratureEncoder<T>;
    using Base = internal::QuadratureEncoderBase<Self, T>;
    using typename Base::Traits;

    using Base::Base;

    /// @brief Current accumulated position in ticks
    [[nodiscard]] auto position_ticks() const noexcept -> typename Traits::Config::Common::TickType {
        return _position_ticks;
    }

    /// @brief Overwrite the current tick count
    void position_ticks(Traits::Config::Common::TickType position) noexcept {
        _position_ticks = position;
    }

    /// @brief Current position converted to physical units
    [[nodiscard]] auto position_units() const noexcept -> typename Traits::Measurement {
        return this->_config.common.units_from_ticks(_position_ticks);
    }

    /// @brief Set position in physical units (converted to ticks)
    void position_units(Traits::Measurement position) noexcept {
        _position_ticks = this->_config.common.ticks_from_units(position);
    }

private:
    using G = gpio::DigitalInput;
    G *_gpio_phase_a{}, *_gpio_phase_b{};
    typename Traits::Config::Common::TickType volatile _position_ticks{0};  ///< Accumulated step count
    typename Traits::Config::Common::PhaseStateType volatile _last_state{0};///< Previous AB phase state

    /// @brief ISR triggered on any edge of either phase
    /// @note Avoid `switch` because it may generate a jump table in flash, unreachable from IRAM due to `l32r` range limits.
    // FIXME: linker issues if IRAM ATTR
    static void on_any_phase_change(void *arg) noexcept {
        auto &self = *static_cast<Self *>(arg);

        auto const current_state = self.read();

        // Index formed by concatenating last and current states (4 bits)
        // 4X decoding lookup table (index = (last_A << 3 | last_B << 2 | cur_A << 1 | cur_B))
        auto const index = (self._last_state << 2) | current_state;

        // cannot use switch case where cuz lookup table is forbidden in IRAM ISR handler
        if (
            (index == 0b00'01) or
            (index == 0b01'11) or
            (index == 0b10'00) or
            (index == 0b11'10)) {
            self._position_ticks += 1;
        } else if (
            (index == 0b00'10) or
            (index == 0b01'00) or
            (index == 0b10'11) or
            (index == 0b11'01)) {
            self._position_ticks -= 1;
        }

        self._last_state = current_state;
    }

    KF_IMPL_SENSOR(Self);

    auto init_impl(Traits::Resource gpio_pool) noexcept -> typename Traits::ActionResult {

        if (auto r = G::create(gpio_pool, this->_config.specific.gpio_num_phase_a, this->_config.common.pull); r.is_ok()) {
            _gpio_phase_a = &r.ok();
        } else {
            return error(Traits::Error::GpioInitFail);
        }

        if (auto r = G::create(gpio_pool, this->_config.specific.gpio_num_phase_b, this->_config.common.pull); r.is_ok()) {
            _gpio_phase_b = &r.ok();
        } else {
            G::destroy(_gpio_phase_a, gpio_pool);
            return error(Traits::Error::GpioInitFail);
        }

        _gpio_phase_a->attach_interrupt(on_any_phase_change, static_cast<void *>(this), G::Interrupt::OnChange);
        _gpio_phase_b->attach_interrupt(on_any_phase_change, static_cast<void *>(this), G::Interrupt::OnChange);
        this->reset();

        return ok();
    }

    void deinit_impl(Traits::Resource gpio_pool) noexcept {
        _gpio_phase_a->destroy(gpio_pool);
        _gpio_phase_b->destroy(gpio_pool);
    }

    auto read_impl() const noexcept -> typename Traits::Config::Common::PhaseStateType {
        auto const state_a = static_cast<Traits::Config::Common::PhaseStateType>(_gpio_phase_a->level());
        auto const state_b = static_cast<Traits::Config::Common::PhaseStateType>(_gpio_phase_b->level());
        return (state_a << 1) | state_b;// pack as AB
    }

    KF_IMPL_RESETTABLE(Self);
    void reset_impl() noexcept {
        _position_ticks = 0;
        _last_state = this->read();
    }
};

}// namespace kf::drv