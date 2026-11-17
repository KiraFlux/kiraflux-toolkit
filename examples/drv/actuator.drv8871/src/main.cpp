// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/actuator.drv8871'

#include <kf/drv/DRV8871.hpp>
#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::drv::DRV8871;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/actuator.drv8871");

    // --- Motor driver common (shared) configuration (must outlive the motor instance) ---

    DRV8871::Traits::Config::Common driver_common_config{
        .pwm = {
            .frequency_hz = 30'000,
            .resolution_bits = 10,
        },
        .max_input = 1000,    // max absolute control value
        .duty_dead_zone = 500,// minimum PWM to start moving forward
    };

    // --- Motor resources ---

    auto pwm_gpio_pool = init.arena.create_pool<gpio::PwmOutput>(2).unwrap();
    auto motor_pool = init.arena.create_pool<DRV8871>(1).unwrap();

    // Initialize the motor driver
    auto motor_creation_result = DRV8871::create(
        motor_pool,
        pwm_gpio_pool,
        DRV8871::Dependencies{
            .config{
                .common = driver_common_config,
                .specific{
                    .gpio_num_forward = gpio::G25,
                    .gpio_num_backward = gpio::G26,
                },
            },
        });

    init.logger.info("arena: {}", init.arena.available());

    if (motor_creation_result.is_error()) {
        init.logger.error("Failed to create motor: {}", motor_creation_result.error());
        return;
    }

    auto &motor = motor_creation_result.ok();
    kf_defer(
        motor.destroy(motor_pool, pwm_gpio_pool);
        init.logger.info("Motor deinitialized"););

    // Small delay to stabilize hardware
    rtos::Task::sleep(1000);

    // Run forward at full speed (max_input = 1000)
    motor.set(+1000);
    init.logger.info("Forward (1000)");
    rtos::Task::sleep(2000);

    // Run backward at half speed (max_input = 1000, so -500 = 50%)
    motor.set(-500);
    init.logger.info("Backward (500)");
    rtos::Task::sleep(2000);

    // Stop the motor
    motor.stop();
    init.logger.info("Stopped");
}