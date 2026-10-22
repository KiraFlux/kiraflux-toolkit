// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    Queue.hpp

#pragma once

#include <new>    // placement new
#include <utility>// move, forward

#include "kf/Option.hpp"
#include "kf/Slice.hpp"
#include "kf/core.hpp"

#include "kf/mixin/Length.hpp"
#include "kf/mixin/ReadAvailable.hpp"
#include "kf/mixin/Readable.hpp"
#include "kf/mixin/Resettable.hpp"
#include "kf/mixin/Writable.hpp"
#include "kf/mixin/WriteAvailable.hpp"

namespace kf {

struct QueueTag {};

/// @brief  FIFO queue on a fixed‑size ring buffer with front/back access and reset.
/// @tparam T Element type
template<typename T> struct Queue :

    QueueTag,
    mixin::Length<Queue<T>, usize>,
    mixin::Readable<Queue<T>, T>,
    mixin::ReadAvailable<Queue<T>>,
    mixin::Writable<Queue<T>, T>,
    mixin::WriteAvailable<Queue<T>>,
    mixin::Resettable<Queue<T>>

{

    using Self = Queue<T>;

    /// @brief Construct empty queue (no buffer, length 0)
    constexpr Queue() noexcept :
        _buffer{}, _length{0} {}

    /// @brief Construct queue on existing buffer
    /// @param buffer       Slice of memory to use as storage
    /// @param init_length  Initial number of elements already in buffer (default 0)
    explicit constexpr Queue(Slice<T> buffer, usize init_length = 0) noexcept :
        _buffer{buffer}, _length{init_length} {}

    /// @brief Get queue capacity (max number of elements)
    [[nodiscard]] constexpr usize capacity() const noexcept {
        return _buffer.length();
    }

    /// @brief Check if queue is empty
    [[nodiscard]] constexpr bool empty() const noexcept {
        return _length == 0;
    }

    /// @brief Check if queue is full
    [[nodiscard]] constexpr bool full() const noexcept {
        return _length >= this->capacity();
    }

    /// @brief Get mutable reference to the front element
    [[nodiscard]] constexpr auto front() noexcept -> Option<T &> {
        return this->empty() ? none : some_ref(_buffer[index_at(0)]);
    }

    /// @brief Get const reference to the front element
    [[nodiscard]] constexpr auto front() const noexcept -> Option<T const &> {
        return this->empty() ? none : some_ref<T const &>(_buffer[index_at(0)]);
    }

    /// @brief Get mutable reference to the back element
    [[nodiscard]] constexpr auto back() noexcept -> Option<T &> {
        return this->empty() ? none : some_ref(_buffer[index_at(_length - 1)]);
    }

    /// @brief Get const reference to the back element
    [[nodiscard]] constexpr auto back() const noexcept -> Option<T const &> {
        return this->empty() ? none : some_ref<T const &>(_buffer[index_at(_length - 1)]);
    }

private:
    Slice<T> _buffer;
    usize _head{0};
    usize _length;

    /// @brief Compute absolute index for given offset from head
    /// @param offset Distance from head (0 <= offset < count)
    /// @note Call only when this->capacity() > 0
    [[nodiscard]] constexpr usize index_at(usize offset) const noexcept {
        return (_head + offset) % this->capacity();
    }

    KF_IMPL_LENGTH(Self, usize);
    constexpr usize length_impl() const noexcept {
        return _length;
    }

    KF_IMPL_READABLE(Self, T);
    constexpr auto read_impl() noexcept -> Option<T> {
        if (this->empty()) {
            return none;
        }

        auto value = std::move(_buffer[_head]);
        _buffer[_head].~T();

        _head = index_at(1);
        _length -= 1;

        return some(std::move(value));
    }

    KF_IMPL_READ_AVAILABLE(Self);
    usize available_for_read_impl() const noexcept {
        return this->length();
    }

    KF_IMPL_WRITABLE(Self, T);
    constexpr bool write_impl(auto &&item) noexcept {
        if (this->full()) {
            return false;
        }

        new (&_buffer[index_at(_length)]) T(std::forward<decltype(item)>(item));

        _length += 1;

        return true;
    }

    KF_IMPL_WRITE_AVAILABLE(Self);
    usize available_for_write_impl() const noexcept {
        return this->capacity() - this->length();
    }

    KF_IMPL_RESETTABLE(Self);
    constexpr void reset_impl() noexcept {
        for (auto i = 0u; i < _length; i += 1) {
            _buffer[index_at(i)].~T();
        }

        _head = 0;
        _length = 0;
    }
};

}// namespace kf