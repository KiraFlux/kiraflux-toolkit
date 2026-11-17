// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/sensor.sharp'

#include <kf/drv/Sharp.hpp>
#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::drv::Sharp;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/sensor.sharp");

    // --- Set ADC resolution globally ---
    // The Sharp formula assumes 10-bit resolution (0..1023).
    // Must be set before any read() to match the sensor's expected range.
    gpio::AdcInput::resolution(10);
    init.logger.debug("ADC resolution set to 10 bits");

    // --- ADC pool (arena-backed, single slot for the sensor) ---

    auto adc_pool = init.arena.create_pool<gpio::AdcInput>(1).unwrap();

    // --- Sensor instance ---

    auto sensor_pool = init.arena.create_pool<Sharp>(1).unwrap();

    auto result = Sharp::create(
        sensor_pool,
        adc_pool,
        Sharp::Dependencies{.config{
            .gpio_num = gpio::G34,
        }});

    if (result.is_error()) {
        init.logger.error("Sharp init failed: {}", result.error());
        return;
    }

    auto &distance_sensor = result.ok();
    kf_defer(
        distance_sensor.destroy(sensor_pool, adc_pool);
        init.logger.info("Sharp sensor deinitialized"););

    init.logger.info("Sharp sensor initialized");

    // Small delay for hardware stabilization.
    rtos::Task::sleep(100);

    // --- Main loop: read distance every 50 ms for 10 seconds ---

    for (int i = 0; i < 200; i += 1) {
        auto const distance_mm = distance_sensor.read();

        init.logger.info("Distance: {} mm", distance_mm);
        rtos::Task::sleep(50);
    }
}
