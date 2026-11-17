// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/display.ssd1306'

#include <kf/drv/I2C.hpp>
#include <kf/drv/SSD1306.hpp>
#include <kf/gfx/Canvas.hpp>
#include <kf/gfx/fonts/gyver_5x7.hpp>
#include <kf/image/DynamicImage.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

using kf::drv::I2C;
using kf::drv::SSD1306;

using Orientation = SSD1306::Traits::Orientation;

void run_demo(kf::Init &init, SSD1306 &display, char const *orientation_name) {
    using kf::gfx::Canvas;
    using kf::gfx::fonts::gyver_5x7_en;
    using kf::image::DynamicImage;
    using kf::rtos::Task;

    using P = SSD1306::Traits::ImageImpl::PixelImpl;

    constexpr auto on = true;
    constexpr auto off = false;

    Canvas<P> canvas{DynamicImage<P>{display.image()}};
    canvas.font(gyver_5x7_en);

    auto const step_time_ms = 500;

    // Fill screen
    canvas.background(on);
    canvas.fill();
    (void) display.send();
    Task::sleep(step_time_ms);

    // Clear screen
    canvas.background(off);
    canvas.fill();
    (void) display.send();
    Task::sleep(step_time_ms);

    // Diagonal lines
    canvas.background(off);
    canvas.fill();
    canvas.foreground(on);
    for (int i = 0; i < canvas.width(); i += 10) {
        canvas.line(0, 0, i, canvas.max_y());
        canvas.line(canvas.max_x(), 0, canvas.max_x() - i, canvas.max_y());
    }
    (void) display.send();
    Task::sleep(2 * step_time_ms);

    // Orientation label
    canvas.background(off);
    canvas.fill();
    canvas.foreground(on);
    canvas.text(10, 10, orientation_name);
    (void) display.send();
    Task::sleep(2 * step_time_ms);

    init.logger.info("Demo for {} completed", orientation_name);
}

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/display.ssd1306");

    // --- I2C bus ---

    I2C::Traits::Config::Common const bus_common_config{
        .clock_hz = some<usize>(100'000),
        .buffer_size = none,
        .timeout_ms = none,
    };

    auto i2c_pool = init.arena.create_pool<I2C>(1).unwrap();

    auto const bus_result = I2C::create(
        i2c_pool,
        I2C::Dependencies{
            .config{
                .common = bus_common_config,
                .specific{
                    .bus_num = 0,
                    .gpio_num_sda = none,
                    .gpio_num_scl = none,
                },
            },
        });

    if (bus_result.is_error()) {
        init.logger.error("I2C bus init failed: {}", bus_result.error());
        return;
    }
    auto &i2c_bus = bus_result.ok();
    kf_defer(
        i2c_bus.destroy(i2c_pool);
        init.logger.info("I2C bus deinitialized"););

    init.logger.info("I2C bus initialized");

    // --- Display ---

    SSD1306::Traits::Config::Common const display_common_config{
        .chunk_size = SSD1306::Traits::Config::Common::default_chunk_size,
    };

    auto display_pool = init.arena.create_pool<SSD1306>(1).unwrap();

    auto const display_result = SSD1306::create(
        display_pool,
        SSD1306::Dependencies{
            .config{
                .common = display_common_config,
                .specific{
                    .i2c_node = {
                        .address = SSD1306::Traits::Config::Specific::default_address,
                    },
                },
            },
            .extra{i2c_bus},
        });

    if (display_result.is_error()) {
        init.logger.error("Display init failed: {}", display_result.error());
        return;
    }
    auto &display = display_result.ok();
    kf_defer(
        display.destroy(display_pool);
        init.logger.info("Display deinitialized"););

    init.logger.info("Display initialized");

    Orientation orientations[] = {
        Orientation::Normal,
        Orientation::MirrorX,
        Orientation::MirrorY,
    };
    char const *orientation_names[] = {"Normal", "Mirror X", "Mirror Y"};

    for (usize i = 0; i < 3; i += 1) {
        if (auto r = display.orientation(orientations[i]); r.is_ok()) {
            run_demo(init, display, orientation_names[i]);
        } else {
            init.logger.error("Orientation '{}' set error: {}", orientation_names[i], r.error());
        }
    }
}