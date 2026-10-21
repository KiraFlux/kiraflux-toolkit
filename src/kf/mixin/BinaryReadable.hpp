// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/BinaryReadable.hpp
/// @brief   Adds read_byte/read_buffer/read_packet for binary streams.

#pragma once

#include "kf/Bytes.hpp"
#include "kf/BytesView.hpp"
#include "kf/Result.hpp"
#include "kf/core.hpp"

namespace kf::mixin {

struct BinaryReadableTag {};

/// @brief CRTP base class for readable byte streams
/// @tparam Impl      Derived class
/// @tparam ErrorImpl Error type used by the implementation
/// @note Derived classes must implement:
///
///       - `auto read_buffer_impl(Bytes buffer) noexcept -> Result<BytesView, Error>`
///         Read up to buffer.length() bytes into the given buffer. Returns a slice of actually read data
///
///       - `template<typename T> auto read_packet_impl() noexcept -> Result<T, Error>`
///         Read a trivially copyable object of type T (size fixed at compile time)
template<typename Impl, typename ErrorImpl> struct BinaryReadable : BinaryReadableTag {

    /// @brief Read single byte
    /// @return Byte value or error
    [[nodiscard]] auto read_byte() noexcept -> Result<u8, ErrorImpl> {
        return this->read_packet<u8>();
    }

    /// @brief Read arbitrary number of bytes into buffer
    /// @param buffer Destination buffer
    /// @return Slice containing actually read data, or error
    [[nodiscard]] auto read_buffer(Bytes buffer) noexcept -> Result<BytesView, ErrorImpl> {
        return static_cast<Impl *>(this)->read_buffer_impl(buffer);
    }

    /// @brief Read fixed‑size buffer
    /// @tparam T Type of packet
    /// @return Packet value or error
    template<trivial T> [[nodiscard]] auto read_packet() noexcept -> Result<T, ErrorImpl> {
        return static_cast<Impl *>(this)->template read_packet_impl<T>();
    }
};

}// namespace kf::mixin

#define KF_IMPL_BINARY_READABLE(...) friend struct kf::mixin::BinaryReadable<__VA_ARGS__>
