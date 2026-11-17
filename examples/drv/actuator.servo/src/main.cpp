// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/actuator.servo'

// Sweeps a standard RC servo (SG90, MG90S, MG995, MG996R, DS3218, ...)
// within a safe 0..90° range and returns to 0°.

#include <kf/drv/Servo.hpp>
#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::drv::Servo;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/actuator.servo");

    // --- Common config (MUST outlive the Servo instance) ---
    // Standard RC servo timing: 50 Hz PWM, 500–2500 us pulse for 0..180°.
    Servo::Traits::Config::Common const servo_common_config{
        .pwm{
            .frequency_hz = 50,
            .resolution_bits = 12,
        },
        .angle_range{.start = 0, .end = 180},
        .pulse_range{.start = 500, .end = 2500},
    };

    // --- pools ---
    auto servo_pool = init.arena.create_pool<Servo>(1).unwrap();
    auto pwm_pool = init.arena.create_pool<gpio::PwmOutput>(1).unwrap();

    // --- Create servo, overriding the safe range to 0..90° ---
    auto const servo_result = Servo::create(
        servo_pool,
        pwm_pool,
        Servo::Dependencies{
            .config{
                .common = servo_common_config,
                .specific{
                    .gpio_num = gpio::G13,
                    .angle_safe_range{Servo::Traits::Config::Common::AngleRange::create(0, 90)},
                },
            },
        });

    if (servo_result.is_error()) {
        init.logger.error("Servo init failed: {}", servo_result.error());
        return;
    }

    auto &servo = servo_result.ok();
    kf_defer(
        servo.destroy(servo_pool, pwm_pool);
        init.logger.info("Servo deinitialized"););

    init.logger.info("Servo initialized (safe range 0..90°)");

    // Let the hardware settle.
    rtos::Task::sleep(500);

    // --- Sweep 0 -> 90 ---
    init.logger.info("Sweeping 0 -> 90");
    for (auto angle = 0; angle <= 90; angle += 1) {
        servo.set(angle);
        rtos::Task::sleep(20);
    }

    rtos::Task::sleep(500);

    // --- Sweep 90 -> 0 ---
    init.logger.info("Sweeping 90 -> 0");
    for (auto angle = 90; angle > 0; angle -= 1) {
        servo.set(angle);
        rtos::Task::sleep(20);
    }

    // --- Disable output (no holding torque) ---
    servo.stop();
    init.logger.info("Servo stopped");
}