// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    ui/render/Renderer.hpp
/// @brief   CRTP base for UI renderers with begin/end page, widget, and value rendering.

#pragma once

#include <type_traits>

#include "kf/Option.hpp"
#include "kf/StringView.hpp"
#include "kf/core.hpp"
#include "kf/math.hpp"

#include "kf/mixin/NonCopyable.hpp"
#include "kf/mixin/Representable.hpp"

#include "kf/ui/Block.hpp"
#include "kf/ui/Color.hpp"
#include "kf/ui/Decoration.hpp"
#include "kf/ui/Layout.hpp"

namespace kf::ui::render {

struct RenderTag {};

/// @brief CRTP base class for UI rendering systems
/// @tparam Impl Concrete renderer implementation type
/// @note Delegates all rendering operations to derived class implementation
template<typename Impl> struct Renderer :

    RenderTag,
    mixin::NonCopyable

{
    /// @brief Check whether a render has been requested
    /// @return `true` if a render is pending, `false` otherwise
    [[nodiscard]] constexpr bool render_requested() noexcept {
        return _render_requested;
    }

    /// @brief Request a full UI redraw on the next poll cycle
    /// @note The actual rendering will be performed during the next call to `poll()`
    void request_render() noexcept {
        _render_requested = true;
    }

    // Control operations

    /// @brief Prepare render buffer for new frame
    void begin_frame() noexcept {
        static_cast<Impl *>(this)->begin_frame_impl();
    }

    /// @brief Finalize frame after rendering
    void end_frame() noexcept {
        static_cast<Impl *>(this)->end_frame_impl();
        _render_requested = false;
    }

    /// @brief Begin rendering page
    /// @param title Page title
    /// @param layout Page layout hint
    /// @return Number of widgets that can still be rendered in current frame
    [[nodiscard]] usize begin_page(StringView title, Layout layout) noexcept {
        return static_cast<Impl *>(this)->begin_page_impl(title, layout);
    }

    /// @brief Finish rendering page
    void end_page() noexcept {
        static_cast<Impl *>(this)->end_page_impl();
    }

    /// @brief Begin rendering specific widget
    /// @param index Widget position on page
    /// @param is_focused contrasting text region (higher visibility)
    /// @param offset Widget offset
    void begin_widget(usize index, bool is_focused, usize offset) noexcept {
        static_cast<Impl *>(this)->begin_widget_impl(index, is_focused, offset);
    }

    /// @brief Finish rendering current widget
    void end_widget() noexcept {
        static_cast<Impl *>(this)->end_widget_impl();
    }

    /// @brief Begin content block
    void begin_block(Block block_type = Block::Standard) noexcept {
        static_cast<Impl *>(this)->begin_block_impl(block_type);
    }

    /// @brief End content block
    void end_block(Block block_type = Block::Standard) noexcept {
        static_cast<Impl *>(this)->end_block_impl(block_type);
    }

    // Value rendering

    /// @brief Render a semantic decoration element
    /// @param decoration The type of decoration to render
    /// @note Decorations are lightweight markers that help users interpret the UI.
    void decoration(Decoration decoration) noexcept {
        static_cast<Impl *>(this)->decoration_impl(decoration);
    }

    /// @brief Render checkbox
    void checkbox(bool enabled) noexcept {
        static_cast<Impl *>(this)->checkbox_impl(enabled);
    }

    /// @brief Render slider
    /// @brief fill slider fill value [0..1]
    void slider(f32 fill) noexcept {
        static_cast<Impl *>(this)->slider_impl(fill);
    }

    /// @brief Render value
    void value(auto const &v) noexcept {
        using T = decltype(v);

        if constexpr (std::is_base_of_v<mixin::RepresentableTag, T>) {
            this->value(v.repr());
        } else if constexpr (std::is_base_of_v<OptionTag, T>) {

            if (v.is_some()) {
                this->value(v.unwrap());
            } else {
                static_cast<Impl *>(this)->value_impl(none);
            }

        } else {
            static_cast<Impl *>(this)->value_impl(v);
        }
    }

    // Semantic color

    /// @brief Set foreground Semantic color
    void foreground(Color color) noexcept {
        static_cast<Impl *>(this)->set_foreground_impl(color);
    }

    /// @brief Set background Semantic color
    void background(Color color) noexcept {
        static_cast<Impl *>(this)->set_background_impl(color);
    }

    // Properties

    /// @brief Get Current Layout
    [[nodiscard]] constexpr Layout layout() const noexcept {
        return static_cast<Impl const *>(this)->get_layout_impl();
    }

private:
    bool _render_requested{false};
};

}// namespace kf::ui::render

#define KF_IMPL_RENDERER(...) friend struct ::kf::ui::render::Renderer<__VA_ARGS__>