// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'network/espnow-ping'

#include <kf/BytesView.hpp>
#include <kf/Option.hpp>
#include <kf/Slice.hpp>
#include <kf/core.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Clock.hpp>
#include <kf/rtos/Task.hpp>

#include <WiFi.h>// ESP-NOW requires STA mode – set explicitly
#include <kf/MacAddress.hpp>
#include <kf/esp/Espnow.hpp>

using kf::MacAddress;
using kf::esp::Espnow;

// --- Globals ---

kf::Option<Espnow::Peer> broadcast_peer{kf::none};
kf::Option<Espnow::Peer> target_peer{kf::none};

// --- Factory function ---

kf::Option<Espnow::Peer> create_peer(kf::Init &init, MacAddress const &mac_address) noexcept {
    auto result = Espnow::Peer::create(Espnow::Peer::Config{
        .mac_address = mac_address,
        .wifi_interface_sta = true,
    });

    if (result.is_ok()) {
        return kf::some(std::move(result.ok()));
    }

    init.logger.warn("Failed to create peer {}: {}", mac_address, result.error());
    return kf::none;
}

// --- Callback ---

void on_receive(kf::Init &init, MacAddress const &mac, kf::Slice<kf::u8 const> data) {
    if (target_peer.is_some() and target_peer.unwrap().mac() == mac) {
        init.logger.info("target: got {} bytes", data.length());
        return;
    }

    init.logger.debug("(unknown): from {} got {} bytes", mac, data.length());
}

// --- Main application ---

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: network/espnow-ping");

    // --- WiFi setup ---

    if (not WiFiClass::mode(WIFI_MODE_STA)) {
        init.logger.error("Failed to set Wi-Fi mode to STA");
        return;
    }

    // --- ESP-NOW initialization ---

    auto init_result = Espnow::instance().init();
    if (init_result.is_error()) {
        init.logger.error("ESP-NOW init failed: {}", init_result.error());
        return;
    }
    init.logger.info("ESP-NOW initialized successfully");

    // --- Self MAC address ---

    MacAddress const &self_mac = Espnow::instance().mac();
    init.logger.info("Self MAC: {}", self_mac);

    // --- Callback registration ---

    Espnow::instance().callback([&](MacAddress const &mac, BytesView data) {
        on_receive(init, mac, data);
    });

    // --- Create peers ---

    broadcast_peer = create_peer(init, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff});
    target_peer = create_peer(init, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06});

    // Bounded ping loop: 10 pings, ~5 s apart.
    for (int iteration = 0; iteration < 10; iteration += 1) {
        if (broadcast_peer.is_some()) {
            init.logger.info("Sending: broadcast");
            auto const result = broadcast_peer.unwrap().write_packet("[broadcast]: ping");
            if (result.is_error()) {
                init.logger.error("[broadcast]: send failed: {}", result.error());
            }
        }

        if (target_peer.is_some()) {
            init.logger.info("Sending: target");
            auto const result = target_peer.unwrap().write_packet("ping");
            if (result.is_error()) {
                init.logger.error("[target]: send failed: {}", result.error());
            }
        }

        rtos::Task::sleep(5000);
        init.logger.debug("Tick: {}", rtos::Clock::now());
    }

    // Peers must be released BEFORE deinit of ESP-NOW —
    // Peer::~Peer() calls esp_now_del_peer(), which fails after esp_now_deinit().
    broadcast_peer.reset();
    target_peer.reset();

    // Stop ESP-NOW and unregister the receive callback.
    Espnow::instance().deinit();
    init.logger.info("ESP-NOW deinitialized");
}
