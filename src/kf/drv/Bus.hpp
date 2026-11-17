// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/Bus.hpp
/// @brief   CRTP base for bus implementations and node creation.

#pragma once

#include "kf/Result.hpp"
#include "kf/core.hpp"

#include "kf/mixin/BinaryReadable.hpp"
#include "kf/mixin/BinaryWritable.hpp"
#include "kf/mixin/Initable.hpp"
#include "kf/mixin/NonCopyable.hpp"

#include "kf/drv/Driver.hpp"

namespace kf::drv {

// TODO: docs
struct BusNodeTag {};

// TODO: docs
template<typename Impl, typename ErrorImpl> struct BusNode :

    BusNodeTag,
    mixin::NonCopyable,
    mixin::BinaryReadable<Impl, ErrorImpl>,
    mixin::BinaryWritable<Impl, Result<void, ErrorImpl>>

{};

// TODO: docs
struct BusTraitsTag {};

template<

    implements<BusNodeTag> N,
    typename Config,
    typename Extra,
    typename... Args

    >
struct BusTraits :

    BusTraitsTag,
    DriverTraits<Config, Extra, typename N::Error, Args...>

{
    using Node = N;
};

// TODO: docs
struct BusTag {};

/// @brief CRTP base class for bus implementations.
/// @tparam Impl Concrete bus implementation (must inherit from this class).
/// @tparam D TODO: docs
/// @note The bus implementation must provide methods `init_impl()` and `deinit_impl()`.
template<typename Impl, implements<BusTraitsTag> D> struct Bus :

    BusTag,
    Driver<Impl, D>

{
    using Driver<Impl, D>::Driver;

    [[nodiscard]] constexpr typename D::Node create_node(typename D::Node::Config const &config) noexcept {
        return typename D::Node{*static_cast<Impl *>(this), config};
    }
};

}// namespace kf::drv

#define KF_IMPL_BUS_NODE(__impl__, __error_impl__)     \
    KF_IMPL_BINARY_READABLE(__impl__, __error_impl__); \
    KF_IMPL_BINARY_WRITABLE(__impl__, ::kf::Result<void, __error_impl__>)

#define KF_IMPL_BUS(...) KF_IMPL_DRIVER(__VA_ARGS__)
