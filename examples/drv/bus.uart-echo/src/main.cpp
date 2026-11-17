// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/bus.uart-echo'

// Demonstrates a secondary UART: creates UART1 on GPIO 25/26 and echoes bytes back.
// UART0 is used by main for logging, so use UART1 (or UART2) here.
// For a real test: connect an external serial device, or jumper G25 to G26.

#include <kf/drv/UART.hpp>
#include <kf/gpio.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::drv::UART;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/bus.uart-echo");

    // --- Config (MUST outlive the UART) ---
    UART::Traits::Config::Common const common_config{
        .baudrate = 115'200,
        // rx_buffer_length / tx_buffer_length / inverted keep their defaults
    };

    auto uart_pool = init.arena.create_pool<UART>(1).unwrap();

    auto const uart_result = UART::create(
        uart_pool,
        UART::Dependencies{
            .config{
                .common = common_config,
                .specific{
                    .uart_num = 1,
                    .rx_gpio_num = gpio::G25,
                    .tx_gpio_num = gpio::G26,
                },
            },
        });

    if (uart_result.is_error()) {
        init.logger.error("UART init failed: {}", uart_result.error());
        return;
    }
    auto &uart = uart_result.ok();
    kf_defer(
        uart.destroy(uart_pool);
        init.logger.info("UART deinitialized"););

    init.logger.info("UART1 initialized on G25(RX)/G26(TX)");

    // --- Echo loop (bounded, ~30 s at 10 ms/iteration) ---
    init.logger.info("Entering echo loop. Send bytes to UART1...");

    for (int i = 0; i < 3000; i += 1) {
        if (uart.available_for_read() > 0) {
            if (auto const rd = uart.read_byte(); rd.is_ok()) {
                u8 const b = rd.ok();

                if (b >= 0x20 and b < 0x7F) {
                    init.logger.debug("RX: '{}'", static_cast<char>(b));
                } else {
                    init.logger.debug("RX byte: {}", static_cast<unsigned>(b));
                }

                if (auto const wr = uart.write_byte(b); wr.is_error()) {
                    init.logger.error("write_byte failed: {}", wr.error());
                }
            }
        }

        rtos::Task::sleep(10);
    }
}