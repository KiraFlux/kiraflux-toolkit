// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'listener/gpio-as-button'

#include <kf/gpio.hpp>
#include <kf/listener/LogicalLevelListener.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Clock.hpp>
#include <kf/rtos/Task.hpp>

using kf::listener::LogicalLevelListener;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: listener/gpio-as-button");

    // --- Configuration (MUST outlive the listener instance) ---

    LogicalLevelListener::Config button_config{
        .debounce = 50,// ms debounce time
    };

    // --- GPIO input (moved into the listener) ---

    gpio::DigitalInput button_gpio{
        gpio::G25,
        gpio::DigitalInput::Pull::InternalUp,
    };

    // --- Listener instance ---
    // Config is passed by const reference (must stay alive).
    // GPIO is NOT stored inside the listener; we set it via `set()` in the loop.
    LogicalLevelListener button_listener{
        button_config,// by const reference (MUST outlive this instance)
    };

    // Initialize the GPIO pin (sets pin mode and pull-up).
    button_gpio.init();
    init.logger.info("GPIO initialized");

    // Set up callback for button clicks.
    // The callback is called when a stable press is detected (after debounce).
    button_listener.callback([&]() {
        init.logger.info("Button clicked");
    });

    // Bounded loop: 1000 iterations * 10 ms ≈ 10 seconds.
    for (int i = 0; i < 1000; i += 1) {
        bool level = button_gpio.level();

        // Feed the current level to the listener.
        button_listener.set(level);

        // Poll the listener to detect stable transitions and invoke callbacks.
        // Small delay to avoid busy-loop.
        rtos::Task::sleep(10);
    }

    // Release the GPIO pin (detaches any interrupt, if attached).
    button_gpio.deinit();
    init.logger.info("GPIO deinitialized");
}
