// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    driver/display/ST7735.hpp
/// @brief   ST7735 TFT driver (128×160, SPI, RGB565).

#pragma once

#include "kf/BytesView.hpp"
#include "kf/Result.hpp"
#include "kf/bus/SPI.hpp"
#include "kf/core.hpp"
#include "kf/gpio.hpp"
#include "kf/image/ViewportImage.hpp"
#include "kf/pixel/Rgb565Pixel.hpp"
#include "kf/rtos/Task.hpp"

#include "kf/mixin/Configured.hpp"
#include "kf/mixin/NonCopyable.hpp"

#include "kf/driver/display/DisplayDriver.hpp"
#include "kf/driver/display/Orientation.hpp"

namespace kf::internal {

using ST7735Image = image::ViewportImage<pixel::Rgb565Pixel, 128, 160>;

/// @brief Hardware configuration for ST7735
struct ST7735Config final {
    bus::SPI::Node::Config spi_node;
    driver::display::Orientation init_orientation;
};

}// namespace kf::internal

namespace kf::driver::display {

/// @brief ST7735 TFT display driver for 128x160 RGB565 panels
struct ST7735 final :

    DisplayDriver<ST7735, internal::ST7735Image, internal::SpiWriteResult>,
    mixin::Configured<internal::ST7735Config>

{
    using Self = ST7735;
    using PixelImpl = internal::ST7735Image::PixelImpl;
    using Config = internal::ST7735Config;

    /// @brief Memory Access Control (MADCTL) register bits
    enum MadCtl : u8 {
        RgbMode = 0x00,///< RGB color order
        BgrMode = 0x08,///< BGR color order

        MirrorTranspose = 0x20,///< Transpose image (Swap X and Y axes)
        MirrorX = 0x40,        ///< Horizontal image mirror
        MirrorY = 0x80,        ///< Vertical image mirror
    };

    /// @brief ST7735 command set (partial)
    enum class Command : u8 {
        SWRESET = 0x01,///< Software reset

        SLPIN = 0x10, ///< Enter sleep mode
        SLPOUT = 0x11,///< Exit sleep mode

        INVOFF = 0x20, ///< Disable color inversion
        INVON = 0x21,  ///< Enable color inversion
        DISPOFF = 0x28,///< Turn display off
        DISPON = 0x29, ///< Turn display on
        CASET = 0x2A,  ///< Set column address range
        RASET = 0x2B,  ///< Set row address range
        RAMWR = 0x2C,  ///< Write to display RAM

        MADCTL = 0x36,///< Memory access control
        COLMOD = 0x3A ///< Color mode setting
    };

    explicit constexpr ST7735(bus::SPI &spi_bus, Config const &config, gpio::GpioNumber gpio_num_data_command, gpio::GpioNumber gpio_num_reset) noexcept :
        mixin::Configured<internal::ST7735Config>{config},
        _spi_node{spi_bus.create_node(config.spi_node)},
        _gpio_data_command{gpio_num_data_command},
        _gpio_hardware_reset{gpio_num_reset} {}

private:
    bus::SPI::Node _spi_node;
    gpio::DigitalOutput _gpio_data_command;
    gpio::DigitalOutput _gpio_hardware_reset;

    u8 _madctl_base_mode{0};///< Base MADCTL value

    // Low-level communication

    auto send_buffer(BytesView buffer) noexcept -> internal::SpiWriteResult {
        _gpio_data_command.level(true);
        return _spi_node.write_buffer(buffer);
    }

    auto send_packet(trivial auto const &packet) noexcept -> internal::SpiWriteResult {
        _gpio_data_command.level(true);
        return _spi_node.write_packet(packet);
    }

    auto send_command(Command c) noexcept -> internal::SpiWriteResult {
        _gpio_data_command.level(false);
        return _spi_node.write_byte(static_cast<u8>(c));
    }

    KF_IMPL_DISPLAY_DRIVER(Self, internal::ST7735Image, internal::SpiWriteResult);

    auto init_impl() noexcept -> internal::SpiWriteResult {
        _spi_node.init();
        _gpio_data_command.init();
        _gpio_hardware_reset.init();

        this->reset();

        KF_TRY(send_command(Command::SWRESET));
        rtos::Task::sleep(150);

        KF_TRY(send_command(Command::SLPOUT));
        rtos::Task::sleep(255);

        KF_TRY(send_command(Command::COLMOD));

        constexpr u8 color_mode{0x05};// 16-bit color (RGB565)
        KF_TRY(send_packet(color_mode));

        KF_TRY(this->orientation(this->config().init_orientation));
        KF_TRY(send_command(Command::DISPON));

        rtos::Task::sleep(100);

        return ok();
    }

    void deinit_impl() noexcept {
        (void) send_command(Command::DISPOFF);
        _spi_node.deinit();
        _gpio_data_command.deinit();
        _gpio_hardware_reset.deinit();
    }

    void reset_impl() const noexcept {
        // Required after power‑up to initialise the internal state machine.
        _gpio_hardware_reset.level(false);
        rtos::Task::sleep(10);
        _gpio_hardware_reset.level(true);
        rtos::Task::sleep(120);
    }

    auto send_impl() noexcept -> internal::SpiWriteResult {
        KF_TRY(send_command(Command::RAMWR));
        return send_buffer({reinterpret_cast<u8 const *>(this->image().buffer().data()), this->image().size()});
    }

    auto set_orientation_impl(Orientation orientation) noexcept -> internal::SpiWriteResult {
        // full 6-way support
        constexpr u8 orient_to_transform[]{
            0,                                        // Orientation::Normal
            MadCtl::MirrorX,                          // Orientation::MirrorX
            MadCtl::MirrorY,                          // Orientation::MirrorY
            MadCtl::MirrorX | MadCtl::MirrorY,        // Orientation::Flip
            MadCtl::MirrorX | MadCtl::MirrorTranspose,// Orientation::ClockWise
            MadCtl::MirrorY | MadCtl::MirrorTranspose,// Orientation::CounterClockWise
        };

        u8 const madctl = _madctl_base_mode | orient_to_transform[static_cast<u8>(orientation)];

        this->image().transposed((madctl & MadCtl::MirrorTranspose) != 0);

        u8 const data_x[4]{0, 0, 0, static_cast<u8>(this->image().max_x())};
        u8 const data_y[4]{0, 0, 0, static_cast<u8>(this->image().max_y())};

        KF_TRY(send_command(Command::MADCTL));
        KF_TRY(send_packet(madctl));
        KF_TRY(send_command(Command::CASET));
        KF_TRY(send_packet(data_x));
        KF_TRY(send_command(Command::RASET));
        KF_TRY(send_packet(data_y));
        return ok();
    }
};

}// namespace kf::driver::display