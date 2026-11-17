// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'gpio/digital-input'

#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::gpio::DigitalInput;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: gpio/digital-input");

    auto gpio_pool = init.arena.create_pool<DigitalInput>(1).unwrap();

    auto const result = DigitalInput::create(
        gpio_pool,
        gpio::G4,
        DigitalInput::Pull::InternalDown);

    if (result.is_error()) {
        init.logger.error("DigitalInput creation failed: {}", result.error());
        return;
    }
    auto &my_digital_input_pin = result.ok();
    kf_defer(
        my_digital_input_pin.destroy(gpio_pool);
        init.logger.info("Digital input deinitialized"););

    for (int i = 0; i < 100; i += 1) {
        bool const level = my_digital_input_pin.level();
        init.logger.info("level: {}", level);

        rtos::Task::sleep(100);
    }
}
