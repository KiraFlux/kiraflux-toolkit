// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/Servo.hpp
/// @brief   Generic RC servo driver (50 Hz PWM, 500–2500 us typical).
///          Compatible with SG90, MG90S, MG995, MG996R, DS3218, etc.

#pragma once

#include "kf/Option.hpp"
#include "kf/Pool.hpp"
#include "kf/gpio.hpp"
#include "kf/math.hpp"
#include "kf/units.hpp"

#include "kf/drv/Actuator.hpp"

namespace kf::internal {

struct ServoCommonConfig final {
    using AngleRange = math::Range<units::Degrees>;
    using PulseRange = math::Range<units::Microseconds>;

    gpio::PwmOutput::Config pwm;
    AngleRange angle_range;
    PulseRange pulse_range;

    /// @brief Convert angle to pulse width using linear interpolation
    /// @param angle Target servo angle
    /// @return Required pulse width in microseconds
    [[nodiscard]] constexpr units::Microseconds pulse_width_from_angle(units::Degrees angle) const noexcept {
        return math::linear_map<i32>(angle, angle_range.start, angle_range.end, pulse_range.start, pulse_range.end);
    }
};

struct ServoSpecificConfig final {
    gpio::GpioNumber gpio_num;///< PWM output pin

    /// @brief Optional additional clamp (e.g., mechanical limits).
    /// @note After `init_impl` this is normalized to a `some()` value:
    ///       `requested ∩ common.angle_range`.
    Option<ServoCommonConfig::AngleRange> angle_safe_range{none};
};

enum struct ServoError {
    NoSlot,
    PwmInitFailed,
};

template<typename Impl> using ServoBase = drv::Actuator<
    Impl,
    drv::ActuatorTraits<
        units::Degrees,
        drv::CommonSpecific<ServoCommonConfig, ServoSpecificConfig>,
        NoneType,
        ServoError,
        Pool<gpio::PwmOutput> &>>;

}// namespace kf::internal

namespace kf::drv {

/// @brief PWM-controlled position servo driver for ESP32 LEDC hardware
/// @note Converts angular positions to PWM pulse widths for standard RC servos
struct Servo : internal::ServoBase<Servo> {

    using internal::ServoBase<Servo>::ServoBase;

private:
    gpio::PwmOutput *_pwm_output{};

    KF_IMPL_ACTUATOR(Servo);

    auto init_impl(Traits::Resource pwm_pool) noexcept -> Traits::ActionResult {
        auto const &common = this->_config.common;
        auto &specific = this->_config.specific;

        if (auto r = gpio::PwmOutput::create(pwm_pool, common.pwm, specific.gpio_num); r.is_ok()) {
            _pwm_output = &r.ok();
        } else {
            return error(Traits::Error::PwmInitFailed);
        }

        // Normalize safe range in-place: requested (or full) ∩ common.angle_range.
        auto const requested = specific.angle_safe_range.unwrap_or(common.angle_range);
        specific.angle_safe_range.unwrap().start = math::max(requested.start, common.angle_range.start);
        specific.angle_safe_range.unwrap().end = math::min(requested.end, common.angle_range.end);

        return ok();
    }

    void deinit_impl(Traits::Resource pwm_pool) noexcept {
        stop_impl();
        _pwm_output->destroy(pwm_pool);
    }

    void set_impl(units::Degrees angle) noexcept {
        auto const &safe = this->_config.specific.angle_safe_range.unwrap();
        _pwm_output->write_pulse(this->_config.common.pulse_width_from_angle(safe.clamped(angle)));
    }

    void stop_impl() noexcept {
        _pwm_output->write(0);
    }
};

}// namespace kf::drv
