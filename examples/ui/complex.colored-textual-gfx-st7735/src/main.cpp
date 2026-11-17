// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// Demo: colored textual UI rendered onto a ST7735 TFT over SPI.
//
// Controls (type in serial monitor):
//   w / s  - move cursor up / down
//   a / d  - decrease / increase value of selected widget
//   space  - click (activate) selected widget

#include <kf/Logger.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Clock.hpp>
#include <kf/rtos/Task.hpp>

#include <kf/Array.hpp>

#include <kf/drv/SPI.hpp>
#include <kf/drv/ST7735.hpp>
#include <kf/gpio.hpp>

#include <kf/gfx/Canvas.hpp>
#include <kf/gfx/fonts/gyver_5x7.hpp>
#include <kf/image/DynamicImage.hpp>

#include <kf/ui/Event.hpp>
#include <kf/ui/Style.hpp>
#include <kf/ui/UI.hpp>
#include <kf/ui/render/ColoredTextRenderer.hpp>
#include <kf/ui/widget/Widget.hpp>

using kf::drv::SPI;
using kf::drv::ST7735;

using MyDriver = ST7735;
using Orientation = MyDriver::Traits::Orientation;
using Pixel = MyDriver::Traits::ImageImpl::PixelImpl;

using MyUI = kf::ui::UI<
    kf::ui::UiTraits<
        kf::ui::widget::Widget<
            kf::ui::render::ColoredTextRenderer,
            kf::ui::Event<4>>>>;

using Event = MyUI::Traits::EventImpl;
using Render = MyUI::Traits::RendererImpl;
using Color = kf::ui::Color;
using Style = kf::ui::Style;

// --- Main page ---

struct MainPage : MyUI::Page {
    inline static kf::Logger logger{"main"};

    // Click counter
    int clicks{0};
    MyUI::Button click_button{"Press"};
    MyUI::Display<int> click_display{clicks};

    // Color pickers that recolor the button above
    using ColorCombo = MyUI::ComboBox<Color>;
    kf::Array<ColorCombo::Config::Item, 4> color_items{
        .items = {
            {"Normal", Color::Normal},
            {"Primary", Color::Primary},
            {"Success", Color::Success},
            {"Error", Color::Error},
        },
    };
    ColorCombo::Config color_config{.items = color_items.slice()};
    ColorCombo fg_combo{color_config};
    ColorCombo bg_combo{color_config};

    // Slider that can be disabled via checkbox
    using Slider = MyUI::Slider<int>;
    Slider::Config slider_config{
        .value_range{.start = 0, .end = 100},
        .default_value = 50,
        .step = 10,
        .init_show_value = true,
    };
    Slider slider{slider_config};
    MyUI::CheckBox slider_enabled{true};

    // Collapsible extra widgets
    static constexpr kf::usize extra_count = 9;
    kf::Array<MyUI::Display<int>, extra_count> extras{};

    static constexpr kf::usize base_widgets = 8;
    kf::Array<MyUI::Widget *, (base_widgets + extra_count)> widgets{
        .items = {
            nullptr,        // [0] navigation link, filled from kf::main
            &fg_combo,      // [1]
            &bg_combo,      // [2]
            &click_button,  // [3]
            &click_display, // [4]
            &slider_enabled,// [5]
            &slider,        // [6]
            &fold,          // [7]
        },
    };

    MyUI::Fold fold{{}, "More widgets"};

    explicit MainPage(MyUI &ui) : Page{ui} {
        this->label("Main");

        for (kf::usize i = 0; i < extra_count; i += 1) {
            auto &d = extras[i];
            d.value(static_cast<int>((i + 1) * 1111));
            d.background((i % 2 == 0) ? Color::Secondary : Color::Primary);
            d.offset(i);
            widgets[base_widgets + i] = &d;
        }

        click_button.callback([this] {
            clicks += 1;
            click_display.value(clicks);
            logger.info("clicked: {}", clicks);
            _ui.request_render();
        });

        fg_combo.callback([this](ColorCombo::Config::Item item) {
            click_button.foreground(item.value());
            logger.info("button fg = {}", item.label());
            _ui.request_render();
        });

        bg_combo.callback([this](ColorCombo::Config::Item item) {
            click_button.background(item.value());
            logger.info("button bg = {}", item.label());
            _ui.request_render();
        });

        slider_enabled.callback([this](bool state) {
            if (state) {
                slider.enable();
            } else {
                slider.disable();
            }
            logger.info("slider enabled: {}", state);
            _ui.request_render();
        });

        slider.callback([](int v) {
            logger.info("slider: {}", v);
        });
    }

    WidgetsView build() noexcept override {
        return widgets.slice().first(
            fold.expanded() ? widgets.length() : base_widgets);
    }

    void on_entry() noexcept override { logger.debug("enter main"); }
    void on_exit() noexcept override { logger.debug("exit main"); }
};

// --- Settings page ---

struct SettingsPage : MyUI::Page {
    inline static kf::Logger logger{"settings"};

    using LayoutCombo = MyUI::ComboBox<kf::ui::Layout>;
    kf::Array<LayoutCombo::Config::Item, 2> layout_items{
        .items = {
            {"Vertical", kf::ui::Layout::Vertical},
            {"Horizontal", kf::ui::Layout::Horizontal},
        },
    };
    LayoutCombo::Config layout_config{.items = layout_items.slice()};
    LayoutCombo layout_combo{layout_config};

