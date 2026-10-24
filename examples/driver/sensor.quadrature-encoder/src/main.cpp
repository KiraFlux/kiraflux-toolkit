// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'driver/sensor.quadrature-encoder'

#include <kf/driver/sensor/QuadratureEncoder.hpp>
#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::driver::sensor::QuadratureEncoder;

// Unit type: we use degrees for position.
using UnitType = float;

// Encoder specialization with DigitalInput GPIO and UnitType.
using Encoder = QuadratureEncoder<UnitType>;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: driver/sensor.quadrature-encoder");

    // --- Configuration (MUST outlive the encoder instance) ---

    // Encoder config: 100 units (degrees) per full tick.
    Encoder::Config encoder_config{
        .units_per_tick = 100,// 100 degrees per tick
        .pull = gpio::DigitalInput::Pull::External,
    };

    // --- Encoder instance ---

    // Config is passed by const reference (must stay alive).
    // GPIO pins are constructed into the encoder.
    // Swapping A and B reverses the count direction.
    Encoder encoder{
        encoder_config,// by const reference
        gpio::G25,     // gpio number phase A
        gpio::G26,     // gpio number phase B
    };

    // Initialize the encoder (sets up GPIO and ISR).
    encoder.init();
    init.logger.info("Encoder initialized");

    // Small delay to let hardware stabilize.
    rtos::Task::sleep(100);

    // --- Show initial position ---

    init.logger.info("Initial position: {} ticks, {} units",
                     encoder.position_ticks(),
                     encoder.position_units());

    // Bounded loop: 100 iterations * 200 ms = 20 seconds.
    for (int i = 0; i < 100; i += 1) {
        auto ticks = encoder.position_ticks();

        // Convert ticks to physical units using the config.
        auto units = encoder.position_units();

        init.logger.debug("Position: {} ticks, {} units", ticks, units);

        // Wait a bit before next read.
        rtos::Task::sleep(200);
    }

    // CRITICAL: deinit() detaches the ISR on both phase pins.
    // Without this call, GPIO interrupts remain armed after main() returns,
    // which is undefined behaviour on ESP32.
    encoder.deinit();
    init.logger.info("Encoder deinitialized");
}