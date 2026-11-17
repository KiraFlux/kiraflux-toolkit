// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/I2C.hpp
/// @brief   I2C bus and node abstraction

#pragma once

#ifdef ARDUINO
#include <Wire.h>
#endif

#include "kf/Bytes.hpp"
#include "kf/BytesView.hpp"
#include "kf/Option.hpp"
#include "kf/Result.hpp"
#include "kf/StringView.hpp"
#include "kf/core.hpp"
#include "kf/gpio.hpp"
#include "kf/units.hpp"

#include "kf/mixin/Configured.hpp"

#include "kf/drv/Bus.hpp"

#define CASE_RETURN(__v) \
    case __v: return #__v

namespace kf {

namespace internal {

/// @brief Error codes for I2C operations
/// @note Most errors correspond directly to Arduino Wire library failure conditions

enum struct IicError {

    NoSlot,

    // Bus errors

    ClockConfigFailed,     ///< Setting I2C clock frequency failed (Wire.setClock() returned false)
    BufferSizeConfigFailed,///< Setting the internal buffer size failed (Wire.setBufferSize() returned different value)
    BeginFailed,           ///< Wire.begin() failed (returned false)

    // Node errors

    AddressNack,     ///< Device did not acknowledge its address after START condition
    DataNack,        ///< Device did not acknowledge a data byte during transmission
    Timeout,         ///< Transaction timed out (exceeded Wire timeout)
    BufferTooLong,   ///< Data to send exceeds the internal Wire transmit buffer size
    IncompletePacket,///< Read operation returned fewer bytes than requested
    Unknown,         ///< Any other unspecified error from Arduino Wire (endTransmission code 4)
};

using IicResult = Result<void, IicError>;

struct IicNodeConfig final {

    /// @brief 7‑bit I2C device address (usually 0x08–0x77)
    /// @note Wire.h uses 7‑bit format
    u8 address;
};

struct IicBusCommonConfig final {
    Option<usize> clock_hz, buffer_size, timeout_ms;
};

struct IicBusSpecificConfig final {
    u8 bus_num;
    Option<gpio::GpioNumber> gpio_num_sda, gpio_num_scl;
};

template<typename Impl> struct IicNodeBase :

