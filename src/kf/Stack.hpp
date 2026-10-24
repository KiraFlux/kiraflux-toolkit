// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    Stack.hpp

#pragma once

#include <new>    // placement new
#include <utility>// move, forward

#include "kf/Option.hpp"
#include "kf/Slice.hpp"
#include "kf/core.hpp"

#include "kf/mixin/Read.hpp"
#include "kf/mixin/ReadAvailable.hpp"
#include "kf/mixin/Reset.hpp"
#include "kf/mixin/Write.hpp"
#include "kf/mixin/WriteAvailable.hpp"

#include "kf/Sequence.hpp"

namespace kf {

struct StackTag {};

/// @brief   LIFO container (stack) on a fixed‑size buffer with read/write and reset capabilities.
/// @tparam T Element type
template<typename T> struct Stack :

    StackTag,
    Sequence<Stack<T>, T>,
    mixin::Read<Stack<T>, T>,
    mixin::ReadAvailable<Stack<T>>,
    mixin::Write<Stack<T>, T>,
    mixin::WriteAvailable<Stack<T>>,
    mixin::Reset<Stack<T>>

{

    using Self = Stack<T>;

    /// @brief Construct empty stack (no buffer, length 0)
    constexpr Stack() noexcept :
        _buffer{}, _length{0} {}

    /// @brief Construct stack on existing buffer
    /// @param buffer       Slice of memory to use as storage
    /// @param init_length  Initial number of elements already in buffer (default 0)
    explicit constexpr Stack(Slice<T> buffer, usize init_length = 0) noexcept :
        _buffer{buffer}, _length{init_length} {}

    /// @brief Get Stack capacity (max length)
    [[nodiscard]] constexpr usize capacity() const noexcept {
        return _buffer.length();
    }

    /// @brief Is Stack reach max length
    [[nodiscard]] constexpr bool full() const noexcept {
        return this->length() >= this->capacity();
    }

    /// @brief Get mutable access to stack top value
    /// @return optional reference to stack top item
    [[nodiscard]] constexpr auto top() noexcept -> Option<T &> {
        return this->empty() ? none : some_ref(_buffer[_length - 1]);
    }

    /// @brief Get Readonly access to stack top value
    /// @return optional constant reference to stack top item
    [[nodiscard]] constexpr auto top() const noexcept -> Option<T const &> {
        return this->empty() ? none : some_ref<T const &>(_buffer[_length - 1]);
    }

private:
    Slice<T> _buffer;
    usize _length;

    KF_IMPL_SEQUENCE(Self, T);

    constexpr T *get_data_impl() noexcept {
        return _buffer.data();
    }

    constexpr usize length_impl() const noexcept {
        return _length;
    }

    KF_IMPL_READ(Self, T);
    constexpr auto read_impl() noexcept -> Option<T> {
        if (this->empty()) {
            return none;
        }

        _length -= 1;

        auto value = std::move(_buffer[_length]);
        _buffer[_length].~T();

        return some(std::move(value));
    }

    KF_IMPL_READ_AVAILABLE(Self);
    usize available_for_read_impl() const noexcept {
        return this->length();
    }

    KF_IMPL_WRITE(Self, T);
    constexpr bool write_impl(auto &&item) noexcept {
        if (this->full()) {
            return false;
        }

        new (&_buffer[_length]) T(std::forward<decltype(item)>(item));

        _length += 1;

        return true;
    }

    KF_IMPL_WRITE_AVAILABLE(Self);
    usize available_for_write_impl() const noexcept {
        return this->capacity() - this->length();
    }

    KF_IMPL_RESETTABLE(Self);
    constexpr void reset_impl() noexcept {
        for (auto &item: *this) {
            item.~T();
        }

        _length = 0;
    }
};

}// namespace kf