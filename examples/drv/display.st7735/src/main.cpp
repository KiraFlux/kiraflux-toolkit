// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/display.st7735'

#include <kf/drv/SPI.hpp>
#include <kf/drv/ST7735.hpp>
#include <kf/gfx/Canvas.hpp>
#include <kf/gfx/Palette.hpp>
#include <kf/gfx/fonts/gyver_5x7.hpp>
#include <kf/gpio.hpp>
#include <kf/image/DynamicImage.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::drv::SPI;
using kf::drv::ST7735;

using Orientation = ST7735::Traits::Orientation;

void run_demo(kf::Init &init, ST7735 &display, char const *orientation_name) {
    using kf::gfx::Canvas;
    using kf::gfx::Palette;
    using kf::gfx::fonts::gyver_5x7_en;
    using kf::image::DynamicImage;
    using kf::rtos::Task;

    using P = ST7735::Traits::ImageImpl::PixelImpl;

    constexpr auto red = P::from_rgb(0xFF, 0x00, 0x00);
    constexpr auto green = P::from_rgb(0x00, 0xFF, 0x00);
    constexpr auto blue = P::from_rgb(0x00, 0x00, 0xFF);
    constexpr auto black = Palette<P>::black;
    constexpr auto white = Palette<P>::white;

    Canvas<P> canvas{DynamicImage<P>{display.image()}};
    canvas.font(gyver_5x7_en);

    auto const step_time_ms = 200;

    // Fill screen with solid colors
    canvas.background(red);
    canvas.fill();
    (void) display.send();
    Task::sleep(step_time_ms);

    canvas.background(green);
    canvas.fill();
    (void) display.send();
    Task::sleep(step_time_ms);

    canvas.background(blue);
    canvas.fill();
    (void) display.send();
    Task::sleep(step_time_ms);

    // Draw diagonal lines on white background
    canvas.background(white);
    canvas.fill();
    for (int i = 0; i < canvas.width(); i += 10) {
        canvas.foreground(red);
        canvas.line(0, 0, i, canvas.max_y());
        canvas.foreground(blue);
        canvas.line(canvas.max_x(), 0, canvas.max_x() - i, canvas.max_y());
    }
    (void) display.send();
    Task::sleep(2 * step_time_ms);

    // Show orientation name on black background
    canvas.background(black);
    canvas.fill();
    canvas.foreground(white);
    canvas.text(10, 10, orientation_name);
    (void) display.send();
    Task::sleep(2 * step_time_ms);

    init.logger.info("Demo for {} completed", orientation_name);
}

// --- Main application ---

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/display.st7735");

    // --- SPI bus ---

    auto spi_pool = init.arena.create_pool<SPI>(1).unwrap();

    auto const spi_result = SPI::create(
        spi_pool,
        SPI::Dependencies{
            // Use default SPI pins (none means "use defaults")
            .config = {
                .gpio_num_mosi = none,
                .gpio_num_miso = none,
                .gpio_num_sck = none,
            },
        });

    if (spi_result.is_error()) {
        init.logger.error("SPI bus init failed: {}", spi_result.error());
        return;
    }
    auto &spi_bus = spi_result.ok();
    kf_defer(
        spi_bus.destroy(spi_pool);
        init.logger.info("SPI bus deinitialized"););

    init.logger.info("SPI bus initialized");

    // --- GPIO pool for DC/RST ---

    auto gpio_pool = init.arena.create_pool<gpio::DigitalOutput>(2).unwrap();

    // --- Display ---

    // ~40 KiB framebuffer, fits into 64 KiB arena
    auto display_pool = init.arena.create_pool<ST7735>(1).unwrap();

    auto const display_result = ST7735::create(
        display_pool,
        gpio_pool,
        ST7735::Dependencies{
            .config{
                .spi_node = {
                    .clock_hz = 27'000'000,
                    .gpio_num_cs = gpio::G5,
                },

                .init_orientation = Orientation::Normal,
                .gpio_num_data_command = gpio::G16,
                .gpio_num_reset = gpio::G17,
            },
            .extra{spi_bus},
        });

    if (display_result.is_error()) {
        init.logger.error("Display init failed: {}", display_result.error());
        return;
    }
    auto &display = display_result.ok();
    kf_defer(
        display.destroy(display_pool, gpio_pool);
        init.logger.info("Display deinitialized"););

    init.logger.info("Display initialized");

    Orientation orientations[] = {
        Orientation::Normal,
        Orientation::MirrorX,
        Orientation::MirrorY,
        Orientation::Flip,
        Orientation::ClockWise,
        Orientation::CounterClockWise,
    };

    char const *orientation_names[] = {
        "Normal",
        "Mirror X",
        "Mirror Y",
        "Flip",
        "Clock Wise",
        "Counter Clock Wise",
    };

    for (usize i = 0; i < 6; i += 1) {
        auto orient_result = display.orientation(orientations[i]);
        if (orient_result.is_ok()) {
            run_demo(init, display, orientation_names[i]);
        } else {
            init.logger.error("Orientation '{}' set error", orientation_names[i]);
        }
    }
}
