// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/net.espnow-ping'

// Broadcasts a counter every second; every discovered peer
// gets a personalized string on the next tick.

#include <kf/Logger.hpp>
#include <kf/MacAddress.hpp>
#include <kf/Stack.hpp>
#include <kf/String.hpp>
#include <kf/core.hpp>
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

#include <kf/drv/Espnow.hpp>

#ifndef ARDUINO
#error "drv/net.espnow-ping requires ESP32 backend"
#endif

#include <esp_event.h>
#include <esp_netif.h>

using kf::MacAddress;
using kf::drv::Espnow;

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/net.espnow-ping");

    // One-time ESP-IDF bootstrap, before any WiFi/ESP-NOW setup.
    (void) esp_netif_init();
    (void) esp_event_loop_create_default();

    auto espnow_pool = init.arena.create_pool<Espnow>(1).unwrap();
    auto peer_pool = init.arena.create_pool<Espnow::Peer>(8).unwrap();

    auto const espnow_result = Espnow::create(
        espnow_pool,
        Espnow::Dependencies{
            .config{.auto_start_wifi = true},
        });

    if (espnow_result.is_error()) {
        init.logger.error("Espnow: {}", espnow_result.error());
        return;
    }
    auto &espnow = espnow_result.ok();
    kf_defer(
        espnow.destroy(espnow_pool);
        init.logger.debug("destroy espnow"););
    init.logger.info("self: {}", espnow.mac());

    auto const bcast_result = Espnow::Peer::create(
        peer_pool,
        Espnow::Peer::Dependencies{
            .mac_address = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
            .wifi_interface_sta = true,
        });

    if (bcast_result.is_error()) {
        init.logger.error("broadcast: {}", bcast_result.error());
        return;
    }
    auto &broadcast = bcast_result.ok();
    kf_defer(
        init.logger.debug("destroy broadcast");
        broadcast.destroy(peer_pool););

    // Every discovered peer, lives in peer_pool.
    kf::Stack<Espnow::Peer *> peers{init.arena.allocate<Espnow::Peer *>(peer_pool.capacity())};
    kf_defer(for (auto *p : peers) {
        init.logger.debug("destroy: {}", p->mac());
        p->destroy(peer_pool);
    });

    static kf::Logger espnow_logger{"espnow", init.arena.allocate<char>(32)};

    espnow.callback([&peers, &peer_pool](MacAddress const &from, BytesView data) {
        espnow_logger.info("<- {}: {} bytes", from, data.length());

        if (peers.first_where([&](Espnow::Peer *p) { return p->mac() == from; }).is_some()) {
            return;
        }

        auto const pr = Espnow::Peer::create(
            peer_pool,
            Espnow::Peer::Dependencies{
                .mac_address = from,
                .wifi_interface_sta = true,
            });

        if (pr.is_ok()) {
            (void) peers.write(&pr.ok());
            espnow_logger.info("discovered {}", from);
        }
    });

    u32 counter{0};

    // Bounded run: 30 * 1 s = 30 s.
    for (int i = 0; i < 30; i += 1) {
        (void) broadcast.write_packet(counter);
        espnow_logger.info("counter: {}", counter);

        for (auto *p: peers) {
            char msg_buf[48];
            kf::String msg{{msg_buf}};
            (void) msg.append_format("hi {} #{}", p->mac(), counter);
            (void) p->write_buffer({reinterpret_cast<u8 const *>(msg.data()), msg.length()});
            espnow_logger.info("msg: {}", msg);
        }

        counter += 1;
        rtos::Task::sleep(1000);
    }
}
