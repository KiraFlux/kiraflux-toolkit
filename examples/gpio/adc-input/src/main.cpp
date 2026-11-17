// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'gpio/adc-input'

#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::gpio::AdcInput;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: gpio/adc-input");

    AdcInput::resolution(12);

    auto adc_pool = init.arena.create_pool<AdcInput>(1).unwrap();

    auto const result = AdcInput::create(adc_pool, gpio::G35);
    if (result.is_error()) {
        init.logger.error("AdcInput creation failed: {}", result.error());
        return;
    }
    auto &my_adc_input = result.ok();
    kf_defer(
        my_adc_input.destroy(adc_pool);
        init.logger.info("ADC input deinitialized"););

    for (int i = 0; i < 100; i += 1) {
        auto const value = my_adc_input.read();
        init.logger.info("value: {}", value);

        rtos::Task::sleep(100);
    }
}
