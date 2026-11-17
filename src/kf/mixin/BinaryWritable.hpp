// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/BinaryWritable.hpp
/// @brief   Adds write_byte/write_buffer/write_packet for binary streams.

#pragma once

#include "kf/BytesView.hpp"
#include "kf/core.hpp"

namespace kf::mixin {

struct BinaryWritableTag {};

/// @brief CRTP base class for writable streams
/// @tparam Impl      Derived class
/// @tparam ErrorImpl Error type used by the implementation
/// @note Derived classes must implement:
///
///       - `ResultType write_buffer_impl(BytesView buffer) noexcept`
///         Write a contiguous buffer of bytes
///
///       - `ResultType write_packet_impl(trivial auto const &packet) noexcept`
///         Write a trivially copyable object
///
///       - `ResultType write_mixed_impl(trivial auto const &header, BytesView buffer) noexcept`
///         Write a small header followed by a buffer (e.g. command + data) in one transaction
template<typename Impl, typename ResultType> struct BinaryWritable : BinaryWritableTag {

    /// @brief Write single byte
    /// @return Result indicating success or error
    [[nodiscard]] ResultType write_byte(u8 byte) noexcept {
        return this->write_packet(byte);
    }

    /// @brief Write arbitrary data from buffer
    /// @param buffer Source data
    /// @return Result indicating success or error
    [[nodiscard]] ResultType write_buffer(BytesView buffer) noexcept {
        return static_cast<Impl *>(this)->write_buffer_impl(buffer);
    }

    /// @brief Write fixed‑size packet
    /// @param packet Value to write
    /// @return Result indicating success or error
    [[nodiscard]] ResultType write_packet(trivial auto const &packet) noexcept {
        return static_cast<Impl *>(this)->write_packet_impl(packet);
    }

    /// @brief Write mixed packet: fixed-size header and dynamic-sized buffer
    /// @param header Header to write
    /// @param buffer Source buffer
    /// @return Result indicating success or error
    [[nodiscard]] ResultType write_mixed(trivial auto const &header, BytesView buffer) noexcept {
        return static_cast<Impl *>(this)->write_mixed_impl(header, buffer);
    }
};

}// namespace kf::mixin

#define KF_IMPL_BINARY_WRITABLE(...) friend struct kf::mixin::BinaryWritable<__VA_ARGS__>
