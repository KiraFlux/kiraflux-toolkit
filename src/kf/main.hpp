// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    main.hpp
/// @brief   Defines the application entry point and the Init context (logger, arena, I/O).

#pragma once

#include "kf/Arena.hpp"
#include "kf/Logger.hpp"
#include "kf/Option.hpp"
#include "kf/core.hpp"

// autoconfig

#ifndef KF_CONFIG_MAIN_ARENA_BUFFER_LENGTH
#define KF_CONFIG_MAIN_ARENA_BUFFER_LENGTH (1024 * 48)
#endif

#ifndef KF_CONFIG_MAIN_LOGGER_KEY
#define KF_CONFIG_MAIN_LOGGER_KEY "main"
#endif

#ifndef KF_CONFIG_MAIN_LOGGER_BUFFER_LENGTH
#define KF_CONFIG_MAIN_LOGGER_BUFFER_LENGTH 256
#endif

#ifndef KF_CONFIG_MAIN_UART_BAUDRATE
#define KF_CONFIG_MAIN_UART_BAUDRATE 115200
#endif

#ifndef KF_CONFIG_MAIN_UART_NUM
#define KF_CONFIG_MAIN_UART_NUM 0
#endif

//

namespace kf {

/// @brief Application Context
struct Init;

/// @brief KiraFlux Toolkit application entry point
void main(Init &init);

}// namespace kf

#ifdef ARDUINO

#include <Arduino.h>

#include "kf/drv/UART.hpp"

namespace kf::internal {

using MainIo = drv::UART;

constexpr auto create_main_io_common_config() noexcept {
    return drv::UART::Traits::Config::Common{
        .baudrate = KF_CONFIG_MAIN_UART_BAUDRATE,
    };
}

constexpr auto create_main_io_specific_config() noexcept {
    return drv::UART::Traits::Config::Specific{
        .uart_num = KF_CONFIG_MAIN_UART_NUM,
        .rx_gpio_num = kf::gpio::G3,
        .tx_gpio_num = kf::gpio::G1,
    };
}

}// namespace kf::internal

#define KF_PLATFORM_APP_FUNCTION_DECLARATION void setup()
#define KF_PLATFORM_APP_QUIT()

void loop() {}// not used

#else

#include <csignal>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>

#include "kf/Bytes.hpp"
#include "kf/BytesView.hpp"
#include "kf/Result.hpp"

#include "kf/mixin/BinaryReadable.hpp"
#include "kf/mixin/BinaryWritable.hpp"
#include "kf/mixin/Flush.hpp"
#include "kf/mixin/ReadAvailable.hpp"
#include "kf/mixin/WriteAvailable.hpp"

#include "kf/drv/Driver.hpp"

#define KF_PLATFORM_APP_FUNCTION_DECLARATION int main()
#define KF_PLATFORM_APP_QUIT() (0)

namespace kf {

namespace internal {

enum struct MainIoError : char {
    NoSlot,
    ReadFailed,
    WriteFailed,
};

using MainIoWriteResult = Result<void, MainIoError>;

using MainIoTraits = drv::DriverTraits<drv::CommonSpecific<NoneType, NoneType>, NoneType, MainIoError>;

struct MainIo :

    drv::Driver<MainIo, MainIoTraits>,

    mixin::BinaryReadable<MainIo, MainIoError>,
    mixin::ReadAvailable<MainIo>,

    mixin::BinaryWritable<MainIo, MainIoWriteResult>,
    mixin::WriteAvailable<MainIo>,

