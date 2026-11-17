// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/bus.i2c-scanner'

#include <kf/drv/I2C.hpp>
#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::drv::I2C;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/bus.i2c-scanner");

    // --- Common config (outlives the bus) ---

    I2C::Traits::Config::Common const bus_common_config{
        .clock_hz{100'000},
        .buffer_size{none},
        .timeout_ms{none},
    };

    // --- Bus storage ---

    auto i2c_pool = init.arena.create_pool<I2C>(1).unwrap();

    // --- Create the bus ---

    auto const bus_result = I2C::create(
        i2c_pool,
        I2C::Dependencies{
            .config{
                .common{bus_common_config},
                .specific{
                    .bus_num{0},
                    .gpio_num_sda{gpio::G21},
                    .gpio_num_scl{gpio::G22},
                },
            },
        });

    if (bus_result.is_error()) {
        init.logger.error("Bus init failed: {}", bus_result.error());
        return;
    }

    auto &i2c_bus = bus_result.ok();
    kf_defer(
        i2c_bus.destroy(i2c_pool);
        init.logger.info("I2C bus deinitialized"););

    init.logger.info("Bus initialized successfully");

    // --- Scan all I2C addresses ---

    for (u8 address = 1; address < 0x80; address += 1) {
        I2C::Traits::Node::Config const node_config{.address = address};
        auto node = i2c_bus.create_node(node_config);

        auto const result = node.write_buffer({});

        if (result.is_ok()) {
            init.logger.info("Device found at {}", address);
        } else {
            init.logger.debug("No device at {} (error: {})", address, result.error());
        }

        rtos::Task::sleep(5);
    }

    init.logger.info("Scan completed.");
}
