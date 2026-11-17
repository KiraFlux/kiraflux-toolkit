// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    mixin/Initable.hpp
/// @brief   Storage-based create()/destroy() for resource-dependent objects.

#pragma once

#include <concepts>   // std::convertible_to
#include <type_traits>// std::is_void_v
#include <utility>    // std::forward

#include "kf/Result.hpp"
#include "kf/core.hpp"

#include "kf/mixin/Storage.hpp"

namespace kf::mixin {

/// @brief Marker for Initable traits bundles.
struct InitableTraitsTag {};

/// @brief Traits bundle for `mixin::Initable`.
/// @tparam E Error enum with `NoSlot`.
/// @tparam R Resource for `init_impl`/`deinit_impl`; `void` - none.
template<typename E, typename R = void>
    requires requires { { E::NoSlot } -> std::convertible_to<E>; }
struct InitableTraits : InitableTraitsTag {
    using Error = E;
    using Resource = R;

    using ActionResult = Result<void, Error>;
};

/// @brief Marker for Initable interface.
struct InitableTag {};

/// @brief CRTP mixin: construct Impl in storage, call `init_impl` (rollback on failure), free on destroy.
/// @tparam Impl Concrete type. Must implement `init_impl([R]) -> Result<void, E>` and `deinit_impl([R]) -> void`.
/// @tparam D    `InitableTraits<E, R>`.
template<typename Impl, implements<InitableTraitsTag> D> struct Initable : InitableTag {

    struct Traits : D {
        using TraitsBase = D;
    };

    using R = Result<Impl &, typename Traits::Error>;

    // create

    [[nodiscard]] static R create(implements<StorageOf<Impl>> auto &storage, auto &&...args) noexcept
        requires std::is_void_v<typename Traits::Resource>
    {
        return construct(storage, [](auto &self) { return self.init_impl(); }, std::forward<decltype(args)>(args)...);
    }

    [[nodiscard]] static R create(implements<StorageOf<Impl>> auto &storage, auto &&resource, auto &&...args) noexcept
        requires(not std::is_void_v<typename Traits::Resource>)
    {
        return construct(storage, [&resource](auto &self) { return self.init_impl(resource); }, std::forward<decltype(args)>(args)...);
    }

    // destroy (instance) - delegate to static

    void destroy(implements<StorageOf<Impl>> auto &storage) noexcept
        requires std::is_void_v<typename Traits::Resource>
    {
        destroy(static_cast<Impl *>(this), storage);
    }

    void destroy(implements<StorageOf<Impl>> auto &storage, auto &&resource) noexcept
        requires(not std::is_void_v<typename Traits::Resource>)
    {
        destroy(static_cast<Impl *>(this), storage, std::forward<decltype(resource)>(resource));
    }

    // destroy (static, null-safe, owner-safe, idempotent)

    static void destroy(Impl *ptr, implements<StorageOf<Impl>> auto &storage) noexcept
        requires std::is_void_v<typename Traits::Resource>
    {
        if (nullptr == ptr or not storage.contains(ptr)) { return; }
        ptr->deinit_impl();
        storage.free(*ptr);
    }

    static void destroy(Impl *ptr, implements<StorageOf<Impl>> auto &storage, auto &&resource) noexcept
        requires(not std::is_void_v<typename Traits::Resource>)
    {
        if (nullptr == ptr or not storage.contains(ptr)) { return; }
        ptr->deinit_impl(resource);
        storage.free(*ptr);
    }

private:
    [[nodiscard]] static R construct(implements<StorageOf<Impl>> auto &storage, auto &&init, auto &&...args) noexcept {
        auto maybe = storage.alloc(std::forward<decltype(args)>(args)...);
        if (maybe.is_none()) { return error(Traits::Error::NoSlot); }

        auto &self = maybe.unwrap();
        if (auto r = init(self); r.is_error()) {
            storage.free(self);
            return error(r.error());
        }
        return ok_ref(self);
    }
};

}// namespace kf::mixin

#define KF_IMPL_INITABLE(...) friend struct ::kf::mixin::Initable<__VA_ARGS__, typename Traits::TraitsBase>
