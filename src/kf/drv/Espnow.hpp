// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/Espnow.hpp
/// @brief   ESP-NOW peer-to-peer communication driver.

#pragma once

#include <algorithm>
#include <utility>

#include "kf/BytesView.hpp"
#include "kf/MacAddress.hpp"
#include "kf/Result.hpp"
#include "kf/StringView.hpp"
#include "kf/core.hpp"

#include "kf/mixin/BinaryWritable.hpp"
#include "kf/mixin/Callbacked.hpp"
#include "kf/mixin/Initable.hpp"
#include "kf/mixin/MacAddressed.hpp"

#include "kf/drv/Driver.hpp"

#ifdef ARDUINO
#include <esp_event.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_now.h>
#include <esp_wifi.h>
#endif

namespace kf {

namespace internal {

enum struct EspnowError {
    Unknown,

    NotInitialized,
    InvalidArg,
    NoMemory,
    PeerListIsFull,
    NotFound,
    Internal,
    AlreadyExists,
    IncorrectWiFiMode,
    ChannelMismatch,

    TooBigMessage,
    AlreadyInitialized,
    WiFiStartFailed,
    NoSlot,
};

struct EspnowConfig {
    /// @note true  - Espnow starts WiFi (STA) on init, stops on deinit.
    ///       false - caller manages WiFi lifetime.
    /// `esp_netif_init` and `esp_event_loop_create_default` MUST be called by the caller once at application startup, regardless of this flag.
    bool auto_start_wifi;

    static constexpr usize max_data_len{
#ifdef ARDUINO
        ESP_NOW_MAX_DATA_LEN
#else
        250
#endif
    };
};

template<typename Impl> using EspnowDriver = drv::Driver<
    Impl,
    drv::DriverTraits<
        EspnowConfig,
        NoneType,
        EspnowError>>;

template<typename Impl> struct EspnowBase :

    EspnowDriver<Impl>,
    mixin::MacAddressed,
    mixin::Callbacked<void(MacAddress const &, BytesView)>

{
    explicit constexpr EspnowBase(typename EspnowDriver<Impl>::Dependencies const &deps) noexcept :
        EspnowDriver<Impl>{deps}, mixin::MacAddressed{MacAddress{}} {}
};

template<typename Impl> struct EspnowPeerBase :

    mixin::NonCopyable,
    mixin::MacAddressed,
    mixin::BinaryWritable<Impl, Result<void, EspnowError>>,
    mixin::Initable<Impl, mixin::InitableTraits<EspnowError>>

{

    struct Dependencies {
        MacAddress mac_address;
        bool wifi_interface_sta;
    };

    explicit EspnowPeerBase(Dependencies const &deps) noexcept :
        mixin::MacAddressed{deps.mac_address},
        _wifi_interface_sta{deps.wifi_interface_sta} {}

protected:
    bool _wifi_interface_sta;
};

}// namespace internal

namespace drv {

/// @brief ESP-NOW protocol driver.
/// @note Only one instance may be initialized at a time.
struct Espnow;

#ifdef ARDUINO

struct Espnow : internal::EspnowBase<Espnow> {

    using internal::EspnowBase<Espnow>::EspnowBase;

    /// @brief ESP-NOW peer representation.
    /// @note Managed via `Initable`: `create(pool, deps)` / `destroy(ptr, pool)`.
    struct Peer : internal::EspnowPeerBase<Peer> {

        using internal::EspnowPeerBase<Peer>::EspnowPeerBase;

    private:
        KF_IMPL_INITABLE(Peer);

        auto init_impl() noexcept -> Traits::ActionResult {
            esp_now_peer_info_t info{
                .channel = 0,
                .ifidx = static_cast<wifi_interface_t>(_wifi_interface_sta ? WIFI_IF_STA : WIFI_IF_AP),
                .encrypt = false,
            };
            std::copy(this->mac().begin(), this->mac().end(), info.peer_addr);

            return map_espnow_result(esp_now_add_peer(&info));
        }