    drv::BusNode<Impl, IicError>,
    mixin::Configured<IicNodeConfig>

{
    using mixin::Configured<IicNodeConfig>::Configured;
};

template<typename B> struct IicNodeImpl;

#ifdef ARDUINO

/// @brief I2C node implementation that adapts Arduino TwoWire to the library's BinaryReadable/BinaryWritable interfaces
/// @tparam B The bus implementation type (I2C)
/// @note This class is movable but not copyable. It holds a reference to the underlying TwoWire instance
///       and manages the I2C address and transaction state. All I/O operations are blocking
template<typename B> struct IicNodeImpl : IicNodeBase<IicNodeImpl<B>> {
    using Self = IicNodeImpl<B>;

    using Error = IicError;
    using Config = IicNodeConfig;

    explicit constexpr IicNodeImpl(B &bus, Config const &config) noexcept :
        IicNodeBase<IicNodeImpl>{config}, _wire{bus._wire} {}

private:
    TwoWire &_wire;

    KF_IMPL_BUS_NODE(Self, Error);

    auto read_buffer_impl(Bytes buffer) noexcept -> Result<BytesView, Error> {
        usize const received = request(buffer.length());

        if (received == 0) {
            return error(Error::Timeout);
        }

        read_bytes_unchecked(buffer.data(), received);

        return ok(BytesView{buffer.data(), received});
    }

    template<trivial T> auto read_packet_impl() noexcept -> Result<T, Error> {
        constexpr usize requested = sizeof(T);

        usize const received = request(requested);

        if (received == 0) {
            return error(Error::Timeout);
        }

        if (received != requested) {
            discard_receive_buffer();
            return error(Error::IncompletePacket);
        }

        if constexpr (requested == sizeof(u8)) {
            return ok(static_cast<T>(_wire.read()));
        } else {
            T packet;
            read_bytes_unchecked(reinterpret_cast<u8 *>(&packet), requested);
            return ok(packet);
        }
    }

    IicResult write_buffer_impl(BytesView buffer) noexcept {
        begin_transmission();
        usize const written = write_bytes(buffer.data(), buffer.length());
        return end_transmission(written, buffer.length());
    }

    IicResult write_packet_impl(trivial auto const &packet) noexcept {
        begin_transmission();
        usize const written = write_packet_unchecked(packet);
        return end_transmission(written, sizeof(decltype(packet)));
    }

    IicResult write_mixed_impl(trivial auto const &header, BytesView buffer) noexcept {
        begin_transmission();
        usize const header_written = write_packet_unchecked(header);
        usize const buffer_written = write_bytes(buffer.data(), buffer.length());
        return end_transmission(header_written + buffer_written, sizeof(decltype(header)) + buffer.length());
    }

    /// @brief Request `requested` bytes from the I2C device
    /// @return Number of bytes actually available
    [[nodiscard]] usize request(usize requested) noexcept {
        return _wire.requestFrom(static_cast<int>(this->config().address), static_cast<int>(requested));
    }

    /// @brief Read raw bytes from the internal Wire buffer after a successful request
    /// @warning Assumes that exactly `length` bytes are available; call only after checking request() result
    void read_bytes_unchecked(u8 *buffer, usize length) noexcept {
        (void) _wire.readBytes(buffer, length);
    }

    /// @brief Discard any remaining bytes in the receive buffer (flush)
    /// @note Used after a partial read or error to prepare for the next transaction
    void discard_receive_buffer() noexcept {
        auto const to_discard = _wire.available();
        for (auto i = 0; i < to_discard; i += 1) {
            (void) _wire.read();
        }
    }

    /// @brief Begin an I2C transmission (send START condition)
    /// @note Must be called before writing any data
    void begin_transmission() noexcept {
        _wire.beginTransmission(this->config().address);
    }

    /// @brief Write raw bytes to the I2C device (must be between begin/endTransmission)
    /// @return number of bytes actually placed in the internal transmit buffer (may be less than `length` if buffer full)
    [[nodiscard]] usize write_bytes(u8 const *buffer, usize length) noexcept {
        return _wire.write(buffer, length);
    }

    /// @brief End the transmission (send STOP) and check for errors
    /// @param written Number of bytes successfully written in this transaction
    /// @param to_write Total number of bytes that were intended to be written
    /// @return Success or specific I2C error
    /// @note Possible errors: AddressNack, DataNack, Timeout, BufferTooLong, Unknown
    [[nodiscard]] IicResult end_transmission(usize written, usize to_write) noexcept {
        u8 const code = _wire.endTransmission();

        if (written != to_write) { return error(Error::BufferTooLong); }

        switch (code) {
            case 0: return ok();
            case 1: return error(Error::BufferTooLong);
            case 2: return error(Error::AddressNack);
            case 3: return error(Error::DataNack);
            case 4: return error(Error::Unknown);
            case 5: return error(Error::Timeout);
            default: return error(Error::Unknown);
        }
    }

    /// @brief Write a single‑byte packet without checking (used internally for small writes)
    [[nodiscard]] usize write_packet_unchecked(u8 packet) noexcept {
        return _wire.write(packet);
    }

    /// @brief Write a multi‑byte packet without checking (used internally)
    [[nodiscard]] usize write_packet_unchecked(auto const &packet) noexcept {
        return write_bytes(reinterpret_cast<u8 const *>(&packet), sizeof(decltype(packet)));
    }
};

#else

template<typename B> struct IicNodeImpl : IicNodeBase<IicNodeImpl<B>> {
    using Self = IicNodeImpl<B>;

    using Error = IicError;
    using Config = IicNodeConfig;

    explicit constexpr IicNodeImpl(B &bus, Config const &config) noexcept :
        IicNodeBase<Self>{config} {}

private:
    KF_IMPL_BUS_NODE(Self, Error);

    auto read_buffer_impl(Bytes buffer) noexcept -> Result<BytesView, Error> {
        return error(Error::Unknown);
    }

    template<trivial T> auto read_packet_impl() noexcept -> Result<T, Error> {
        return error(Error::Unknown);
    }

    IicResult write_buffer_impl(BytesView buffer) noexcept {
        return ok();
    }

    IicResult write_packet_impl(trivial auto const &packet) noexcept {
        return ok();
    }

    IicResult write_mixed_impl(trivial auto const &header, BytesView buffer) noexcept {
        return ok();
    }
};

#endif

template<typename Impl> using IicBusBase = drv::Bus<
    Impl,
    drv::BusTraits<
        IicNodeImpl<Impl>,
        drv::CommonSpecific<IicBusCommonConfig, IicBusSpecificConfig>,
        NoneType>>;

#ifdef ARDUINO

struct IicBusImpl : IicBusBase<IicBusImpl> {
    using Self = IicBusImpl;

