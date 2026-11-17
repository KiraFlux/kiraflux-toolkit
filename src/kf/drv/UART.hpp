// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/UART.hpp
/// @brief   Buffered UART driver.

#pragma once

#ifdef ARDUINO
#include <HardwareSerial.h>
#endif

#include "kf/Bytes.hpp"
#include "kf/BytesView.hpp"
#include "kf/Option.hpp"
#include "kf/Result.hpp"
#include "kf/core.hpp"
#include "kf/gpio.hpp"

#include "kf/mixin/BinaryReadable.hpp"
#include "kf/mixin/BinaryWritable.hpp"
#include "kf/mixin/Flush.hpp"
#include "kf/mixin/Initable.hpp"
#include "kf/mixin/NonCopyable.hpp"
#include "kf/mixin/ReadAvailable.hpp"
#include "kf/mixin/WriteAvailable.hpp"

#include "kf/drv/Driver.hpp"

namespace kf {

namespace internal {

enum struct UartError {
    NoSlot,
    ReadFailed,
    WriteFailed,
};

struct UartCommonConfig {
    u32 baudrate;

    /// @brief `none` keeps the platform default.
    Option<usize> rx_buffer_length{none};

    /// @brief `none` keeps the platform default.
    Option<usize> tx_buffer_length{none};

    /// @brief Invert line levels (RS-485 transceivers, etc.).
    bool inverted{false};
};

struct UartSpecificConfig {
    u8 uart_num;

    gpio::GpioNumber rx_gpio_num, tx_gpio_num;
};

using UartTraits = drv::DriverTraits<drv::CommonSpecific<UartCommonConfig, UartSpecificConfig>, NoneType, UartError>;

using UartResult = Result<void, UartError>;

template<typename Impl> struct UartBase :

    drv::Driver<Impl, UartTraits>,

    mixin::BinaryReadable<Impl, UartError>,
    mixin::ReadAvailable<Impl>,

    mixin::BinaryWritable<Impl, UartResult>,
    mixin::WriteAvailable<Impl>,

    mixin::Flush<Impl> {
    using drv::Driver<Impl, UartTraits>::Driver;
};

#define KF_IMPL_UART_BASE(__impl__)                                                \
    KF_IMPL_DRIVER(__impl__);                                                      \
    KF_IMPL_BINARY_READABLE(__impl__, typename Traits::Error);                     \
    KF_IMPL_READ_AVAILABLE(__impl__);                                              \
    KF_IMPL_BINARY_WRITABLE(__impl__, ::kf::Result<void, typename Traits::Error>); \
    KF_IMPL_WRITE_AVAILABLE(__impl__);                                             \
    KF_IMPL_FLUSH(__impl__)

#ifdef ARDUINO

struct UartImpl : UartBase<UartImpl> {
    using Self = UartImpl;

    explicit UartImpl(Dependencies const &deps) noexcept :
        UartBase<UartImpl>{deps}, _serial{deps.config.specific.uart_num} {}

private:
    mutable HardwareSerial _serial;

    KF_IMPL_UART_BASE(Self);

    UartResult init_impl() noexcept {

        if (this->_config.common.rx_buffer_length.is_some()) {
            (void) _serial.setRxBufferSize(this->_config.common.rx_buffer_length.unwrap());
        }

        if (this->_config.common.tx_buffer_length.is_some()) {
            (void) _serial.setTxBufferSize(this->_config.common.tx_buffer_length.unwrap());
        }

        ::pinMode(static_cast<u8>(this->_config.specific.rx_gpio_num), INPUT_PULLUP);

        _serial.begin(
            this->_config.common.baudrate,
            SERIAL_8N1,
            static_cast<i8>(this->_config.specific.rx_gpio_num),
            static_cast<i8>(this->_config.specific.tx_gpio_num),
            this->_config.common.inverted);

        return ok();
    }

    void deinit_impl() noexcept {
        _serial.end();
    }

