// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/Display.hpp
/// @brief   CRTP base for display drivers (framebuffer, send, orientation).

#pragma once

#include "kf/Result.hpp"
#include "kf/core.hpp"
#include "kf/image/Image.hpp"

#include "kf/drv/Driver.hpp"

namespace kf::internal {

enum struct DisplayOrientation : u8 {
    Normal = 0,          ///< Default orientation
    MirrorX = 1,         ///< Horizontal mirror
    MirrorY = 2,         ///< Vertical mirror
    Flip = 3,            ///< 180-degree rotation
    ClockWise = 4,       ///< 90-degree clockwise rotation
    CounterClockWise = 5,///< 90-degree counterclockwise rotation
};

}

namespace kf::drv {

struct DisplayTraitsTag {};

template<implements<image::ImageTag> I, typename... Args> struct DisplayTraits : DisplayTraitsTag, DriverTraits<Args...> {
    using ImageImpl = I;

    /// @brief Display orientation modes
    using Orientation = internal::DisplayOrientation;
};

struct DisplayTag {};

/// @brief CRTP base class for display driver implementations
/// @tparam Impl Concrete driver implementation type
/// @tparam ImageImpl Image buffer type
template<typename Impl, implements<DisplayTraitsTag> D> struct Display :

    DisplayTag,
    Driver<Impl, D>

{

    using Driver<Impl, D>::Driver;

    /// @brief Mutable access at image buffer
    [[nodiscard]] constexpr auto image() noexcept -> typename D::ImageImpl & {
        return _screen_image;
    }

    /// @brief Readonly access at image buffer
    [[nodiscard]] constexpr auto image() const noexcept -> typename D::ImageImpl const & {
        return _screen_image;
    }

    /// @brief Transfer software buffer to display hardware
    [[nodiscard]] auto send() noexcept -> typename D::ActionResult {
        return static_cast<Impl *>(this)->send_impl();
    }

    /// @brief Set display orientation.
    /// @param new_orientation New orientation value.
    [[nodiscard]] auto orientation(typename D::Orientation new_orientation) noexcept -> typename D::ActionResult {
        return static_cast<Impl *>(this)->set_orientation_impl(new_orientation);
    }

private:
    typename D::ImageImpl _screen_image{};///<  Software frame buffer for display operations
};

}// namespace kf::drv

#define KF_IMPL_DISPLAY(...)                                                    \
    friend struct ::kf::drv::Display<__VA_ARGS__, typename Traits::TraitsBase>; \
    KF_IMPL_DRIVER(__VA_ARGS__)
