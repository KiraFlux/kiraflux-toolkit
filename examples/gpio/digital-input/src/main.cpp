// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'gpio/digital-input'

// Demonstrates reading a digital input pin with configurable pull resistor.
// The loop is bounded so that deinit() is reachable.

#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: gpio/digital-input");

    // Create a digital input object on GPIO pin 4 with internal pull-down.
    // Pull options: InternalDown (INPUT_PULLDOWN), InternalUp (INPUT_PULLUP), External (INPUT).
    gpio::DigitalInput my_digital_input_pin{
        gpio::G4,
        gpio::DigitalInput::Pull::InternalDown,
    };

    // Initialize the GPIO pin as an input with the specified pull configuration.
    my_digital_input_pin.init();

    // Read and log the pin state every 100 ms for 10 seconds.
    for (int i = 0; i < 100; i += 1) {
        bool const level = my_digital_input_pin.level();
        init.logger.info("level: {}", level);

        // Wait 100 ms before the next read.
        rtos::Task::sleep(100);
    }

    // Release the GPIO pin (detaches any interrupt, if attached).
    my_digital_input_pin.deinit();
    init.logger.info("Digital input deinitialized");
}