// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/bus.spi-loopback'

// Demonstrates the SPI bus + node API with a MOSI->MISO loopback.
// Connect MOSI (default GPIO 23) to MISO (default GPIO 19) to see real data.

#include <kf/drv/SPI.hpp>
#include <kf/gpio.hpp>
#include <kf/main.hpp>

using kf::drv::SPI;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/bus.spi-loopback");

    auto spi_pool = init.arena.create_pool<SPI>(1).unwrap();

    auto const spi_result = SPI::create(
        spi_pool,
        SPI::Dependencies{
            .config{
                // none = default VSPI pins: SCK=18, MISO=19, MOSI=23
                .gpio_num_mosi = none,
                .gpio_num_miso = none,
                .gpio_num_sck = none,
            },
        });

    if (spi_result.is_error()) {
        init.logger.error("SPI init failed: {}", spi_result.error());
        return;
    }
    auto &spi = spi_result.ok();
    kf_defer(
        spi.destroy(spi_pool);
        init.logger.info("SPI deinitialized"););

    init.logger.info("SPI bus initialized");

    // Node config MUST outlive the node (node stores it by reference).
    SPI::Traits::Node::Config const node_config{
        .clock_hz = 1'000'000,
        .gpio_num_cs = gpio::G5,
    };

    auto node = spi.create_node(node_config);

    // --- Write ---
    (void) node.write_byte(0xA5);

    u8 const tx[]{0x01, 0x02, 0x03, 0x04};
    (void) node.write_buffer({tx});

    // --- Read back (loopback) ---
    u8 rx[sizeof(tx)]{};
    if (auto const rd = node.read_buffer({rx}); rd.is_ok()) {
        init.logger.info("read {} bytes: {}", rd.ok().length(), rd.ok());
    } else {
        init.logger.error("read failed: {}", rd.error());
    }
}
