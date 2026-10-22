// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    ui/render/ColoredTextRenderer.hpp
/// @brief   Text renderer with semantic colours (gfx::Canvas codes).

#pragma once

#include "kf/Slice.hpp"
#include "kf/mixin/Configured.hpp"
#include "kf/mixin/DefaultResettable.hpp"

#include "kf/ui/Color.hpp"
#include "kf/ui/Layout.hpp"
#include "kf/ui/render/Renderer.hpp"
#include "kf/ui/render/TextualRenderer.hpp"

namespace kf::internal {

struct ColoredTextRendererConfig : mixin::DefaultResettable<ColoredTextRendererConfig> {

    struct Palette final {

        /// @brief Semantic colors
        enum Item : u8 {
            Black = 0x00,
            DarkRed = 0x01,
            DarkGreen = 0x02,
            DarkYellow = 0x03,
            DarkBlue = 0x04,
            DarkPurple = 0x05,
            DarkCyan = 0x06,
            LightGray = 0x07,
            DarkGray = 0x08,
            LightRed = 0x09,
            LightGreen = 0x0A,
            LightYellow = 0x0B,
            LightBlue = 0x0C,
            LightPurple = 0x0D,
            LightCyan = 0x0E,
            White = 0x0F,
        };

        Item normal, primary, secondary, success, warning, error, info, disabled, highlight;

        [[nodiscard]] constexpr Item get(ui::Color color) const noexcept {
            switch (color) {
                case ui::Color::Normal: return normal;
                case ui::Color::Primary: return primary;
                case ui::Color::Secondary: return secondary;
                case ui::Color::Success: return success;
                case ui::Color::Warning: return warning;
                case ui::Color::Error: return error;// same value as ui::Color::Danger
                case ui::Color::Info: return info;
                case ui::Color::Disabled: return disabled;
                case ui::Color::Highlight: return highlight;
                default: return normal;
            }
        }
    };

    ui::render::TextualRenderer::Config textual{};

    Palette
        normal_foreground_palette{
            .normal = Palette::White,
            .primary = Palette::LightBlue,
            .secondary = Palette::LightGray,
            .success = Palette::LightGreen,
            .warning = Palette::LightYellow,
            .error = Palette::LightRed,
            .info = Palette::LightCyan,
            .disabled = Palette::DarkGray,
            .highlight = Palette::LightPurple,
        },
        focused_foreground_palette{
            .normal = Palette::Black,
            .primary = Palette::Black,
            .secondary = Palette::Black,
            .success = Palette::Black,
            .warning = Palette::Black,
            .error = Palette::Black,
            .info = Palette::Black,
            .disabled = Palette::LightGray,
            .highlight = Palette::DarkPurple,
        },
        normal_background_palette{
            .normal = Palette::Black,
            .primary = Palette::DarkBlue,
            .secondary = Palette::DarkGray,
            .success = Palette::DarkGreen,
            .warning = Palette::DarkYellow,
            .error = Palette::DarkRed,
            .info = Palette::DarkCyan,
            .disabled = Palette::DarkGray,
            .highlight = Palette::DarkPurple,
        },
        focused_background_palette{
            .normal = Palette::White,
            .primary = Palette::LightBlue,
            .secondary = Palette::LightGray,
            .success = Palette::LightGreen,
            .warning = Palette::LightYellow,
            .error = Palette::LightRed,
            .info = Palette::LightCyan,
            .disabled = Palette::LightGray,
            .highlight = Palette::LightPurple,
        };
};

};// namespace kf::internal

namespace kf::ui::render {

struct ColoredTextRenderer :

    Renderer<ColoredTextRenderer>,
    mixin::Configured<internal::ColoredTextRendererConfig>

{
    using Wrapped = TextualRenderer;
    using Config = internal::ColoredTextRendererConfig;

    explicit constexpr ColoredTextRenderer(Config const &config, Slice<char> source) noexcept :
        mixin::Configured<Config>{config}, _wrapped{config.textual, source} {}

    void callback(auto &&callback) noexcept {
        _wrapped.callback(std::forward<decltype(callback)>(callback));
    }

private:
    Wrapped _wrapped;
    bool _focus_active{false};

    void write_color(Color color, char base_code, Config::Palette const &palette) noexcept {
        _wrapped.write_char(base_code + static_cast<char>(palette.get(color)));
    }

    void write_foreground_color(Color color, Config::Palette const &palette) noexcept {
        write_color(color, '\xF0', palette);
    }

    void write_background_color(Color color, Config::Palette const &palette) noexcept {
        write_color(color, '\xB0', palette);
    }

    KF_IMPL_RENDERER(ColoredTextRenderer);

    // control

    void begin_frame_impl() noexcept {
        _wrapped.begin_frame();
    }

    void end_frame_impl() noexcept {
        _wrapped.end_frame();
    }

    usize begin_page_impl(StringView title, Layout layout) noexcept {
        write_foreground_color(Color::Normal, this->config().focused_foreground_palette);
        write_background_color(Color::Primary, this->config().focused_background_palette);
        return _wrapped.begin_page(title, layout);
    }

    void end_page_impl() noexcept {
        _wrapped.end_page();
    }

    void begin_widget_impl(usize index, bool is_focused, usize offset) noexcept {
        _focus_active = is_focused;

        if (_wrapped.layout() == Layout::Vertical) {
            _wrapped.fill_space(offset * this->config().textual.offset_size);
        }
    }

    void end_widget_impl() noexcept {
        if (not _focus_active) { _wrapped.write_char('\x80'); }
        _wrapped.end_widget();
        _focus_active = false;
    }

    void begin_block_impl(Block block_type) noexcept {
        _wrapped.begin_block(block_type);
    }

    void end_block_impl(Block block_type) noexcept {
        _wrapped.end_block(block_type);
    }

    void decoration_impl(Decoration decoration) noexcept {
        _wrapped.decoration(decoration);
    }

    void checkbox_impl(bool enabled) noexcept {
        this->background(enabled ? Color::Primary : Color::Secondary);
        _wrapped.checkbox(enabled);
    }

    void slider_impl(f32 fill) noexcept {
        _wrapped.slider(fill);
    }

    // value

    void value_impl(auto const &value) {
        _wrapped.value(value);
    }

    // semantic color

    void set_foreground_impl(Color color) noexcept {
        write_foreground_color(color, _focus_active ? this->config().focused_foreground_palette : this->config().normal_foreground_palette);
    }

    void set_background_impl(Color color) noexcept {
        write_background_color(color, _focus_active ? this->config().focused_background_palette : this->config().normal_background_palette);
    }

    // Properties

    constexpr Layout get_layout_impl() const noexcept {
        return _wrapped.layout();
    }
};

}// namespace kf::ui::render