// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/ReprTo.hpp
/// @brief   Adds repr_to(char_writable) method for string representation.

#pragma once

#include "kf/core.hpp"

namespace kf::mixin {

template<typename> struct WritableTag;// forward declaration

struct ReprToTag {};

/// @brief CRTP mixin that adds a `usize repr_to(char_writable)` method
/// @tparam Impl Implementation class (must implement `usize repr_to_impl(kf::implements<kf::mixin::WritableTag<char>> auto &char_writable) const noexcept`)
template<typename Impl> struct ReprTo : ReprToTag {

    /// @brief Represent object to the writable
    /// @returns numbers of chars written
    constexpr usize repr_to(implements<mixin::WritableTag<char>> auto &char_writable) const noexcept {
        return static_cast<Impl const *>(this)->repr_to_impl(char_writable);
    }
};

}// namespace kf::mixin

#define KF_IMPL_REPR_TO(...) friend struct ::kf::mixin::ReprTo<__VA_ARGS__>
