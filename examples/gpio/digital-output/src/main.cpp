// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'gpio/digital-output'

#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::gpio::DigitalOutput;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: gpio/digital-output");

    auto gpio_pool = init.arena.create_pool<DigitalOutput>(1).unwrap();

    auto const result = DigitalOutput::create(gpio_pool, gpio::G4);
    if (result.is_error()) {
        init.logger.error("DigitalOutput creation failed: {}", result.error());
        return;
    }
    auto &my_digital_output_pin = result.ok();
    kf_defer(
        my_digital_output_pin.destroy(gpio_pool);
        init.logger.info("Digital output deinitialized"););

    bool level = true;

    for (int i = 0; i < 10; i += 1) {
        my_digital_output_pin.level(level);
        init.logger.info("level: {}", level);

        level = not level;

        rtos::Task::sleep(1000);
    }
}
