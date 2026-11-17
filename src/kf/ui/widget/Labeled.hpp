// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    ui/widget/Labeled.hpp
/// @brief   Wrapper that adds a label to another widget.

#pragma once

#include "kf/StringView.hpp"
#include "kf/core.hpp"

#include "kf/mixin/Labeled.hpp"

#include "kf/ui/Decoration.hpp"
#include "kf/ui/Request.hpp"
#include "kf/ui/Style.hpp"
#include "kf/ui/UiTraits.hpp"

namespace kf::ui::widget {

struct LabeledTag {};

/// @brief Widget wrapper adding label to another widget
/// @tparam U UI Traits Type
template<implements<UiTraitsTag> U> struct Labeled :

    LabeledTag,
    U::Widget,
    mixin::Labeled

{
    explicit constexpr Labeled(StringView label, typename U::Widget &wrapped, Style style = Style::defaults()) noexcept :
        U::Widget{style}, mixin::Labeled{label}, _wrapped{wrapped} {}

    /// @brief Forward click event to wrapped widget
    /// @return Result from wrapped widget's on_click()
    Request on_click() noexcept override {
        return _wrapped.on_click();
    }

    /// @brief Forward change event to wrapped widget
    /// @return Result from wrapped widget's on_event_value()
    Request on_event_value(typename U::EventImpl::Value event_value) noexcept override {
        return _wrapped.on_event_value(event_value);
    }

    void do_render(typename U::RendererImpl &render) const noexcept override {
        render.value(this->label());
        render.decoration(Decoration::Colon);
        _wrapped.do_render(render);
        // restore color
        render.foreground(this->foreground());
        render.background(this->background());
    }

private:
    typename U::Widget &_wrapped;
};

}// namespace kf::ui::widget