    friend typename Traits::Node;

    explicit IicBusImpl(Dependencies const &deps) noexcept :
        IicBusBase<Self>{deps}, _wire{deps.config.specific.bus_num} {}

private:
    TwoWire _wire;

    [[nodiscard]] static constexpr int uncast(Option<gpio::GpioNumber> g) noexcept {
        if (g.is_none()) {
            return -1;
        } else {
            return static_cast<int>(g.unwrap());
        }
    }

    KF_IMPL_BUS(Self);

    IicResult init_impl() noexcept {

        if (not _wire.begin(uncast(this->_config.specific.gpio_num_sda), uncast(this->_config.specific.gpio_num_scl))) {
            return error(Traits::Error::BeginFailed);
        }

        if (this->_config.common.clock_hz.is_some()) {
            if (not _wire.setClock(this->_config.common.clock_hz.unwrap())) {
                return error(Traits::Error::ClockConfigFailed);
            }
        }

        if (this->_config.common.timeout_ms.is_some()) {
            _wire.setTimeout(static_cast<u16>(this->_config.common.timeout_ms.unwrap()));// TwoWire::setTimeout
        }

        if (this->_config.common.buffer_size.is_some()) {
            if (_wire.setBufferSize(this->_config.common.buffer_size.unwrap()) != this->_config.common.buffer_size.unwrap()) {
                return error(Traits::Error::BufferSizeConfigFailed);
            }
        }

        return ok();
    }

    void deinit_impl() noexcept {
        (void) _wire.end();// just ignore
    }
};

#else

struct IicBusImpl : IicBusBase<IicBusImpl> {
    using Self = IicBusImpl;

    friend typename Traits::Node;

    using IicBusBase<IicBusImpl>::IicBusBase;

private:
    KF_IMPL_BUS(Self);

    IicResult init_impl() noexcept {
        return ok();
    }

    void deinit_impl() noexcept {}
};

#endif

}// namespace internal

namespace drv {

using I2C = internal::IicBusImpl;

}// namespace drv

[[nodiscard]] constexpr StringView repr(drv::I2C::Traits::Error e) noexcept {
    switch (e) {
        CASE_RETURN(drv::I2C::Traits::Error::NoSlot);
        CASE_RETURN(drv::I2C::Traits::Error::ClockConfigFailed);
        CASE_RETURN(drv::I2C::Traits::Error::BufferSizeConfigFailed);
        CASE_RETURN(drv::I2C::Traits::Error::BeginFailed);
        CASE_RETURN(drv::I2C::Traits::Error::AddressNack);
        CASE_RETURN(drv::I2C::Traits::Error::DataNack);
        CASE_RETURN(drv::I2C::Traits::Error::Timeout);
        CASE_RETURN(drv::I2C::Traits::Error::BufferTooLong);
        CASE_RETURN(drv::I2C::Traits::Error::IncompletePacket);

        default:
            CASE_RETURN(drv::I2C::Traits::Error::Unknown);
    }
}

}// namespace kf

#undef CASE_RETURN
