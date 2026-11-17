// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/DRV8871.hpp
/// @brief   DRV8871 motor driver (H‑bridge) with dead‑zone compensation.

#pragma once

#include "kf/Pool.hpp"
#include "kf/StringView.hpp"
#include "kf/gpio.hpp"
#include "kf/math.hpp"

#include "kf/mixin/Configured.hpp"

#include "kf/drv/Actuator.hpp"

namespace kf::internal {

/// @brief Configuration for the DRV8871 motor driver
struct DRV8871CommonConfig {
    using InputType = kf::i16;///< Signed input command type

    gpio::PwmOutput::Config pwm;         ///< PWM configuration (shared with both GPIO objects)
    InputType max_input;                 ///< Maximum absolute input value (e.g., 1000)
    gpio::PwmOutput::Duty duty_dead_zone;///< Minimum PWM duty to start moving forward

    /// @brief Map speed command to PWM duty cycle respecting dead zone
    [[nodiscard]] constexpr gpio::PwmOutput::Duty calc_duty(InputType value) const noexcept {
        auto const clamped_input = math::clamp(value, 0, max_input);
        return static_cast<gpio::PwmOutput::Duty>(math::linear_map(clamped_input, 0, max_input, duty_dead_zone, pwm.max_duty()));
    }
};

struct DRV8871SpecificConfig {
    gpio::GpioNumber gpio_num_forward, gpio_num_backward;
};

enum struct DRV8871Error {
    NoSlot,
    ForwardPwmInitFail,
    BackwardPwmInitFail,
};

template<typename Impl> using DRV8871Base = drv::Actuator<
    Impl,
    drv::ActuatorTraits<
        DRV8871CommonConfig::InputType,
        drv::CommonSpecific<DRV8871CommonConfig, DRV8871SpecificConfig>,
        NoneType,
        DRV8871Error,
        Pool<gpio::PwmOutput> &>>;

}// namespace kf::internal

namespace kf {

namespace drv {

/// @brief DRV8871 H-bridge motor driver abstraction
struct DRV8871 : internal::DRV8871Base<DRV8871> {

    using internal::DRV8871Base<DRV8871>::DRV8871Base;

private:
    using G = gpio::PwmOutput;

    G *_pwm_gpio_forward{}, *_pwm_gpio_backward{};

    KF_IMPL_ACTUATOR(DRV8871);

    auto init_impl(Traits::Resource &gpio_pool) noexcept -> Traits::ActionResult {

        if (auto r = G::create(gpio_pool, this->_config.common.pwm, this->_config.specific.gpio_num_forward); r.is_ok()) {
            _pwm_gpio_forward = &r.ok();
        } else {
            return error(Traits::Error::ForwardPwmInitFail);
        }

        if (auto r = G::create(gpio_pool, this->_config.common.pwm, this->_config.specific.gpio_num_backward); r.is_ok()) {
            _pwm_gpio_backward = &r.ok();
        } else {
            G::destroy(_pwm_gpio_forward, gpio_pool);
            return error(Traits::Error::BackwardPwmInitFail);
        }

        return ok();
    }

    void deinit_impl(Traits::Resource &gpio_pool) noexcept {
        this->set(0);
        _pwm_gpio_forward->destroy(gpio_pool);
        _pwm_gpio_backward->destroy(gpio_pool);
    }

    void set_impl(Traits::Unit value) noexcept {
        if (value < 0) {
            _pwm_gpio_forward->write(0);
            _pwm_gpio_backward->write(this->_config.common.calc_duty(-value));
        } else {
            _pwm_gpio_forward->write(this->_config.common.calc_duty(value));
            _pwm_gpio_backward->write(0);
        }
    }

    void stop_impl() noexcept {
        _pwm_gpio_forward->write(0);
        _pwm_gpio_backward->write(0);
    }
};

}// namespace drv

#define CASE_RETURN(__v) \
    case __v: return #__v

[[nodiscard]] constexpr StringView repr(drv::DRV8871::Traits::Error e) noexcept {
    switch (e) {
        CASE_RETURN(drv::DRV8871::Traits::Error::NoSlot);
        CASE_RETURN(drv::DRV8871::Traits::Error::ForwardPwmInitFail);
        CASE_RETURN(drv::DRV8871::Traits::Error::BackwardPwmInitFail);
    }
    return "todo";
}

}// namespace kf

#undef CASE_RETURN
