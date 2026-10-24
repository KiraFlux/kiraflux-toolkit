// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    Sequence.hpp

#pragma once

#include "kf/Option.hpp"
#include "kf/core.hpp"

#include "kf/mixin/Indexable.hpp"
#include "kf/mixin/Length.hpp"
#include "kf/mixin/ReprTo.hpp"

namespace kf {

template<typename> struct Slice;// forward declaration

template<typename T> struct SequenceTag {
    struct SequenceTraits {
        using Type = T;
    };
};

/// @brief   CRTP base for sequence containers providing iteration, indexing, and slicing.
/// @tparam Impl Sequence Implementation class with `length_impl` and `get_data_impl` methods
/// @tparam T Item type
template<typename Impl, typename T> struct Sequence :

    SequenceTag<T>,
    mixin::Indexable<Sequence<Impl, T>, T>,
    mixin::ReprTo<Sequence<Impl, T>>,
    mixin::Length<Impl, usize>

{
    using Self = Sequence<Impl, T>;

    [[nodiscard]] constexpr T *data() noexcept {
        return static_cast<Impl *>(this)->get_data_impl();
    }

    [[nodiscard]] constexpr T const *data() const noexcept {
        return const_cast<Sequence *>(this)->data();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return this->length() == 0;
    }

    [[nodiscard]] constexpr T *begin() noexcept {
        return data();
    }

    [[nodiscard]] constexpr T *end() noexcept {
        return begin() + this->length();
    }

    [[nodiscard]] constexpr T const *begin() const noexcept {
        return data();
    }

    [[nodiscard]] constexpr T const *end() const noexcept {
        return begin() + this->length();
    }

    /// @brief Get mutable slice
    [[nodiscard]] constexpr Slice<T> slice() noexcept {
        return {
            this->data(),
            this->length(),
        };
    }

    /// @brief Get readonly slice
    [[nodiscard]] constexpr Slice<T const> slice() const noexcept {
        return {
            this->data(),
            this->length(),
        };
    }

    [[nodiscard]] constexpr auto first_where(callable<bool(T &)> auto &&f) noexcept -> Option<T &> {
        for (auto &item: *this) {
            if (f(item)) {
                return some_ref(item);
            }
        }
        return none;
    }

    [[nodiscard]] constexpr auto first_where(callable<bool(T const &)> auto &&f) const noexcept -> Option<T const &> {
        for (auto const &item: *this) {
            if (f(item)) {
                return some_ref(item);
            }
        }
        return none;
    }

private:
    KF_IMPL_INDEXABLE(Self, T);
    constexpr T &get_item_impl(usize index) noexcept {
        return data()[index];
    }

    KF_IMPL_REPR_TO(Self);
    constexpr usize repr_to_impl(implements<mixin::WriteTag<char>> auto &char_writable) const noexcept {
        if constexpr (type_like<T, char>) {
            return char_writable.append_null_terminated_string(this->data(), this->length());
        } else {
            usize ret = 0;
            bool f = false;
            ret += char_writable.append('{');
            for (auto const &v: *this) {
                if (f) {
                    ret += char_writable.append(',');
                    ret += char_writable.append(' ');
                }
                f = true;
                ret += char_writable.append(v);
            }
            ret += char_writable.append('}');
            return ret;
        }
    }
};

}// namespace kf

#define KF_IMPL_SEQUENCE(__impl__, ...)                  \
    friend struct ::kf::Sequence<__impl__, __VA_ARGS__>; \
    KF_IMPL_LENGTH(__impl__, ::kf::usize)
