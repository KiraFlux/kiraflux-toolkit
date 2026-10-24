// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Write.hpp

#pragma once

#include <charconv>
#include <tuple>
#include <type_traits>
#include <utility>

#include "kf/StringView.hpp"
#include "kf/core.hpp"
#include "kf/math.hpp"

#include "kf/mixin/ReprTo.hpp"
#include "kf/mixin/Representable.hpp"

namespace kf::mixin {

template<typename T> struct WriteTag {
    struct WriteTraits {
        using Type = T;
    };
};

}// namespace kf::mixin

namespace kf::internal {

template<typename Impl, typename T> struct WriteBase : mixin::WriteTag<T> {

    /// @brief Write Value
    /// @param value Value to write (perfect-forwarded)
    /// @return `true` if write successful, `false` otherwise
    [[nodiscard]] constexpr bool write(auto &&value) noexcept {
        return static_cast<Impl *>(this)->write_impl(std::forward<decltype(value)>(value));
    }
};

struct FormatToken {
    u8 start, length;

    enum Kind : u8 {
        Literal,
        Anchor
    } kind;
};

struct FormatResult {
    static constexpr auto max_tokens{16u};

    FormatToken tokens[max_tokens];
    u8 anchor_indices[max_tokens];
    u8 count;

    constexpr void put_anchor(u8 position, u8 arg_index) noexcept {
        anchor_indices[count] = arg_index;
        tokens[count] = {
            .start = position,
            .length = 2,// "{}"
            .kind = FormatToken::Anchor,
        };

        count += 1;
    }

    constexpr void put_literal(u8 start, u8 length) noexcept {
        tokens[count] = {
            .start = start,
            .length = length,
            .kind = FormatToken::Literal,
        };

        count += 1;
    }
};

template<typename... Args> struct BasicFormatString {

    static constexpr auto
        anchor_begin_char{'{'},
        anchor_end_char{'}'};

    char const *str;
    usize length;
    internal::FormatResult result;

    template<usize N> consteval BasicFormatString(char const (&s)[N]) noexcept :
        str{s}, length{N - 1}, result{parse(s)} {

        if (result.count != sizeof...(Args)) {

            //TODO: compile-time termination
        }
    }

    template<usize M> static constexpr FormatResult parse(char const (&fmt)[M]) noexcept {
        constexpr auto format_literal_length{M - 1};

        FormatResult result{
            .count = 0,
        };

        auto i = 0u;
        auto arg_index = 0u;

        while (i < format_literal_length and result.count < FormatResult::max_tokens) {
            //  {{
            if (fmt[i] == anchor_begin_char and (i + 1) < format_literal_length and fmt[i + 1] == anchor_begin_char) {
                result.put_literal(i, 2);
                i += 2;
                continue;
            }

            //  }}
            if (fmt[i] == anchor_end_char and (i + 1) < format_literal_length and fmt[i + 1] == anchor_end_char) {
                result.put_literal(i, 2);
                i += 2;
                continue;
            }

            if (fmt[i] == anchor_begin_char) {
                auto start = i;
                i += 1;

                if (i < format_literal_length and fmt[i] == anchor_end_char) {
                    // {} — Anchor
                    result.put_anchor(start, arg_index);
                    arg_index += 1;
                    i += 1;
                    continue;
                }

                // Invalid anchor. Skip to next anchor_end_char
                while (i < format_literal_length and fmt[i] != anchor_end_char) {
                    i += 1;
                }

                if (i < format_literal_length) {
                    i += 1;
                }

                result.put_literal(start, i - start);
            } else {

                // Literal
                auto start = i;
                while (i < format_literal_length and fmt[i] != anchor_begin_char and fmt[i] != anchor_end_char) {
                    i += 1;
                }

                result.put_literal(start, i - start);
            }
        }
        return result;
    }
};

template<typename... Args> using FormatString = BasicFormatString<std::type_identity_t<Args>...>;

}// namespace kf::internal

