// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/SSD1306.hpp
/// @brief   SSD1306 OLED driver (128×64, I2C, monochrome).

#pragma once

#include "kf/Result.hpp"
#include "kf/core.hpp"
#include "kf/drv/I2C.hpp"
#include "kf/image/StaticImage.hpp"
#include "kf/math.hpp"
#include "kf/pixel/MonochromePixel.hpp"

#include "kf/drv/Display.hpp"

namespace kf::internal {

struct SSD1306CommonConfig {
    static constexpr u8 default_chunk_size{64};

    u8 chunk_size{default_chunk_size};
};

struct SSD1306SpecificConfig {
    static constexpr u8 default_address{0x3C};

    drv::I2C::Traits::Node::Config i2c_node{
        .address{default_address},
    };
};

struct SSD1306Extra {
    drv::I2C &i2c_bus;
};

template<typename Impl> using SSD1306Base = drv::Display<
    Impl,
    drv::DisplayTraits<
        image::StaticImage<pixel::MonochromePixel, 128, 64>,
        drv::CommonSpecific<SSD1306CommonConfig, SSD1306SpecificConfig>,
        SSD1306Extra,
        drv::I2C::Traits::Error>>;

}// namespace kf::internal

namespace kf::drv {

/// @brief SSD1306 OLED display driver for 128x64 monochrome panels
struct SSD1306 : internal::SSD1306Base<SSD1306> {

    /// @brief SSD1306 command set
    enum Command : u8 {
        DisplayOff = 0xAE,///< Turn display off
        DisplayOn = 0xAF, ///< Turn display on

        CommandMode = 0x00,   ///< Start command stream
        OneCommandMode = 0x80,///< Single command prefix
        DataMode = 0x40,      ///< Data transmission prefix

        AddressingMode = 0x20,///< Set addressing mode
        Horizontal = 0x00,    ///< Horizontal addressing
        Vertical = 0x01,      ///< Vertical addressing

        NormalV = 0xC8,///< Normal vertical scan direction
        FlipV = 0xC0,  ///< Flipped vertical scan direction
        NormalH = 0xA1,///< Normal horizontal segment remap
        FlipH = 0xA0,  ///< Flipped horizontal segment remap

        Contrast = 0x81,     ///< Set contrast command
        SetComPins = 0xDA,   ///< COM pins hardware configuration
        SetVcomDetect = 0xDB,///< Set VCOMH deselect level
        ClockDiv = 0xD5,     ///< Set display clock divide ratio
        SetMultiplex = 0xA8, ///< Set multiplex ratio
        ColumnAddr = 0x21,   ///< Set column address range
        PageAddr = 0x22,     ///< Set page address range
        ChargePump = 0x8D,   ///< Charge pump setting

        NormalDisplay = 0xA6,///< Normal pixel color (black on white)
        InvertDisplay = 0xA7 ///< Inverted pixel color (white on black)
    };

    /// @brief Construct SSD1306 driver instance
    explicit constexpr SSD1306(Dependencies const &deps) noexcept :
        internal::SSD1306Base<SSD1306>{deps}, _i2c_node{deps.extra.i2c_bus, this->_config.specific.i2c_node} {}

    /// @brief Set display contrast level (0..255)
    [[nodiscard]] auto contrast(u8 value) noexcept -> Traits::ActionResult {
        u8 const packet[]{CommandMode, Contrast, value};
        return _i2c_node.write_packet(packet);
    }

    /// @brief Enable or disable display power
    [[nodiscard]] auto power(bool on) noexcept -> Traits::ActionResult {
        return send_command(on ? DisplayOn : DisplayOff);
    }

    /// @brief Invert display colors
    [[nodiscard]] auto invert(bool invert) noexcept -> Traits::ActionResult {
        return send_command(invert ? InvertDisplay : NormalDisplay);
    }

    [[nodiscard]] static constexpr bool support_orientation(Traits::Orientation orientation) noexcept {
        return orientation == Traits::Orientation::Normal or orientation == Traits::Orientation::MirrorX or orientation == Traits::Orientation::MirrorY;
    }

private:
    drv::I2C::Traits::Node _i2c_node;

    auto send_command(Command c) noexcept -> Traits::ActionResult {
        u8 const packet[]{
            OneCommandMode,
            static_cast<u8>(c),
        };

        return _i2c_node.write_buffer({packet});
    }

    KF_IMPL_DISPLAY(SSD1306);

    auto init_impl() noexcept -> Traits::ActionResult {
        static constexpr u8 init_commands[]{
            CommandMode,

            // Turn off for safe configuration
            DisplayOff,

            // Clock divider
            ClockDiv,
            0x80,

            // Enable internal charge pump
            ChargePump,
            0x14,

            // Horizontal addressing mode
            AddressingMode,
            Horizontal,

            // Default contrast 127
            Contrast,
            0x7F,

            // VCOM voltage
            SetVcomDetect,
            0x40,

            // Normal orientation
            NormalH,
            NormalV,

            // Turn display on
            DisplayOn,

            // Pin configuration (128x64)
            SetComPins,
            0x12,

            // Multiplex (64 lines)
            SetMultiplex,
            0x3F,
        };

        return _i2c_node.write_buffer({init_commands});
    }

    void deinit_impl() noexcept {
        (void) power(false);
    }

    auto send_impl() noexcept -> Traits::ActionResult {
        // Transfer software buffer to display via I2C

        static constexpr u8 set_area_commands[]{
            CommandMode,
            // Set full display window
            ColumnAddr,
            0,
            Traits::ImageImpl::comptime_width - 1,
            PageAddr,
            0,
            Traits::ImageImpl::PixelImpl::pages(Traits::ImageImpl::comptime_height) - 1,
        };

        kf_try(_i2c_node.write_buffer({set_area_commands}));

        auto p = this->image().buffer().data();
        auto remaining = this->image().buffer().length();

        while (remaining > 0) {
            auto const chunk = math::min(this->_config.common.chunk_size, remaining);

            kf_try(_i2c_node.write_mixed(Command::DataMode, {p, chunk}));

            p += chunk;
            remaining -= chunk;
        }

        return ok();
    }

    auto set_orientation_impl(Traits::Orientation orientation) noexcept -> Traits::ActionResult {
        if (not support_orientation(orientation)) {
            return ok();
        }

        constexpr auto flip_x = 0b01;
        constexpr auto flip_y = 0b10;
        auto const flags = static_cast<u8>(orientation);

        kf_try(send_command((flags & flip_x) ? FlipH : NormalH));
        kf_try(send_command((flags & flip_y) ? FlipV : NormalV));

        return ok();
    }
};

}// namespace kf::drv
