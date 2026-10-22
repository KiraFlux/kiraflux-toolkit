// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    ui/UI.hpp
/// @brief   Main UI framework: manages pages, widgets, events, and rendering.

#pragma once

#include "kf/Option.hpp"
#include "kf/Queue.hpp"
#include "kf/StringView.hpp"
#include "kf/core.hpp"
#include "kf/math.hpp"
#include "kf/units.hpp"

#include "kf/mixin/Labeled.hpp"
#include "kf/mixin/NonCopyable.hpp"
#include "kf/mixin/Poll.hpp"

#include "kf/ui/Decoration.hpp"
#include "kf/ui/Layout.hpp"
#include "kf/ui/Request.hpp"
#include "kf/ui/Style.hpp"
#include "kf/ui/UiTraits.hpp"
#include "kf/ui/render/Renderer.hpp"
#include "kf/ui/widget/Button.hpp"
#include "kf/ui/widget/CheckBox.hpp"
#include "kf/ui/widget/ComboBox.hpp"
#include "kf/ui/widget/Display.hpp"
#include "kf/ui/widget/Labeled.hpp"
#include "kf/ui/widget/Slider.hpp"
#include "kf/ui/widget/SpinBox.hpp"

namespace kf::ui {

/// @brief User interface framework with widget-based rendering
/// @tparam U UI Traits implementation
template<implements<UiTraitsTag> U> struct UI :

