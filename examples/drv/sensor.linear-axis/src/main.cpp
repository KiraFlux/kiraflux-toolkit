// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/sensor.linear-axis'

// Single analog axis with calibration:
//   - send any byte on the serial to confirm the current extreme
//   - move to MIN, confirm; move to MAX, confirm
//   - read normalized value in [-1, +1]

#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Clock.hpp>
#include <kf/rtos/Task.hpp>

#include <kf/drv/LinearAxis.hpp>

using kf::drv::LinearAxis;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/sensor.linear-axis");

    // Common config lives by reference inside Dependencies - must outlive the driver.
    LinearAxis::Traits::Config::Common common_config{
        .filter = {.factor = 0.6f},
    };

    auto adc_pool = init.arena.create_pool<gpio::AdcInput>(1).unwrap();
    auto axis_pool = init.arena.create_pool<LinearAxis>(1).unwrap();

    auto const result = LinearAxis::create(
        axis_pool,
        adc_pool,
        LinearAxis::Dependencies{
            .config{
                .common = common_config,
                .specific{
                    .gpio_num = gpio::G34,
                    .inverted = false,
                    .calibrated = false,
                    .center = 0,
                    .dead_zone = 0,
                    .range_positive = 0,
                    .range_negative = 0,
                },
            },
        });

    if (result.is_error()) {
        init.logger.error("LinearAxis: {}", result.error());
        return;
    }
    auto &axis = result.ok();
    kf_defer(
        axis.destroy(axis_pool, adc_pool);
        init.logger.debug("axis destroyed"););
    init.logger.info("LinearAxis ready on G34");

    // --- Calibration ---
    LinearAxis::Tuner tuner{axis};
    tuner.reset();

    init.logger.info("Move axis to MIN, then press any key");
    auto last = LinearAxis::Tuner::State::Done;

    while (tuner.running()) {
        tuner.poll(rtos::Clock::now());

        auto const cur = tuner.state();
        if (cur != last) {
            if (cur == LinearAxis::Tuner::State::WaitEnd) {
                init.logger.info("Move axis to MAX, then press any key");
            }
            last = cur;
        }

        if (cur == LinearAxis::Tuner::State::WaitStart or cur == LinearAxis::Tuner::State::WaitEnd) {
            if (init.io.read_byte().is_ok()) {
                tuner.confirm();
            }
        }

        rtos::Task::sleep(10);
    }

    if (not tuner.calibrated()) {
        init.logger.error("Calibration failed: span < 25% of ADC range");
        return;
    }
    init.logger.info("Calibration OK");

    // --- Read loop (bounded, ~10 s) ---
    for (int i = 0; i < 100; i += 1) {
        init.logger.debug("raw={} norm={}", axis.read_raw(), axis.read());
        rtos::Task::sleep(100);
    }
}
