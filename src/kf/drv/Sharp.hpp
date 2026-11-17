// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/Sharp.hpp
/// @brief   Sharp IR distance sensor (analog output, mm).

#pragma once

#include "kf/Pool.hpp"
#include "kf/Result.hpp"
#include "kf/gpio.hpp"
#include "kf/units.hpp"

#include "kf/drv/Sensor.hpp"

namespace kf::internal {

enum struct SharpError {
    NoSlot,
    GpioCreateFail,
};

struct SharpSpecificConfig {
    gpio::GpioNumber gpio_num;
};

template<typename Impl> using SharpBase = drv::Sensor<
    Impl,
    drv::SensorTraits<
        units::Millimeters,
        SharpSpecificConfig,
        NoneType,
        SharpError,
        Pool<gpio::AdcInput> &>>;

}// namespace kf::internal

namespace kf::drv {

/// @brief Sharp infrared distance sensor driver
/// @note Provides distance measurements in millimeters using analog voltage output
struct Sharp : internal::SharpBase<Sharp> {

    using internal::SharpBase<Sharp>::SharpBase;

private:
    gpio::AdcInput *_adc_input{};

    KF_IMPL_SENSOR(Sharp);

    auto init_impl(Traits::Resource &gpio_pool) noexcept -> Traits::ActionResult {
        auto const result = gpio::AdcInput::create(
            gpio_pool,
            this->_config.gpio_num);

        if (result.is_error()) {
            return error(Traits::Error::GpioCreateFail);
        }

        _adc_input = &result.ok();
        return ok();
    }

    void deinit_impl(Traits::Resource &gpio_pool) noexcept {
        _adc_input->destroy(gpio_pool);
    }

    auto read_impl() noexcept -> Traits::Measurement {
        // correct calculus only with 10-bit ADC resolution
        // TODO: generalize formula
        return 65535.0F / Traits::Measurement(_adc_input->read());
    }
};

}// namespace kf::drv