    mixin::NonCopyable,
    mixin::Poll<UI<U>>

{
    using Self = UI<U>;

    /// @brief UI Traits implementation
    struct Traits : U {};

    using Widget = typename U::Widget;

    /// @brief Button widget with click handler
    struct Button : widget::Button<Traits> {
        using widget::Button<Traits>::Button;
    };

    /// @brief Checkbox with boolean state and change handler
    struct CheckBox : widget::CheckBox<Traits> {
        using widget::CheckBox<Traits>::CheckBox;
    };

    /// @brief Combo box for selecting from a fixed set of options
    /// @tparam T Value type of options
    template<typename T> struct ComboBox : widget::ComboBox<Traits, T> {
        using widget::ComboBox<Traits, T>::ComboBox;
    };

    /// @brief Read-only display of a value (by reference)
    /// @tparam T Type of displayed value (must outlive the widget)
    template<typename T> struct Display : widget::Display<Traits, T> {
        using widget::Display<Traits, T>::Display;
    };

    /// @brief Wrapper that adds a label to another widget
    struct Labeled : widget::Labeled<Traits> {
        using widget::Labeled<Traits>::Labeled;
    };

    /// @brief Spin box for numeric adjustment with configurable step mode
    template<arithmetic T, implements<typename Traits::AdjusterTag> A> struct SpinBox : widget::SpinBox<Traits, T, A> {
        using widget::SpinBox<Traits, T, A>::SpinBox;
    };

    /// @brief Slider for numeric adjustment with constraints
    template<arithmetic T> struct Slider : widget::Slider<Traits, T> {
        using widget::Slider<Traits, T>::Slider;
    };

    struct Page;// forward declaration

    explicit constexpr UI(Slice<typename Traits::EventImpl> event_buffer, typename Traits::RendererImpl &renderer) noexcept :
        _events{event_buffer}, _renderer{renderer} {}

    /// @brief Set active page
    /// @param page Page to make active (must remain valid)
    void active_page(Page &page) noexcept {
        if (_active_page.is_some()) {
            _active_page.unwrap().on_exit();
        }
        page.on_entry();
        _active_page = some_ref(page);
    }

    /// @brief Requests a UI redraw on the next poll cycle
    void request_render() noexcept {
        _renderer.request_render();
    }

    /// @brief Get readonly access to active page
    constexpr auto active_page() const noexcept -> kf::Option<Page &> {
        return _active_page;
    }

    /// @brief Add event to processing queue
    void add_event(typename Traits::EventImpl event) noexcept {
        (void) _events.write(event);// TODO: return
    }

private:
    /// @brief Special widget for creating page navigation buttons
    /// @note Internal use only - use Page::link() for page navigation
    struct PageSetter : Widget {
        explicit PageSetter(Page &target) noexcept : _target{target}, Widget{Style::defaults()} {
            this->hint("Navigate to page..");
        }

        /// @brief Set target page as active on click
        Request on_click() noexcept override {
            _target._ui.active_page(_target);

            return Request::Redraw;
        }

        void do_render(typename Traits::RendererImpl &render) const noexcept override {
            render.decoration(Decoration::LongArrowRight);
            render.decoration(Decoration::Space);
            render.value(_target.label());
        }

    private:
        Page &_target;///< Target page for navigation
    };

public:
    /// @brief UI page containing widgets and label
    struct Page :

        mixin::NonCopyable,
        mixin::Labeled,
        mixin::Poll<Page>

    {
        friend struct PageSetter;

        using WidgetsView = Slice<Widget *>;

        explicit constexpr Page(UI &ui, Layout layout = Layout::Vertical) noexcept :
            _ui{ui}, _layout{layout} {}

        /// @brief Build page content
        /// @note Not for direct call. Use `Page::request_build()`
        [[nodiscard]] virtual WidgetsView build() noexcept = 0;

        /// @brief Page behavior on entry
        virtual void on_entry() noexcept {}

        /// @brief Page behavior on leave
        virtual void on_exit() noexcept {}

    protected:
        /// @brief Page behavior on UI polling
        virtual void on_poll(units::Milliseconds now) noexcept {}

    public:
        /// @brief Request Page content build
        void request_build() noexcept {
            _build_requested = true;
        }

        /// @brief Get page layout
        [[nodiscard]] constexpr Layout layout() noexcept {
            return _layout;
        }

        /// @brief Get Mutable access to 'go to this page' Widget
        [[nodiscard]] constexpr Widget &link() noexcept {
            return _to_this;
        }

        /// @brief Get Readonly access to 'go to this page' Widget
        [[nodiscard]] constexpr Widget const &link() const noexcept {
            return _to_this;
        }

        /// @brief Get selected widget
        [[nodiscard]] constexpr auto selected_widget() const noexcept -> Option<Widget const &> {
            return _widgets.empty() ? none : some_ref<Widget const &>(*_widgets[_cursor]);
        }

        /// @brief Render page content to display.
        /// @param render Renderer instance.
        /// @note Handles cursor positioning and widget focus.
        void render(typename Traits::RendererImpl &render) noexcept {
            usize const available = render.begin_page(this->label(), _layout);

            if (_widgets.empty()) {
                // TODO: render placeholder page content
            } else {
                usize const start = (_widgets.length() > available) ? math::min(static_cast<usize>(_cursor), _widgets.length() - available) : 0;
                usize const end = math::min(start + available, _widgets.length());

                for (auto i = start; i < end; i += 1) {
                    auto widget = _widgets[i];

                    if (nullptr == widget) {
                        // TODO: render placeholder widget
                    } else {
                        widget->render(render, i, (i == _cursor));
                    }
                }
            }

            render.end_page();
        }

        /// @brief Add event to processing queue
        /// @param event The event to be processed
        Request on_event(typename Traits::EventImpl event) noexcept {
            using Kind = typename Traits::EventImpl::Kind;

            if (_widgets.empty()) {
                return Request::Nothing;
            }

            auto &active_widget = *_widgets[_cursor];

            switch (event.kind()) {
                case Kind::PageCursorMove: {
                    return move_cursor(event.value());
                }
                case Kind::WidgetClick: {
                    if (active_widget.enabled()) {
                        return active_widget.on_click();
                    }
                    break;
                }
                case Kind::WidgetValue: {
                    if (active_widget.enabled()) {
                        return active_widget.on_event_value(event.value());
                    }
                    break;
                }
            }

            return Request::Nothing;
        }

    protected:
        /// @brief Set page layout
        void layout(Layout new_layout) noexcept {
            _layout = new_layout;
        }

        /// @brief Set page cursor
        void cursor(usize new_cursor) noexcept {
            _cursor = math::clamp<isize>(new_cursor, 0, _widgets.length() - 1);
        }

        UI &_ui;

    private:
        PageSetter _to_this{*this};///< Navigation widget to this page
        WidgetsView _widgets{};    ///< Widgets on this page
        isize _cursor{0};          ///< Current widget cursor position (focused widget index)
        Layout _layout;            ///< Current layout
        bool _build_requested{true};

        /// @brief Move cursor within page bounds
        /// @param delta Cursor movement delta (positive/negative)
        [[nodiscard]] Request move_cursor(isize delta) noexcept {
            auto const n = _widgets.length();
            if (n > 1) {
                _cursor = (_cursor + delta + n) % n;
                return Request::Redraw;
            } else {
                return Request::Nothing;
            }
        }

        KF_IMPL_POLL(Page);
        void poll_impl(units::Milliseconds now) noexcept {
            on_poll(now);

            if (_build_requested) {
                _build_requested = false;

                auto new_widgets = build();

                if (new_widgets.length() == _widgets.length()) {
                    for (auto i = 0u; i < _widgets.length(); i += 1) {
                        if (new_widgets[i] != _widgets[i]) {
                            _ui.request_render();
                            break;
                        }
                    }
                } else {
                    _ui.request_render();
                }

                _widgets = new_widgets;
                cursor(_cursor);
            }
        }
    };

private:
    Queue<typename Traits::EventImpl> _events;///< Event queue for pending UI events
    typename Traits::RendererImpl &_renderer; ///< Renderer system implementation
    Option<Page &> _active_page{none};        ///< Currently active page for rendering

    KF_IMPL_POLL(Self);
    void poll_impl(units::Milliseconds now) noexcept {
        if (_active_page.is_none()) { return; }

        if (not _events.empty()) {
            constexpr usize max_events_per_poll{20};
            auto events_processed = 0u;

            while (not _events.empty() and events_processed < max_events_per_poll) {
                auto const event = _events.read();

                if (event.is_none()) {
                    break;
                }

                auto const request = _active_page.unwrap().on_event(event.unwrap());

                switch (request) {
                    case Request::Nothing:
                        break;

                    case Request::Redraw:
                        _renderer.request_render();
                        break;

                    case Request::Rebuild:
                        _active_page.unwrap().request_build();
                        break;
                }

                events_processed += 1;
            }
        }

        _active_page.unwrap().poll(now);

        if (_renderer.render_requested()) {
            _renderer.begin_frame();
            _active_page.unwrap().render(_renderer);
            _renderer.end_frame();
        }
    }
};

}// namespace kf::ui