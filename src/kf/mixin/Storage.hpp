// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Storage.hpp
/// @brief   CRTP contract for object storages (0..N slots of T).

#pragma once

#include <type_traits>
#include <utility>

#include "kf/core.hpp"

namespace kf {

template<typename> struct Option;// forward

namespace mixin {

template<typename T> struct StorageOf {};

/// @brief Unified storage contract for T.
/// @note Impl provides private `alloc_impl` / `free_impl` / `contains_impl`
template<typename Impl, typename T> struct Storage : StorageOf<T> {

    /// @brief Construct T in a free slot; `none` if no slot available.
    /// @note Caller must `free()` on construction failure — no rollback here.
    template<typename... Args> [[nodiscard]] auto alloc(Args &&...args) noexcept -> Option<T &> {
        return static_cast<Impl *>(this)->alloc_impl(std::forward<Args>(args)...);
    }

    /// @brief Destroy T and release its slot.
    void free(T &obj) noexcept {
        static_cast<Impl *>(this)->free_impl(obj);
    }

    /// @brief Check whether `ptr` points to a live slot owned by this storage.
    [[nodiscard]] bool contains(std::remove_reference_t<T> const *ptr) const noexcept {
        return static_cast<Impl const *>(this)->contains_impl(ptr);
    }
};

}// namespace mixin

}// namespace kf

#define KF_IMPL_STORAGE(...) friend struct ::kf::mixin::Storage<__VA_ARGS__>