    using SpinBox = MyUI::SpinBox<int, MyUI::Traits::GeometricAdjuster<int>>;
    SpinBox::Config spin_config{
        .default_step = 1,
        .step_adjust = 2,
    };
    SpinBox spin_box{spin_config, 10};
    MyUI::Labeled labeled_spin_box{"Step", spin_box};
    MyUI::Display<int> spin_display{10};

    // Debug fold
    static constexpr kf::usize debug_count = 3;
    kf::Array<MyUI::Display<int>, debug_count> debug_displays{};

    static constexpr kf::usize base_widgets = 5;
    kf::Array<MyUI::Widget *, (base_widgets + debug_count)> widgets{
        .items = {
            nullptr,          // [0] navigation link
            &layout_combo,    // [1]
            &labeled_spin_box,// [2]
            &spin_display,    // [3]
            &fold,            // [4]
        },
    };

    MyUI::Fold fold{{}, "Debug info"};

    explicit SettingsPage(MyUI &ui) : Page{ui} {
        this->label("Settings");

        for (kf::usize i = 0; i < debug_count; i += 1) {
            auto &d = debug_displays[i];
            d.value(static_cast<int>(200 + i));
            d.background(Color::Info);
            widgets[base_widgets + i] = &d;
        }

        layout_combo.callback([this](LayoutCombo::Config::Item item) {
            this->layout(item.value());
            logger.info("layout: {}", item.label());
            request_build();
        });

        spin_box.callback([this](int v) {
            spin_display.value(v);
            logger.info("spin: {}", v);
            _ui.request_render();
        });
    }

    WidgetsView build() noexcept override {
        return widgets.slice().first(
            fold.expanded() ? widgets.length() : base_widgets);
    }

    void on_entry() noexcept override { logger.debug("enter settings"); }
    void on_exit() noexcept override { logger.debug("exit settings"); }
};

// --- Input mapping ---

Event event_from_char(char c) {
    switch (c) {
        case 'w': return Event::page_cursor_move(-1);
        case 's': return Event::page_cursor_move(+1);
        case 'a': return Event::widget_value(-1);
        case 'd': return Event::widget_value(+1);
        case ' ':
        default: return Event::widget_click();
    }
}

// --- Entry point ---

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: ui/complex.colored-textual-gfx-st7735");

    // --- SPI bus ---

    SPI::Traits::Config const spi_config{
        .gpio_num_mosi = none,
        .gpio_num_miso = none,
        .gpio_num_sck = none,
    };

    auto spi_pool = init.arena.create_pool<SPI>(1).unwrap();

    auto const spi_r = SPI::create(spi_pool, SPI::Dependencies{.config = spi_config});
    if (spi_r.is_error()) {
        init.logger.error("SPI: {}", spi_r.error());
        return;
    }
    auto &spi_bus = spi_r.ok();
    kf_defer(spi_bus.destroy(spi_pool););
    init.logger.info("SPI ready");

    // --- Display ---

    MyDriver::Traits::Config const driver_config{
        .spi_node = {
            .clock_hz = 27'000'000,
            .gpio_num_cs = gpio::G5,
        },
        .init_orientation = Orientation::ClockWise,
        .gpio_num_data_command = gpio::G16,
        .gpio_num_reset = gpio::G17,
    };

    auto gpio_pool = init.arena.create_pool<gpio::DigitalOutput>(2).unwrap();
    auto display_pool = init.arena.create_pool<MyDriver>(1).unwrap();

    auto const disp_r = MyDriver::create(
        display_pool,
        gpio_pool,
        MyDriver::Dependencies{
            .config = driver_config,
            .extra{spi_bus},
        });
    if (disp_r.is_error()) {
        init.logger.error("Display: {}", disp_r.error());
        return;
    }
    auto &display = disp_r.ok();
    kf_defer(display.destroy(display_pool, gpio_pool););
    init.logger.info("Display ready");

    // --- Canvas over the display image ---

    using Canvas = kf::gfx::Canvas<Pixel>;
    Canvas canvas{
        kf::image::DynamicImage<Pixel>{display.image()},
        Canvas::State{
            .active_font = kf::some_ref(kf::gfx::fonts::gyver_5x7_en),
            .foreground_color = Canvas::PaletteType::white,
            .background_color = Canvas::PaletteType::black,
            .auto_next_line = true,
        },
    };

    // --- UI ---

    static Render::Config renderer_config{};
    renderer_config.textual.float_places = 3;
    renderer_config.textual.double_places = 6;
    renderer_config.textual.rows_total = canvas.height_in_glyphs();
    renderer_config.textual.row_max_length = canvas.width_in_glyphs();

    Render renderer{renderer_config, init.arena.allocate<char>(canvas.height_in_glyphs() * canvas.width_in_glyphs() + 1)};
    MyUI ui{init.arena.allocate<Event>(64), renderer};

    MainPage main_page{ui};
    SettingsPage settings_page{ui};

    renderer.callback([&](kf::StringView text) {
        canvas.fill();
        canvas.text(0, 0, text);
        (void) display.send();
    });

    // --- Navigation links ---

    main_page.widgets[0] = &settings_page.link();
    settings_page.widgets[0] = &main_page.link();

    ui.active_page(main_page);
    ui.request_render();

    // --- Event loop ---

    while (true) {
        while (init.io.available_for_read() > 0) {
            if (auto const rd = init.io.read_byte(); rd.is_ok()) {
                ui.add_event(event_from_char(static_cast<char>(rd.ok())));
            }
        }

        ui.poll(rtos::Clock::now());
        rtos::Task::sleep(10);
    }
}