        void deinit_impl() noexcept {
            (void) esp_now_del_peer(this->mac().data());
        }

        KF_IMPL_BINARY_WRITABLE(Peer, Traits::ActionResult);

        auto write_buffer_impl(BytesView buffer) noexcept -> Traits::ActionResult {
            if (buffer.length() > internal::EspnowConfig::max_data_len) {
                return error(Traits::Error::TooBigMessage);
            }
            return send_raw(buffer.data(), buffer.length());
        }

        auto write_packet_impl(trivial auto const &packet) noexcept -> Traits::ActionResult {
            static_assert(sizeof(packet) <= internal::EspnowConfig::max_data_len, "Message is too big");
            return send_raw(reinterpret_cast<u8 const *>(&packet), sizeof(packet));
        }

        auto write_mixed_impl(trivial auto const &header, BytesView buffer) noexcept -> Traits::ActionResult {
            auto const mixed_size = sizeof(header) + buffer.length();
            if (mixed_size > internal::EspnowConfig::max_data_len) {
                return error(Traits::Error::TooBigMessage);
            }

            u8 mixed[mixed_size];
            auto const header_data = reinterpret_cast<u8 const *>(&header);
            std::copy(header_data, header_data + sizeof(header), mixed);
            std::copy(buffer.begin(), buffer.end(), mixed + sizeof(header));
            return send_raw(mixed, mixed_size);
        }

        auto send_raw(u8 const *data, usize len) noexcept -> Traits::ActionResult {
            return map_espnow_result(esp_now_send(this->mac().data(), data, len));
        }
    };

private:
    inline static Espnow *_instance{nullptr};

    [[nodiscard]] static auto map_espnow_error(esp_err_t e) noexcept -> internal::ResultErrorWrapper<Traits::Error> {
        using E = Traits::Error;
        switch (e) {
            case ESP_ERR_ESPNOW_NOT_INIT: return {E::NotInitialized};
            case ESP_ERR_ESPNOW_ARG: return {E::InvalidArg};
            case ESP_ERR_ESPNOW_NO_MEM: return {E::NoMemory};
            case ESP_ERR_ESPNOW_FULL: return {E::PeerListIsFull};
            case ESP_ERR_ESPNOW_NOT_FOUND: return {E::NotFound};
            case ESP_ERR_ESPNOW_INTERNAL: return {E::Internal};
            case ESP_ERR_ESPNOW_EXIST: return {E::AlreadyExists};
            case ESP_ERR_ESPNOW_IF: return {E::IncorrectWiFiMode};
            case ESP_ERR_ESPNOW_CHAN: return {E::ChannelMismatch};
            default: return {E::Unknown};
        }
    }

    [[nodiscard]] static auto map_espnow_result(esp_err_t e) noexcept -> Traits::ActionResult {
        if (ESP_OK == e) {
            return ok();
        } else {
            return map_espnow_error(e);
        }
    }

    static void on_receive(esp_now_recv_info_t const *info, u8 const *data, int size) noexcept {
        MacAddress source{};
        std::copy(info->src_addr, info->src_addr + ESP_NOW_ETH_ALEN, source.begin());
        _instance->invoke(source, BytesView{data, static_cast<usize>(size)});
    }

    [[nodiscard]] auto start_wifi() noexcept -> Traits::ActionResult {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        if (auto e = esp_wifi_init(&cfg); ESP_OK != e) {
            return error(Traits::Error::WiFiStartFailed);
        }
        if (auto e = esp_wifi_set_mode(WIFI_MODE_STA); ESP_OK != e) {
            return error(Traits::Error::WiFiStartFailed);
        }
        if (auto e = esp_wifi_start(); ESP_OK != e) {
            return error(Traits::Error::WiFiStartFailed);
        }
        return ok();
    }

