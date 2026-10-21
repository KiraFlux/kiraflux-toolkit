// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    Function.hpp

#pragma once

#include <cstdlib>
#include <new>
#include <type_traits>
#include <utility>

#include "kf/core.hpp"

#include "kf/mixin/NonCopyable.hpp"

namespace kf {

namespace internal {

struct OptionHelper;

template<typename> struct SentinelStorage;

}// namespace internal

template<typename> struct Function;

/// @brief Type‑erased callable with small buffer optimisation (4 words)
/// @note Never empty.
/// @tparam R Return type
/// @tparam Args Argument types
template<typename R, typename... Args> struct Function<R(Args...)> : mixin::NonCopyable {
    friend struct ::kf::internal::OptionHelper;
    friend struct ::kf::internal::SentinelStorage<Function<R(Args...)>>;

    using ReturnType = R;

    explicit Function(std::nullptr_t) = delete;

    Function(auto &&f) noexcept {
        using Decayed = std::decay_t<decltype(f)>;
        static_assert(not std::is_same_v<Decayed, Function>, "Cannot construct Function from another Function (use move)");
        static_assert(sizeof(Impl<Decayed>) <= (buffer_size - 1), "Callable object too large for Function buffer");
        static_assert(alignof(Impl<Decayed>) <= alignment, "Callable alignment too strict");
        static_assert(std::is_invocable_r<R, decltype(f), Args...>::value, "Callable not invocable with given arguments");

        new (static_cast<void *>(_storage)) Impl<Decayed>{std::forward<decltype(f)>(f)};
        is_some(true);
    }

    Function(Function &&other) noexcept {
        move_from(std::move(other));
    }

    Function &operator=(Function &&other) noexcept {
        if (this != &other) {
            destroy();
            move_from(std::move(other));
        }
        return *this;
    }

    ~Function() noexcept {
        destroy();
    }

    /// @brief Invoke the stored callable
    template<typename... CallArgs> R operator()(CallArgs &&...args) const noexcept {
        if constexpr (std::is_void_v<R>) {
            func().invoke(std::forward<CallArgs>(args)...);
        } else {
            return func().invoke(std::forward<CallArgs>(args)...);
        }
    }

private:
    struct Base {
        virtual ~Base() noexcept = default;

        virtual R invoke(Args... args) const noexcept = 0;

        virtual void move_to(void *dest) noexcept = 0;
    };

    template<typename _F> struct Impl final : Base {
        _F f;

        explicit Impl(auto &&func) noexcept :
            f(std::forward<decltype(func)>(func)) {}

        R invoke(Args... args) const noexcept override {
            return f(std::forward<Args>(args)...);
        }

        void move_to(void *dest) noexcept override {
            new (dest) Impl{std::move(f)};
        }
    };

    static constexpr usize buffer_size{4 * sizeof(void *)}, alignment{alignof(std::max_align_t)};

    alignas(alignment) u8 _storage[buffer_size]{};

    // access to default constructor only for SentinelStorage
    constexpr Function() noexcept {
        is_some(false);
    }

    [[nodiscard]] bool is_some() const noexcept {
        return static_cast<bool>(_storage[buffer_size - 1]);
    }

    void is_some(bool is_some) noexcept {
        _storage[buffer_size - 1] = static_cast<u8>(is_some);
    }

    [[nodiscard]] Base &func() noexcept {
        return *reinterpret_cast<Base *>(_storage);
    }

    [[nodiscard]] Base const &func() const noexcept {
        return *reinterpret_cast<Base const *>(_storage);
    }

    void destroy() noexcept {
        if (is_some()) {
            func().~Base();
            is_some(false);
        }
    }

    void move_from(Function &&other) noexcept {
        if (other.is_some()) {
            other.func().move_to(static_cast<void *>(_storage));
            is_some(true);
            other.is_some(false);
        }
    }
};

}// namespace kf