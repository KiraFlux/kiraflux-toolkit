// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'drv/storage.nvm'

// Demonstrates typed non-volatile storage as a key/value map:
// - NVM lifecycle via create/destroy + kf_defer
// - get<T> / set<T> for fixed types: POD, enum, bool, integer
// - get(key, Bytes) / set(key, BytesView) for variable-length payloads
// - size(key) queries blob length without reading
// - exists(key) / erase(key) at namespace level
// - Single commit() for the whole batch

#include <kf/Option.hpp>
#include <kf/main.hpp>

#include <kf/drv/NVM.hpp>

// Detects corrupted or version-mismatched config blobs.
constexpr kf::u32 config_magic = 0xAA'BB'CC'DD;

struct AppConfig {
    kf::u32 magic;
    char device_name[16];
    float x, y, z;
};

enum struct Mode : kf::u8 {
    Idle,
    Active,
    Calibration,
};

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: drv/storage.nvm");

    // Lifecycle: create + defer(destroy).
    Option<drv::NVM> storage{none};
    auto const nvm_r = drv::NVM::create(
        storage,
        drv::NVM::Dependencies{
            .config{.nvm_namespace = "app_cfg"},
        });

    if (nvm_r.is_error()) {
        init.logger.error("NVM init failed: {}", nvm_r.error());
        return;
    }
    auto &nvm = nvm_r.ok();
    kf_defer(nvm.destroy(storage););

    // POD config with version magic.
    // Missing, corrupted or version-mismatched - write defaults.
    AppConfig cfg{};
    if (auto ld = nvm.get<AppConfig>("cfg"); ld.is_ok()) {
        cfg = ld.ok();
    }
    if (cfg.magic != config_magic) {
        cfg = {
            .magic = config_magic,
            .device_name = "KiraFlux",
            .x = 1.0f,
            .y = 2.0f,
            .z = 3.0f,
        };
        (void) nvm.set("cfg", cfg);
        init.logger.info("cfg: wrote defaults");
    } else {
        init.logger.info("cfg: name='{}' pos=({}, {}, {})",
                         cfg.device_name, cfg.x, cfg.y, cfg.z);
    }

    // Fixed types: enum (u8), bool, int - each dispatches to a native NVS type.
    if (auto m = nvm.get<Mode>("mode"); m.is_error()) {
        (void) nvm.set<Mode>("mode", Mode::Idle);
    }
    if (auto a = nvm.get<bool>("armed"); a.is_error()) {
        (void) nvm.set<bool>("armed", false);
    }
    if (auto c = nvm.get<u32>("count"); c.is_error()) {
        (void) nvm.set<u32>("count", 0);
    }

    // Variable-length bytes:
    // size() queries length without reading,
    // get(key, Bytes) reads into a caller-provided buffer,
    // set(key, BytesView) writes.
    if (auto sz = nvm.size("data"); sz.is_ok()) {
        auto buf = init.arena.allocate<u8>(sz.ok());
        if (auto rd = nvm.get("data", buf); rd.is_ok()) {
            init.logger.info("data: {} bytes", rd.ok().length());
        }
    }

    u8 const payload[]{0x01, 0x02, 0x03, 0x04, 0x05};
    (void) nvm.set("data", BytesView{payload, sizeof(payload)});

    // Namespace-level: exists / erase, no typed value needed.
    if (auto ex = nvm.exists("legacy_key"); ex.is_ok() and ex.ok()) {
        (void) nvm.erase("legacy_key");
        init.logger.info("legacy_key erased");
    }

    // One commit for the batch above.
    if (auto c = nvm.commit(); c.is_error()) {
        init.logger.error("commit: {}", c.error());
    }
}