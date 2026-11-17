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

    auto gpio_pool = init.arena.create_pool<gpio::DigitalInput>(1).unwrap();

    auto const result = gpio::DigitalInput::create(
        gpio_pool,
        gpio::G25,
        gpio::DigitalInput::Pull::InternalDown);

    if (result.is_error()) {
        init.logger.error("DigitalInput creation failed: {}", result.error());
        return;
    }
    auto &button_gpio = result.ok();
    kf_defer(
        init.logger.info("GPIO deinitialized");
        button_gpio.destroy(gpio_pool););

    // --- Listener instance ---
    // Config is passed by const reference (must stay alive).
    // GPIO is NOT stored inside the listener; we set it via `set()` in the loop.
    LogicalLevelListener button_listener{
        button_config,// by const reference (MUST outlive this instance)
    };

    // Initialize the GPIO pin (sets pin mode and pull-up).
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
}
