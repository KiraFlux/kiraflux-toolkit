// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Match.hpp

#pragma once

namespace kf::mixin {

struct MatchTag {};

/// @brief Adds `match(T) -> bool` to check if the object matches a given value.
/// @tparam Impl Match implementation class with `bool match_impl(T value) noexcept` method
/// @tparam T   The type of the matched value.
template<typename Impl, typename T> struct Match : MatchTag {

    using MatchingType = T;

    /// @brief Compare the object against the provided value.
    /// @param value The value to match against.
    /// @return `true` if the object matches, `false` otherwise.
    [[nodiscard]] bool match(MatchingType value) const noexcept {
        return static_cast<Impl const *>(this)->match_impl(value);
    }
};

}// namespace kf::mixin

#define KF_IMPL_MATCH(...) friend struct ::kf::mixin::Match<__VA_ARGS__>
