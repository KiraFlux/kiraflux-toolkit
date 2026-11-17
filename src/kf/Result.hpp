// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    Result.hpp

#pragma once

#include <cstdlib>
#include <type_traits>
#include <utility>

#include "kf/core.hpp"
#include "kf/mixin/ReprTo.hpp"

namespace kf {

struct ResultTag {};

namespace internal {

/// @brief Base class for result error handling logic (CRTP)
template<typename Impl, trivial E> struct ResultBase :

    ResultTag,
    mixin::ReprTo<Impl>

{

    /// @brief Check if result contains an error
    [[nodiscard]] constexpr bool is_error() const noexcept {
        return static_cast<Impl const *>(this)->is_error_impl();
    }

    /// @brief Get the stored error (panic if not error)
    [[nodiscard]] constexpr E const &error() const noexcept {
        if (is_error()) { return static_cast<Impl const *>(this)->get_error_impl(); }
        abort();
    }
};

#define KF_IMPL_RESULT_BASE(__impl__, __error__)                   \
    friend struct ::kf::internal::ResultBase<__impl__, __error__>; \
    KF_IMPL_REPR_TO(__impl__)

/// @brief Internal wrapper for a success value (used with kf::ok)
template<typename T> struct ResultValueWrapper {
    T value;
};

/// @brief Specialization of ResultValueWrapper for void
template<> struct ResultValueWrapper<void> {};

/// @brief Internal wrapper for an error value (used with kf::error)
template<typename E> struct ResultErrorWrapper {
    E value;
};

}// namespace internal

/// @brief Result type that can hold either a value or an error
/// @tparam T Type of successful result value
/// @tparam E Type of error value
/// @note Embedded‑friendly alternative to exceptions.
///       Use kf::ok() and kf::error() to create instances.
/// @see kf::ok, kf::error
template<typename T, typename E> struct Result final : internal::ResultBase<Result<T, E>, E> {

    using Self = Result<T, E>;
    using ValueType = T;

    /// @brief Construct from ResultValueWrapper
    /// @see kf::ok(T)
    constexpr Result(internal::ResultValueWrapper<T> ok) noexcept :
        _is_ok{true}, _ok{std::move(ok.value)} {}

    /// @brief Construct from ResultErrorWrapper
    /// @see kf::error(E)
    constexpr Result(internal::ResultErrorWrapper<E> error) noexcept :
        _is_ok{false}, _error{std::move(error.value)} {}

    constexpr ~Result() noexcept {
        if (_is_ok) {
            _ok.~T();
        } else {
            _error.~E();
        }
    }

    /// @brief Check if result contains a value (success)
    [[nodiscard]] constexpr bool is_ok() const noexcept { return _is_ok; }

    /// @brief Get the stored value (abort if is error)
    [[nodiscard]] T &ok() & noexcept {
        if (this->is_error()) { abort(); }
        return _ok;
    }

    [[nodiscard]] T ok() && noexcept {
        if (this->is_error()) { abort(); }
        return std::move(_ok);
    }

    /// @brief Get the stored value (const overload)
    [[nodiscard]] T const &ok() const & noexcept {
        return const_cast<Result *>(this)->ok();
    }

    /// @brief Transform the success value using a function
    /// @param f Mapping function: T -> U
    /// @return Result<U, E> containing the mapped value on success,
    ///         otherwise the original error.
    [[nodiscard]] auto map(auto &&f) const noexcept -> Result<decltype(f(std::declval<T>())), E> {
        if (_is_ok) {
            if constexpr (std::is_void_v<decltype(f(std::declval<T>()))>) {
                f(_ok);
                return {internal::ResultValueWrapper<void>{}};
            } else {
                return {internal::ResultValueWrapper<decltype(f(std::declval<T>()))>{f(_ok)}};
            }
        } else {
            return {internal::ResultErrorWrapper<E>{_error}};
        }
    }

    /// @brief Transform the error value using a function
    /// @param f Mapping function: E -> W
    /// @return Result<T, W> containing the mapped error on failure,
    ///         otherwise the original success value.
    [[nodiscard]] auto map_error(auto &&f) const noexcept -> Result<T, decltype(f(std::declval<E>()))> {
        if (_is_ok) {
            return {internal::ResultValueWrapper<T>{_ok}};
        } else {
            return {internal::ResultErrorWrapper<decltype(f(std::declval<E>()))>{f(_error)}};
        }
    }

private:
    union {
        T _ok;
        E _error;
    };
    bool _is_ok;

    KF_IMPL_RESULT_BASE(Self, E);

    [[nodiscard]] constexpr bool is_error_impl() const noexcept {
        return not _is_ok;
    }

    [[nodiscard]] constexpr E const &get_error_impl() const noexcept {
        return _error;
    }

    constexpr usize repr_to_impl(implements<mixin::WriteTag<char>> auto &char_writable) const noexcept {
        usize ret = 0;
        if (this->is_error()) {
            ret += char_writable.append("error(");
            ret += char_writable.append(_error);
        } else {
            ret += char_writable.append("ok(");
            ret += char_writable.append(_ok);
        }
        ret += char_writable.append(')');
        return ret;
    }
};

/// @brief Result type that can hold either reference to a value or an error
/// @tparam T Type of successful result value
/// @tparam E Type of error value
/// @note Embedded‑friendly alternative to exceptions.
///       Use kf::ok_ref() and kf::error() to create instances.
/// @see kf::ok_ref, kf::error
template<typename T, typename E> struct Result<T &, E> final : internal::ResultBase<Result<T &, E>, E> {

    using Self = Result<T &, E>;
    using ValueType = T &;

    /// @brief Construct from ResultValueWrapper
    /// @see kf::ok_ref(T&)
    constexpr Result(internal::ResultValueWrapper<T &> ok) noexcept :
        _ok{&ok.value} {}

    /// @brief Construct from ResultErrorWrapper
    /// @see kf::error(E)
    constexpr Result(internal::ResultErrorWrapper<E> error) noexcept :
        _ok{nullptr}, _error{std::move(error.value)} {}

    constexpr ~Result() noexcept {
        if (this->is_error()) {
            _error.~E();
        }
    }

    /// @brief Check if result contains a value (success)
    [[nodiscard]] constexpr bool is_ok() const noexcept {
        return nullptr != _ok;
    }

    [[nodiscard]] T &ok() const noexcept {
        if (this->is_error()) { abort(); }
        return *_ok;
    }

    /// @brief Transform the success value using a function
    /// @param f Mapping function: T& -> U
    /// @return Result<U, E> containing the mapped value on success,
    ///         otherwise the original error.
    [[nodiscard]] auto map(auto &&f) const noexcept -> Result<decltype(f(std::declval<T &>())), E> {
        if (this->is_ok()) {
            if constexpr (std::is_void_v<decltype(f(std::declval<T &>()))>) {
                f(*_ok);
                return {internal::ResultValueWrapper<void>{}};
            } else {
                return {internal::ResultValueWrapper<decltype(f(std::declval<T &>()))>{f(*_ok)}};
            }
        } else {
            return {internal::ResultErrorWrapper<E>{_error}};
        }
    }

    /// @brief Transform the error value using a function
    /// @param f Mapping function: E -> W
    /// @return Result<T&, W> containing the mapped error on failure,
    ///         otherwise the original success value.
    [[nodiscard]] auto map_error(auto &&f) const noexcept -> Result<T &, decltype(f(std::declval<E>()))> {
        if (this->is_ok()) {
            return {internal::ResultValueWrapper<T &>{*_ok}};
        } else {
            return {internal::ResultErrorWrapper<decltype(f(std::declval<E>()))>{f(_error)}};
        }
    }

private:
    T *_ok;
    E _error;

    KF_IMPL_RESULT_BASE(Self, E);

    constexpr bool is_error_impl() const noexcept {
        return nullptr == _ok;
    }

    constexpr E const &get_error_impl() const noexcept {
        return _error;
    }

    constexpr usize repr_to_impl(implements<mixin::WriteTag<char>> auto &char_writable) const noexcept {
        usize ret = 0;
        if (this->is_error()) {
            ret += char_writable.append("error(");
            ret += char_writable.append(_error);
        } else {
            ret += char_writable.append("ok_ref(");
            ret += char_writable.append(*_ok);
        }
        ret += char_writable.append(')');
        return ret;
    }
};

/// @brief Specialization of Result for void success value
/// @tparam E Type of error value
template<typename E> struct Result<void, E> final : internal::ResultBase<Result<void, E>, E> {

    using Self = Result<void, E>;
    using ValueType = void;

    /// @brief Construct from ResultValueWrapper<void>
    /// @see kf::ok()
    constexpr Result(internal::ResultValueWrapper<void>) noexcept :
        _is_error{false} {}

    /// @brief Construct from ResultErrorWrapper
    /// @see kf::error(E)
    constexpr Result(internal::ResultErrorWrapper<E> error) noexcept :
        _is_error{true}, _error{std::move(error.value)} {}

    constexpr ~Result() noexcept {
        if (_is_error) {
            _error.~E();
        }
    }

    constexpr void ok() const noexcept {}

    /// @brief Check if result is successful
    [[nodiscard]] constexpr bool is_ok() const noexcept {
        return not _is_error;
    }

    /// @brief Transform the error value using a function
    /// @param f Mapping function: E -> W
    /// @return Result<T, W> containing the mapped error on failure,
    ///         otherwise the original success value.
    [[nodiscard]] auto map_error(auto &&f) const noexcept -> Result<void, decltype(f(std::declval<E>()))> {
        if (_is_error) {
            return {internal::ResultErrorWrapper<decltype(f(std::declval<E>()))>{f(_error)}};
        } else {
            return {internal::ResultValueWrapper<void>{}};
        }
    }

private:
    E _error;
    bool _is_error;

    KF_IMPL_RESULT_BASE(Self, E);

    [[nodiscard]] bool is_error_impl() const noexcept {
        return _is_error;
    }

    [[nodiscard]] E const &get_error_impl() const noexcept {
        return _error;
    }

    constexpr usize repr_to_impl(implements<mixin::WriteTag<char>> auto &char_writable) const noexcept {
        if (this->is_error()) {
            return (
                char_writable.append("error(") +
                char_writable.append(_error) +
                char_writable.append(')'));
        } else {
            return char_writable.append("ok()");
        }
    }
};

/// @brief Create a successful Result (value)
/// @param value The success value
/// @return internal::ResultValueWrapper<T> for implicit conversion to Result<T, E>
constexpr auto ok(auto &&value) noexcept -> internal::ResultValueWrapper<std::decay_t<decltype(value)>> {
    return {std::forward<decltype(value)>(value)};
}

template<typename T> constexpr auto ok_ref(T &value) noexcept -> internal::ResultValueWrapper<T &> {
    return {value};
}

/// @brief Create a successful Result<void,  E>
/// @return internal::ResultValueWrapper<void> for implicit conversion to Result<void, E>
constexpr auto ok() noexcept -> internal::ResultValueWrapper<void> {
    return {};
}

/// @brief Create an error Result
/// @param error The error value
/// @return internal::ResultErrorWrapper<E> for implicit conversion to Result<T, E>
constexpr auto error(auto &&error) noexcept -> internal::ResultErrorWrapper<std::decay_t<decltype(error)>> {
    return {std::forward<decltype(error)>(error)};
}

}// namespace kf

/// @brief Early-return macro for Result error propagation
/// @param __expr__ Expression of type `Result<T, E>`
/// @return The success value `T`, or returns the error from the enclosing function
/// @note Expression evaluated once. Requires GNU statement expressions.
#define kf_try(__expr__)                                                                                                    \
    ({                                                                                                                      \
        auto &&result = (__expr__);                                                                                         \
        static_assert(std::is_base_of_v<::kf::ResultTag, std::decay_t<decltype(result)>>, "kf_try requires a Result type"); \
        if (result.is_error()) {                                                                                            \
            return ::kf::error(result.error());                                                                             \
        }                                                                                                                   \
        result.ok();                                                                                                        \
    })

#define kf_try_map_err(__expr__, __error__)                                                                                     \
    ({                                                                                                                          \
        auto &&_r = (__expr__);                                                                                                 \
        static_assert(std::is_base_of_v<::kf::ResultTag, std::decay_t<decltype(_r)>>, "kf_try_map_err requires a Result type"); \
        if (_r.is_error()) return ::kf::error(__error__);                                                                       \
        _r.ok();                                                                                                                \
    })