    void stop_wifi() noexcept {
        (void) esp_wifi_stop();
        (void) esp_wifi_deinit();
    }

    KF_IMPL_DRIVER(Espnow);

    auto init_impl() noexcept -> Traits::ActionResult {
        if (nullptr != _instance) {
            return error(Traits::Error::AlreadyInitialized);
        }

        if (this->_config.auto_start_wifi) {
            kf_try(start_wifi());
        }

        if (auto e = esp_now_init(); ESP_OK != e) {
            if (this->_config.auto_start_wifi) { stop_wifi(); }
            return map_espnow_error(e);
        }

        _instance = this;

        if (auto e = esp_now_register_recv_cb(on_receive); ESP_OK != e) {
            _instance = nullptr;
            (void) esp_now_deinit();
            if (this->_config.auto_start_wifi) { stop_wifi(); }
            return map_espnow_error(e);
        }

        (void) esp_read_mac(_mac_address.data(), ESP_MAC_WIFI_STA);
        return ok();
    }

    void deinit_impl() noexcept {
        (void) esp_now_unregister_recv_cb();
        _instance = nullptr;
        (void) esp_now_deinit();
        if (this->_config.auto_start_wifi) { stop_wifi(); }
    }
};

#else

struct Espnow : internal::EspnowBase<Espnow> {

    using internal::EspnowBase<Espnow>::EspnowBase;

    struct Peer : internal::EspnowPeerBase<Peer> {

        using internal::EspnowPeerBase<Peer>::EspnowPeerBase;

    private:
        KF_IMPL_INITABLE(Peer);

        auto init_impl() noexcept -> Traits::ActionResult {
            return ok();
        }

        void deinit_impl() noexcept {}

        KF_IMPL_BINARY_WRITABLE(Peer, Traits::ActionResult);

        auto write_buffer_impl(BytesView buffer) noexcept -> Traits::ActionResult {
            return ok();
        }

        auto write_packet_impl(trivial auto const &packet) noexcept -> Traits::ActionResult {
            return ok();
        }

        auto write_mixed_impl(trivial auto const &header, BytesView buffer) noexcept -> Traits::ActionResult {
            return ok();
        }
    };

private:
    KF_IMPL_DRIVER(Espnow);

    auto init_impl() noexcept -> Traits::ActionResult {
        return ok();
    }

    void deinit_impl() noexcept {}
};

#endif

}// namespace drv

#define CASE_RETURN(__v) \
    case __v: return #__v

[[nodiscard]] constexpr auto repr(drv::Espnow::Traits::Error e) noexcept -> StringView {
    switch (e) {
        CASE_RETURN(drv::Espnow::Traits::Error::Unknown);
        CASE_RETURN(drv::Espnow::Traits::Error::NotInitialized);
        CASE_RETURN(drv::Espnow::Traits::Error::InvalidArg);
        CASE_RETURN(drv::Espnow::Traits::Error::NoMemory);
        CASE_RETURN(drv::Espnow::Traits::Error::PeerListIsFull);
        CASE_RETURN(drv::Espnow::Traits::Error::NotFound);
        CASE_RETURN(drv::Espnow::Traits::Error::Internal);
        CASE_RETURN(drv::Espnow::Traits::Error::AlreadyExists);
        CASE_RETURN(drv::Espnow::Traits::Error::IncorrectWiFiMode);
        CASE_RETURN(drv::Espnow::Traits::Error::ChannelMismatch);
        CASE_RETURN(drv::Espnow::Traits::Error::TooBigMessage);
        CASE_RETURN(drv::Espnow::Traits::Error::AlreadyInitialized);
        CASE_RETURN(drv::Espnow::Traits::Error::WiFiStartFailed);
        CASE_RETURN(drv::Espnow::Traits::Error::NoSlot);
    }
    return "Unknown";
}

}// namespace kf

#undef CASE_RETURN
