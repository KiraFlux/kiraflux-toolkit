// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    Pool.hpp
/// @brief   Typed slot pool over caller-owned memory.

#pragma once

#include <new>    // placement new
#include <utility>// std::forward

#include "kf/Option.hpp"
#include "kf/Slice.hpp"
#include "kf/core.hpp"

#include "kf/mixin/Capacity.hpp"
#include "kf/mixin/NonCopyable.hpp"
#include "kf/mixin/Reset.hpp"
#include "kf/mixin/Storage.hpp"

namespace kf {

/// @brief Fixed-capacity pool of T over caller-provided slots.
/// @note  `_last_allocated` — search hint (first possibly-free slot).
///        `_max_allocated`  — high-water mark of ever-used slots (used by `reset`).
template<typename T> struct Pool :

    mixin::NonCopyable,
    mixin::Storage<Pool<T>, T>,
    mixin::Capacity<Pool<T>>,
    mixin::Reset<Pool<T>>

{
    using Self = Pool<T>;

    /// @param slots Uninitialized storage for T (borrowed).
    /// @param used  Occupancy flags; must have the same length as `slots`.
    constexpr Pool(Slice<T> slots, Slice<bool> used) noexcept :
        _slots{slots}, _used{used} {}

private:
    KF_IMPL_STORAGE(Self, T);

    template<typename... Args> [[nodiscard]] auto alloc_impl(Args &&...args) noexcept -> Option<T &> {
        for (usize i = _last_allocated.unwrap_or(0); i < _slots.length(); i += 1) {
            if (_used[i]) { continue; }

            _used[i] = true;
            _last_allocated = some(i + 1);

            if (_max_allocated.is_none() or i > _max_allocated.unwrap()) {
                _max_allocated = some(i);
            }

            return some_ref(*new (static_cast<void *>(&_slots[i])) T(std::forward<Args>(args)...));
        }

        return none;
    }

    void free_impl(T &obj) noexcept {
        auto const index = static_cast<usize>(&obj - _slots.data());
        obj.~T();
        _used[index] = false;

        if (_last_allocated.is_none() or index < _last_allocated.unwrap()) {
            _last_allocated = some(index);
        }
    }

    bool contains_impl(T const *ptr) const noexcept {
        if (nullptr == ptr) { return false; }

        auto const p = reinterpret_cast<usize>(ptr);
        auto const start = reinterpret_cast<usize>(_slots.data());
        auto const end = start + _slots.length() * sizeof(T);

        if (p < start or p >= end) { return false; }
        return _used[(p - start) / sizeof(T)];
    }

    KF_IMPL_CAPACITY(Self);
    constexpr usize capacity_impl() const noexcept {
        return _slots.length();
    }

    KF_IMPL_RESETTABLE(Self);
    void reset_impl() noexcept {
        if (_max_allocated.is_some()) {
            for (usize i = 0; i <= _max_allocated.unwrap(); i += 1) {
                if (_used[i]) {
                    _slots[i].~T();
                    _used[i] = false;
                }
            }
        }

        _last_allocated = none;
        _max_allocated = none;
    }

    Slice<T> _slots;
    Slice<bool> _used;
    Option<usize> _last_allocated{none}, _max_allocated{none};
};

}// namespace kf
