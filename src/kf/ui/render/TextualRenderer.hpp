// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    ui/render/TextualRenderer.hpp
/// @brief   Plain‑text UI renderer for terminals/consoles.

#pragma once

#include "kf/Function.hpp"
#include "kf/Slice.hpp"
#include "kf/String.hpp"
#include "kf/StringView.hpp"
#include "kf/core.hpp"
#include "kf/math.hpp"

#include "kf/mixin/Callbacked.hpp"
#include "kf/mixin/Configured.hpp"
#include "kf/mixin/DefaultResettable.hpp"

#include "kf/ui/Block.hpp"
#include "kf/ui/Color.hpp"
#include "kf/ui/Layout.hpp"
#include "kf/ui/render/Renderer.hpp"

namespace kf::internal {

using Glyph = u8;///< Text interface measurement unit in glyphs

struct TextualRendererConfig : mixin::DefaultResettable<TextualRendererConfig> {
    Glyph row_max_length{16}; ///< Maximum characters per row
    Glyph rows_total{4};      ///< Total available rows in display
    Glyph float_places{2};    ///< Decimal places for float
    Glyph double_places{4};   ///< Decimal places for double
    Glyph offset_size{2};     ///< Spaces per one offset value
    bool title_centered{true};///< Render Title centered
};

/// @brief Cursor state for tracking rendering position
struct PlainTextRendererCursor {
    Glyph row{0}, col{0};///< Current position

    /// @brief Reset cursor to beginning
    void reset() { *this = {}; }

    /// @brief Move to next line
    void newline() {
        row += 1;
        col = 0;
    }

    /// @brief Check if we can write more characters in current row
    /// @param row_max_length Maximum columns per row
    [[nodiscard]] bool can_write(Glyph row_max_length) const noexcept {
        return col < row_max_length;
    }

    /// @brief Advance cursor position by N characters
    void advance(Glyph count, Glyph row_max_length) noexcept {
        col += count;
        if (col >= row_max_length) {
            newline();
        }
    }
};

}// namespace kf::internal

namespace kf::ui::render {

/// @brief Text-based UI rendering system for terminal/console output
/// @tparam N Text buffer capacity in characters
/// @note Implements Renderer CRTP interface for character-based display
struct TextualRenderer :

    Renderer<TextualRenderer>,
    mixin::Callbacked<void(StringView)>,
    mixin::Configured<internal::TextualRendererConfig>

{
    /// @brief Text renderer configuration
    using Config = internal::TextualRendererConfig;

    explicit constexpr TextualRenderer(Config const &config, Slice<char> source) noexcept :
        mixin::Configured<Config>::Configured{config}, _buffer{source} {}

    /// @brief Helper to write character with cursor tracking
    void write_char(char ch) noexcept {
        if (_buffer.full() or _cursor.row >= this->config().rows_total) { return; }

        if (ch == '\n') {
            _cursor.newline();
            (void) _buffer.append(ch);
            return;
        }

        if (_cursor.can_write(this->config().row_max_length)) {
            _cursor.advance(1, this->config().row_max_length);
            (void) _buffer.append(ch);
            return;
        }
    }

    /// @brief Write string with cursor tracking
    void write_string(StringView str) noexcept {
        for (char ch: str) {
            write_char(ch);
        }
    }

    void write_real(f64 real, u8 rounding) noexcept {
        char buffer[24];

        String temp{{buffer}};
        (void) temp.append(real, rounding);

        write_string(temp.view());
    }

    void fill_space(usize spaces) noexcept {
        for (usize i = 0; i < spaces; i += 1) {
            write_char(' ');
        }
    }

private:
    String _buffer;// TODO: implement TUI string with cursor logic
    internal::PlainTextRendererCursor _cursor{};

    Layout _layout{Layout::Vertical};

    KF_IMPL_RENDERER(TextualRenderer);

    void begin_frame_impl() noexcept {
        _buffer.reset();
        _cursor.reset();
    }

    void end_frame_impl() noexcept {
        this->invoke(_buffer.view());
    }

    usize begin_page_impl(StringView title, Layout layout) noexcept {
        _layout = layout;

        if (this->config().title_centered) {
            fill_space(math::max(0, (int(this->config().row_max_length) - int(title.length())) / 2));
        }
        write_string(title);
        write_char('\n');

        return this->config().rows_total - _cursor.row;
    }

    void end_page_impl() noexcept {}

    void begin_widget_impl(usize, bool is_focused, usize offset) noexcept {
        write_char(is_focused ? '*' : ' ');
        write_char(' ');

        if (_layout == Layout::Vertical) {
            fill_space(this->config().offset_size * offset);
        }
    }

    void end_widget_impl() noexcept {
        write_char((_layout == Layout::Horizontal) ? ' ' : '\n');
    }

    void begin_block_impl(Block block_type) noexcept {
        write_char((block_type == Block::Standard) ? '[' : '<');
    }

    void end_block_impl(Block block_type) noexcept {
        write_char((block_type == Block::Standard) ? ']' : '>');
    }

    void decoration_impl(Decoration decoration) noexcept {
        switch (decoration) {
            case Decoration::Space:
                write_char(' ');
                return;

            case Decoration::Colon:
                write_string(": ");
                return;

            case Decoration::LongArrowRight:
                write_string("->");
                return;

            case Decoration::LongArrowLeft:
                write_string("<-");
                return;

            case Decoration::ShortArrowLeft:
                write_char('>');
                return;

            case Decoration::ShortArrowRight:
                write_char('<');
                return;

            case Decoration::ShortArrowUp:
                write_char('A');
                return;

            case Decoration::ShortArrowDown:
                write_char('V');
                return;

            default:
                break;
        }
    }

    void checkbox_impl(bool enabled) noexcept {
        constexpr StringView on{"==[ 1 ]"}, off{"[ 0 ]--"};
        write_string(enabled ? on : off);
    }

    void slider_impl(f32 fill) noexcept {
        usize const start_col = _cursor.col;
        usize const inner_width = this->config().row_max_length - start_col - 1;// -1 for closing char
        usize const fill_chars = fill * inner_width;

        write_char('[');

        for (auto i = 0; i < fill_chars; i += 1) {
            write_char('=');
        }

        write_char('@');

        for (auto i = _cursor.col - start_col; i < inner_width; i += 1) {
            write_char('-');
        }

        write_char(']');
    }

    // Value rendering implementations

    void value_impl(NoneType) noexcept {
        write_string("none");
    }

    void value_impl(auto value) noexcept {

        using T = decltype(value);

        if constexpr (std::is_same_v<T, char>) {
            write_char(value);
        } else if constexpr (std::is_same_v<bool, T>) {
            constexpr StringView label_true{"true"}, label_false{"false"};
            write_string(value ? label_true : label_false);
        } else if constexpr (std::is_same_v<f64, T>) {
            write_real(value, this->config().double_places);
        } else if constexpr (std::is_same_v<f32, T>) {
            write_real(value, this->config().float_places);
        } else if constexpr (std::is_integral_v<T>) {

            char buffer[21];

            String temp{{buffer}};
            (void) temp.append(static_cast<i64>(value));

            write_string(temp.view());
        }
    }

    void value_impl(StringView str) noexcept {
        write_string(str);
    }

    // Semantic color (No-Op in plain text)

    void set_foreground_impl(Color) noexcept {}

    void set_background_impl(Color) noexcept {}

    // Properties

    constexpr Layout get_layout_impl() const noexcept {
        return _layout;
    }
};

}// namespace kf::ui::render