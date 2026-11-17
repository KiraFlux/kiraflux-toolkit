// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'gpio/pwm-output'

#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::gpio::PwmOutput;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: gpio/pwm-output");

    // --- PWM Configuration ---
    // MUST outlive the PwmOutput object: PwmOutput stores Config by const reference.
    PwmOutput::Config const my_pwm_config{
        .frequency_hz = 10'000,
        .resolution_bits = 10,
    };

    auto pwm_pool = init.arena.create_pool<PwmOutput>(1).unwrap();

    auto const result = PwmOutput::create(pwm_pool, my_pwm_config, gpio::G13);
    if (result.is_error()) {
        init.logger.error("PWM init failed: {}", result.error());
        return;
    }
    auto &my_pwm_output = result.ok();
    kf_defer(
        my_pwm_output.destroy(pwm_pool);
        init.logger.info("PWM output deinitialized"););

    PwmOutput::Duty const max_duty = my_pwm_config.max_duty();

    auto current_duty = 0;

    for (int i = 0; i < 2048; i += 1) {
        my_pwm_output.write(current_duty);
        init.logger.info("duty: {}", current_duty);

        current_duty += 1;
        if (current_duty > max_duty) {
            current_duty = 0;
        }

        rtos::Task::sleep(1);
    }
}
