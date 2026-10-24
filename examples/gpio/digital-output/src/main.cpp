// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'gpio/digital-output'

// Demonstrates basic digital output control using kf::gpio::DigitalOutput.
// The example toggles a GPIO pin every second, logging the current state.
// The loop is bounded so that deinit() is reachable.

#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: gpio/digital-output");

    gpio::DigitalOutput my_digital_output_pin{
        gpio::G4,
    };

    // Initialize the GPIO pin as an output.
    // This configures the hardware pin mode (OUTPUT).
    my_digital_output_pin.init();

    bool level = true;

    // Toggle the output 10 times (10 seconds).
    for (int i = 0; i < 10; i += 1) {
        my_digital_output_pin.level(level);

        // Log the current state for monitoring.
        init.logger.info("level: {}", level);

        // Toggle the level for the next iteration.
        level = not level;

        // Wait 1 second before the next toggle.
        rtos::Task::sleep(1000);
    }

    // Release the pin (sets output LOW and returns the pin to INPUT mode).
    my_digital_output_pin.deinit();
    init.logger.info("Digital output deinitialized");
}