namespace kf::mixin {

/// @brief Add `write(auto &&) -> bool`
/// @tparam Impl Write mixin implementation class
/// @tparam T Value type
template<typename Impl, typename T> struct Write;

template<typename Impl, typename T> struct Write : internal::WriteBase<Impl, T> {};

template<typename Impl> struct Write<Impl, char> : internal::WriteBase<Impl, char> {

    // static dispatching

    [[nodiscard]] constexpr usize append(char value) noexcept {
        return static_cast<usize>(this->write(value));
    }

    [[nodiscard]] constexpr usize append(bool value) noexcept {
        return append_null_terminated_string(value ? "true" : "false");
    }

    [[nodiscard]] constexpr usize append(c_string auto const &value) noexcept {
        return append_null_terminated_string(static_cast<char const *>(value));
    }

    [[nodiscard]] constexpr usize append(integer_type auto value) noexcept {
        return append_integer(value);
    }

    [[nodiscard]] constexpr usize append(float_type auto value, usize precision = 3) noexcept {
        return append_real(value, precision);
    }

    [[nodiscard]] constexpr usize append(enum_type auto value) noexcept {
        return (
            append_null_terminated_string("enum(") +
            append_integer(static_cast<usize>(value)) +
            append(')'));
    }

    [[nodiscard]] constexpr usize append(implements<RepresentableTag> auto const &value) noexcept {
        return append(value.repr());
    }

    [[nodiscard]] constexpr usize append(implements<ReprToTag> auto const &value) noexcept {
        return value.repr_to(*this);
    }

    // concrete append algorithms

    [[nodiscard]] constexpr usize append_null_terminated_string(char const *str, usize max_length = static_cast<usize>(-1)) noexcept {
        usize write_count = 0;

        if (nullptr != str) {
            while (write_count < max_length) {
                char const c = str[write_count];

                if (c == '\0') {
                    break;
                }

                if (not this->append(c)) {
                    break;
                }

                write_count += 1;
            }
        }

        return write_count;
    }

    [[nodiscard]] constexpr usize append_integer(i64 value) noexcept {
        char buffer[21];
        auto const result = std::to_chars(buffer, buffer + sizeof(buffer), value);
        if (result.ec == std::errc{}) {
            return append_null_terminated_string(buffer, static_cast<usize>(result.ptr - buffer));
        } else {
            return append_null_terminated_string("err");
        }
    }

    [[nodiscard]] constexpr usize append_real(f64 value, usize precision) noexcept {
        if (math::isnan(value)) {
            return append_null_terminated_string("nan");
        }

        if (math::isinf(value)) {
            return append_null_terminated_string(value < 0 ? "-inf" : "+inf");
        }

        char buffer[64];
        auto const result = std::to_chars(
            buffer, buffer + sizeof(buffer),
            value,
            std::chars_format::fixed,
            static_cast<int>(precision));

        if (result.ec == std::errc{}) {
            return append_null_terminated_string(buffer, static_cast<usize>(result.ptr - buffer));
        } else {
            return append_null_terminated_string("err");
        }
    }

    /// @brief Append Formatted string
    /// @tparam ...Args argument types (auto-deduced), used for `append` method static dispatching
    /// @param fmt format string implicit consteval-constructed from literal
    /// @param ...args format arguments
    /// @note For argument placement use `{}` as anchor
    template<typename... Args> [[nodiscard]] constexpr usize append_format(internal::FormatString<Args...> const &fmt, Args const &...args) noexcept {
        auto const tuple = std::forward_as_tuple(args...);
        usize write_count = 0;

        for (auto i = 0u; i < fmt.result.count; i += 1) {
            auto const &token = fmt.result.tokens[i];

            if (token.kind == internal::FormatToken::Literal) {
                write_count += this->append(StringView{fmt.str + token.start, token.length});
            } else if constexpr (sizeof...(Args) > 0) {
                write_count += apply_arg<0>(fmt.result.anchor_indices[i], tuple);
            }
        }

        return write_count;
    }

private:
    template<usize I, typename... Args> [[nodiscard]] constexpr usize apply_arg(usize index, std::tuple<Args const &...> const &tuple) {
        if (I == index) {
            return append(std::get<I>(tuple));
        } else if constexpr (I + 1 < sizeof...(Args)) {
            return apply_arg<I + 1>(index, tuple);
        }
        return 0;
    }
};

}// namespace kf::mixin

#define KF_IMPL_WRITE(...) friend struct ::kf::internal::WriteBase<__VA_ARGS__>
