// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/ST7735.hpp
/// @brief   ST7735 TFT driver (128×160, SPI, RGB565).

#pragma once

#include "kf/BytesView.hpp"
#include "kf/Pool.hpp"
#include "kf/Result.hpp"
#include "kf/core.hpp"
#include "kf/gpio.hpp"
#include "kf/image/ViewportImage.hpp"
#include "kf/pixel/Rgb565Pixel.hpp"
#include "kf/rtos/Task.hpp"

#include "kf/drv/Display.hpp"
#include "kf/drv/SPI.hpp"

namespace kf::internal {

struct ST7735SpecificConfig {
    drv::SPI::Traits::Node::Config spi_node;
    DisplayOrientation init_orientation;
    gpio::GpioNumber gpio_num_data_command, gpio_num_reset;
};

struct ST7735Extra {
    drv::SPI &spi_bus;
    // gpio_pool;
};

template<typename Impl> using ST7735Base = drv::Display<
    Impl,
    drv::DisplayTraits<
        image::ViewportImage<pixel::Rgb565Pixel, 128, 160>,
        ST7735SpecificConfig,
        ST7735Extra,
        drv::SPI::Traits::Error,
        kf::Pool<gpio::DigitalOutput> &>>;

}// namespace kf::internal

namespace kf::drv {

struct ST7735 final : internal::ST7735Base<ST7735> {

    enum MadCtl : u8 {
        RgbMode = 0x00,
        BgrMode = 0x08,
        MirrorTranspose = 0x20,
        MirrorX = 0x40,
        MirrorY = 0x80,
    };

    enum struct Command : u8 {
        SWRESET = 0x01,
        SLPIN = 0x10,
        SLPOUT = 0x11,
        INVOFF = 0x20,
        INVON = 0x21,
        DISPOFF = 0x28,
        DISPON = 0x29,
        CASET = 0x2A,
        RASET = 0x2B,
        RAMWR = 0x2C,
        MADCTL = 0x36,
        COLMOD = 0x3A,
    };

    explicit ST7735(Dependencies const &deps) noexcept :
        internal::ST7735Base<ST7735>{deps},
        _spi_node{deps.extra.spi_bus, this->_config.spi_node} {}

private:
    drv::SPI::Traits::Node _spi_node;
    gpio::DigitalOutput *_dc{}, *_rst{};

    auto send_buffer(BytesView buffer) noexcept -> Traits::ActionResult {
        _dc->level(true);
        return _spi_node.write_buffer(buffer);
    }

    auto send_packet(trivial auto const &packet) noexcept -> Traits::ActionResult {
        _dc->level(true);
        return _spi_node.write_packet(packet);
    }

    auto send_command(Command c) noexcept -> Traits::ActionResult {
        _dc->level(false);
        return _spi_node.write_byte(static_cast<u8>(c));
    }

    KF_IMPL_DISPLAY(ST7735);

    auto init_impl(Traits::Resource &gpio_pool) noexcept -> Traits::ActionResult {
        kf_errdefer(
            gpio::DigitalOutput::destroy(_rst, gpio_pool);
            gpio::DigitalOutput::destroy(_dc, gpio_pool););

        auto dc_result = gpio::DigitalOutput::create(gpio_pool, this->_config.gpio_num_data_command);
        if (dc_result.is_error()) {
            return error(Traits::Error::NoSlot);
        }
        _dc = &dc_result.ok();

        auto rst_result = gpio::DigitalOutput::create(gpio_pool, this->_config.gpio_num_reset);
        if (rst_result.is_error()) {
            return error(Traits::Error::NoSlot);
        }
        _rst = &rst_result.ok();

        _rst->level(false);
        rtos::Task::sleep(10);
        _rst->level(true);
        rtos::Task::sleep(120);

        kf_try(send_command(Command::SWRESET));
        rtos::Task::sleep(150);
        kf_try(send_command(Command::SLPOUT));
        rtos::Task::sleep(255);
        kf_try(send_command(Command::COLMOD));
        kf_try(send_packet(static_cast<u8>(0x05)));
        kf_try(this->orientation(this->_config.init_orientation));
        kf_try(send_command(Command::DISPON));
        rtos::Task::sleep(100);

        kf_errdefer_cancel();
        return ok();
    }

    void deinit_impl(Traits::Resource &gpio_pool) noexcept {
        (void) send_command(Command::DISPOFF);
        _dc->destroy(gpio_pool);
        _rst->destroy(gpio_pool);
    }

    auto send_impl() noexcept -> Traits::ActionResult {
        kf_try(send_command(Command::RAMWR));
        return send_buffer({
            reinterpret_cast<u8 const *>(this->image().buffer().data()),
            this->image().size(),
        });
    }

    auto set_orientation_impl(Traits::Orientation orientation) noexcept -> Traits::ActionResult {
        constexpr u8 orient_to_transform[]{
            0,                                        // Orientation::Normal
            MadCtl::MirrorX,                          // Orientation::MirrorX
            MadCtl::MirrorY,                          // Orientation::MirrorY
            MadCtl::MirrorX | MadCtl::MirrorY,        // Orientation::Flip
            MadCtl::MirrorX | MadCtl::MirrorTranspose,// Orientation::ClockWise
            MadCtl::MirrorY | MadCtl::MirrorTranspose,// Orientation::CounterClockWise
        };

        u8 const madctl = orient_to_transform[static_cast<u8>(orientation)];

        this->image().transposed((madctl & MadCtl::MirrorTranspose) != 0);

        u8 const data_x[4]{0, 0, 0, static_cast<u8>(this->image().max_x())};
        u8 const data_y[4]{0, 0, 0, static_cast<u8>(this->image().max_y())};

        kf_try(send_command(Command::MADCTL));
        kf_try(send_packet(madctl));
        kf_try(send_command(Command::CASET));
        kf_try(send_packet(data_x));
        kf_try(send_command(Command::RASET));
        kf_try(send_packet(data_y));
        return ok();
    }
};

}// namespace kf::drv
