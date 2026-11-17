// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    image/Image.hpp
/// @brief   CRTP base for image buffers with width/height/buffer access.

#pragma once

#include "kf/Slice.hpp"
#include "kf/core.hpp"
#include "kf/units.hpp"

#include "kf/pixel/Pixel.hpp"

namespace kf::image {

struct ImageTag {};

/// @brief Image
/// @tparam Impl Image implementation
/// @tparam P Pixel implementation
template<typename Impl, implements<pixel::PixelTag> P> struct Image :

    ImageTag

{

    using BufferType = typename P::BufferType;

    // Abstract properties

    /// @brief Get current width in pixels (may differ from physical width due to orientation)
    [[nodiscard]] units::Pixels width() const noexcept {
        return static_cast<Impl const *>(this)->get_width_impl();
    }

    /// @brief Get current height in pixels (may differ from physical width due to orientation)
    [[nodiscard]] units::Pixels height() const noexcept {
        return static_cast<Impl const *>(this)->get_height_impl();
    }

    /// @brief Get current full width in pixels (may differ from physical width due to orientation)
    [[nodiscard]] units::Pixels stride() const noexcept {
        return static_cast<Impl const *>(this)->get_stride_impl();
    }

    /// @brief Get writable frame buffer
    [[nodiscard]] Slice<BufferType> buffer() noexcept {
        return static_cast<Impl *>(this)->get_buffer_impl();
    }

    /// @brief Get readonly frame buffer
    [[nodiscard]] Slice<BufferType const> buffer() const noexcept {
        return const_cast<Image *>(this)->buffer();
    }

    // properties

    /// @brief Get maximum valid X coordinate
    [[nodiscard]] units::Pixels max_x() const noexcept {
        return width() - 1;
    }

    /// @brief Get maximum valid Y coordinate
    [[nodiscard]] units::Pixels max_y() const noexcept {
        return height() - 1;
    }

    /// @brief Get image size in bytes
    [[nodiscard]] usize size() const noexcept {
        return buffer().length() * sizeof(BufferType);
    }
};

}// namespace kf::image

#define KF_IMPL_IMAGE(...) friend struct ::kf::image::Image<__VA_ARGS__>