    mixin::Flush<MainIo>

{
    using Self = MainIo;

    using drv::Driver<MainIo, MainIoTraits>::Driver;

private:
    inline static struct termios _orig_tty{};
    inline static bool _is_raw{false}, _restored{false};

    static void restore_terminal() noexcept {
        if (not _is_raw or _restored) { return; }
        _restored = true;

        // Restore original terminal settings
        (void) ::tcsetattr(STDIN_FILENO, TCSANOW, &_orig_tty);

        // Clear O_NONBLOCK flag
        int flags = ::fcntl(STDIN_FILENO, F_GETFL, 0);
        if (flags != -1) {
            (void) ::fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
        }

        _is_raw = false;
    }

    static void signal_handler(int sig) noexcept {
        (void) sig;
        restore_terminal();
        ::_exit(0);
    }

    static void setup_signal_handlers() noexcept {
        struct sigaction sa{};
        sa.sa_handler = signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        (void) ::sigaction(SIGINT, &sa, nullptr);
        (void) ::sigaction(SIGTERM, &sa, nullptr);
        (void) ::sigaction(SIGQUIT, &sa, nullptr);
    }

    KF_IMPL_DRIVER(Self);

    MainIoWriteResult init_impl() noexcept {
        if (::isatty(STDIN_FILENO)) {
            // Save original terminal settings
            ::tcgetattr(STDIN_FILENO, &_orig_tty);

            // Set raw mode (like Arduino UART)
            struct termios raw = _orig_tty;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            ::tcsetattr(STDIN_FILENO, TCSANOW, &raw);

            // Set stdin to non-blocking mode
            int flags = ::fcntl(STDIN_FILENO, F_GETFL, 0);
            if (flags != -1) {
                (void) ::fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
            }

            _is_raw = true;
            _restored = false;

            // Register terminal restoration on normal exit
            ::atexit(restore_terminal);

            // Install signal handlers
            setup_signal_handlers();
        }

        // Disable sync with C I/O (for raw mode)
        std::ios_base::sync_with_stdio(false);

        return ok();
    }

    void deinit_impl() noexcept {
        restore_terminal();
    }

    KF_IMPL_BINARY_READABLE(Self, Traits::Error);

    auto read_buffer_impl(Bytes buffer) noexcept -> Result<BytesView, Traits::Error> {
        if (buffer.empty()) {
            return ok(BytesView{});
        }

        auto const n = ::read(STDIN_FILENO, buffer.data(), buffer.length());

        if (n < 0) {
            if (errno == EAGAIN or errno == EWOULDBLOCK) {
                return ok(BytesView{});
            }
            return error(Traits::Error::ReadFailed);
        }

        // EOF
        if (n == 0) {
            return error(Traits::Error::ReadFailed);
        }

        return ok(BytesView{buffer.data(), static_cast<usize>(n)});
    }

    template<trivial T> auto read_packet_impl() -> Result<T, Traits::Error> {
        T packet;

        auto result = this->read_buffer(Bytes{
            reinterpret_cast<u8 *>(&packet),
            sizeof(T),
        });

        if (result.is_error()) {
            return error(result.error());
        }

        if (result.ok().length() != sizeof(T)) {
            return error(Traits::Error::ReadFailed);
        }

        return ok(packet);
    }

    KF_IMPL_READ_AVAILABLE(Self);
    usize available_for_read_impl() const noexcept {
        return std::cin.rdbuf()->in_avail();
    }

    KF_IMPL_BINARY_WRITABLE(Self, MainIoWriteResult);

    MainIoWriteResult write_buffer_impl(BytesView buffer) noexcept {
        if (not buffer.empty()) {
            auto const to_write = buffer.length();

            if (not std::cout.write(reinterpret_cast<char const *>(buffer.data()), to_write).good()) {
                return error(Traits::Error::WriteFailed);
            }
        }

        return ok();
    }

    MainIoWriteResult write_packet_impl(trivial auto const &packet) noexcept {
        constexpr auto to_write = sizeof(packet);

        if constexpr (to_write == sizeof(u8)) {
            std::cout.put(static_cast<char>(packet));
        } else {
            std::cout.write(reinterpret_cast<char const *>(&packet), to_write);
        }

        if (not std::cout.good()) {
            return error(Traits::Error::WriteFailed);
        } else {
            return ok();
        }
    }

    MainIoWriteResult write_mixed_impl(trivial auto const &header, BytesView buffer) noexcept {
        kf_try(this->write_packet(header));
        return this->write_buffer(buffer);
    }

    KF_IMPL_WRITE_AVAILABLE(Self);
    usize available_for_write_impl() const noexcept {
        return std::cout.good() ? static_cast<usize>(-1) : 0;
    }

    KF_IMPL_FLUSH(Self);
    void flush_impl() noexcept {
        (void) std::cout.flush();
    }
};

constexpr auto create_main_io_common_config() noexcept {
    return none;
}

constexpr auto create_main_io_specific_config() noexcept {
    return none;
}

}// namespace internal

}// namespace kf

#endif

namespace kf {

struct Init {

    /// @brief Platform-depended standard binary Input/Output Stream
    internal::MainIo &io;

    /// @brief Application Arena allocator
    Arena arena;

    /// @brief Application Logger
    Logger logger;
};

}// namespace kf

KF_PLATFORM_APP_FUNCTION_DECLARATION {

    using Io = kf::internal::MainIo;

    static kf::u8 main_arena_buffer[(KF_CONFIG_MAIN_ARENA_BUFFER_LENGTH)]{};
    static char main_logger_buffer[(KF_CONFIG_MAIN_LOGGER_BUFFER_LENGTH)]{};

    static constexpr auto io_common = kf::internal::create_main_io_common_config();

    static kf::Option<Io> io_storage{kf::none};

    static kf::Init init{
        .io{
            Io::create(
                io_storage,
                Io::Dependencies{
                    .config{
                        .common = io_common,
                        .specific = kf::internal::create_main_io_specific_config(),
                    }})
                .ok(),
        },
        .arena{
            {main_arena_buffer},
        },
        .logger{
            (KF_CONFIG_MAIN_LOGGER_KEY),
            {main_logger_buffer},
        },
    };

    kf::Logger::writer = [](auto str) {
        (void) init.io.write_buffer({reinterpret_cast<kf::u8 const *>(str.data()), str.length()});
        init.io.flush();
    };

    init.logger.info("KiraFlux Toolkit v0.3.5");

    main(init);

    init.io.destroy(io_storage);

    return KF_PLATFORM_APP_QUIT();
}

// cleanup

#undef KF_PLATFORM_APP_FUNCTION_DECLARATION
#undef KF_PLATFORM_APP_QUIT

#undef KF_CONFIG_MAIN_ARENA_BUFFER_LENGTH
#undef KF_CONFIG_MAIN_LOGGER_KEY
#undef KF_CONFIG_MAIN_LOGGER_BUFFER_LENGTH
#undef KF_CONFIG_MAIN_UART_BAUDRATE
#undef KF_CONFIG_MAIN_UART_NUM