    auto read_buffer_impl(Bytes buffer) noexcept -> Result<BytesView, Traits::Error> {
        if (buffer.empty()) {
            return ok(BytesView{});
        }

        auto const bytes_read = _serial.readBytes(buffer.data(), buffer.length());

        if (bytes_read == 0) {
            return error(Traits::Error::ReadFailed);
        }

        return ok(BytesView{buffer.data(), bytes_read});
    }

    template<typename T> auto read_packet_impl() noexcept -> Result<T, Traits::Error> {
        constexpr auto to_read = sizeof(T);

        if constexpr (to_read == sizeof(u8)) {
            int const c = _serial.read();

            if ((-1) == c) {
                return error(Traits::Error::ReadFailed);
            }

            return ok(static_cast<T>(c));

        } else {
            T packet;

            auto const bytes_read = _serial.readBytes(reinterpret_cast<u8 *>(&packet), to_read);

            if (to_read != bytes_read) {
                return error(Traits::Error::ReadFailed);
            }

            return ok(packet);
        }
    }

    usize available_for_read_impl() const noexcept {
        return _serial.available();
    }

    UartResult write_buffer_impl(BytesView buffer) noexcept {
        if (not buffer.empty()) {
            auto const to_write = buffer.length();

            if (_serial.write(buffer.data(), to_write) != to_write) {
                return error(Traits::Error::WriteFailed);
            }
        }

        return ok();
    }

    UartResult write_packet_impl(trivial auto const &packet) noexcept {
        constexpr auto to_write = sizeof(packet);

        usize written;

        if constexpr (to_write == sizeof(u8)) {
            written = _serial.write(static_cast<u8>(packet));
        } else {
            written = _serial.write(reinterpret_cast<u8 const *>(&packet), to_write);
        }

        if (to_write != written) {
            return error(Traits::Error::WriteFailed);
        } else {
            return ok();
        }
    }

    UartResult write_mixed_impl(trivial auto const &header, BytesView buffer) noexcept {
        kf_try(this->write_packet(header));
        return this->write_buffer(buffer);
    }

    usize available_for_write_impl() const noexcept {
        auto const ret = _serial.availableForWrite();
        // Arduino Stream returns 0 when "may write any size".
        return (0 == ret) ? static_cast<usize>(-1) : ret;
    }

    void flush_impl() noexcept {
        _serial.flush();
    }
};

#else

struct UartImpl : UartBase<UartImpl> {
    using Self = UartImpl;

    using UartBase<UartImpl>::UartBase;

private:
    KF_IMPL_UART_BASE(Self);

    UartResult init_impl() noexcept {
        return ok();
    }

    void deinit_impl() noexcept {}

    auto read_buffer_impl(Bytes buffer) const noexcept -> Result<BytesView, Traits::Error> {
        (void) buffer;

        return error(Traits::Error::ReadFailed);
    }

    template<typename T> auto read_packet_impl() const noexcept -> Result<T, Traits::Error> {
        return error(Traits::Error::ReadFailed);
    }

    usize available_for_read_impl() const noexcept {
        return 0;
    }

    UartResult write_buffer_impl(BytesView buffer) const noexcept {
        (void) buffer;

        return ok();
    }

    UartResult write_packet_impl(trivial auto const &packet) const noexcept {
        (void) packet;

        return ok();
    }

    UartResult write_mixed_impl(trivial auto const &header, BytesView buffer) const noexcept {
        (void) header;
        (void) buffer;

        return ok();
    }

    usize available_for_write_impl() const noexcept {
        return static_cast<usize>(-1);
    }

    void flush_impl() noexcept {}
};

#endif

}// namespace internal

namespace drv {

/// @brief Buffered UART with availability checks and flush.
/// @note Arduino: `HardwareSerial`. Native: no-op stub.
using UART = internal::UartImpl;

}// namespace drv

}// namespace kf

#undef KF_IMPL_UART_BASE
