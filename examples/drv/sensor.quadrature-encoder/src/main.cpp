// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/sensor.quadrature-encoder'

#include <kf/drv/QuadratureEncoder.hpp>
#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::drv::QuadratureEncoder;

// Unit type: we use degrees for position.
using UnitType = float;

// Encoder specialization with DigitalInput GPIO and UnitType.
using Encoder = QuadratureEncoder<UnitType>;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/sensor.quadrature-encoder");

    // --- Configuration (MUST outlive the encoder instance) ---

    // Encoder config: 100 units (degrees) per full tick.
    Encoder::Traits::Config::Common encoder_common_config{
        .units_per_tick = 100,// 100 degrees per tick
        .pull = gpio::DigitalInput::Pull::External,
    };

    // --- Encoder instance ---

    auto encoder_pool = init.arena.create_pool<Encoder>(1).unwrap();
    auto gpio_pool = init.arena.create_pool<gpio::DigitalInput>(2).unwrap();

    auto creation_result = Encoder::create(
        encoder_pool,
        gpio_pool,
        Encoder::Dependencies{.config{
            .common = encoder_common_config,
            .specific{
                // Swapping A and B reverses the count direction.
                .gpio_num_phase_a = gpio::G25,
                .gpio_num_phase_b = gpio::G26,
            },
        }});

    if (creation_result.is_error()) {
        init.logger.error("failed to create encoder: {}", creation_result.error());
        return;
    }

    auto &encoder = creation_result.ok();
    kf_defer(
        encoder.destroy(encoder_pool, gpio_pool);
        init.logger.info("Encoder deinitialized"););

    // Initialize the encoder (sets up GPIO and ISR).
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
}
