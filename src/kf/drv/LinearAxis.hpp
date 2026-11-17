// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/LinearAxis.hpp
/// @brief   Single analog axis with filtering, dead-zone, and min/max calibration.

#pragma once

#include "kf/Pool.hpp"
#include "kf/core.hpp"
#include "kf/filter/ExponentialFilter.hpp"
#include "kf/gpio.hpp"
#include "kf/math.hpp"
#include "kf/tuner/Tuner.hpp"
#include "kf/units.hpp"

#include "kf/drv/Sensor.hpp"

namespace kf::internal {

template<implements<filter::FilterTag> F> struct LinearAxisCommonConfig {
    typename F::Config filter;
};

template<integer_type RawValue> struct LinearAxisSpecificConfig {
    gpio::GpioNumber gpio_num;
    bool inverted, calibrated;
    RawValue center, dead_zone, range_positive, range_negative;
};

enum struct LinearAxisError {
    NoSlot,
    GpioCreateFailed,
};

template<typename F, typename R> struct LinearAxisTraits :

    drv::SensorTraits<
        f32,
        drv::CommonSpecific<LinearAxisCommonConfig<F>, LinearAxisSpecificConfig<R>>,
        NoneType,
        LinearAxisError,
        Pool<gpio::AdcInput> &>

{
    using Filter = F;
    using RawValue = R;
};

template<typename Impl> using LinearAxisBase = drv::Sensor<
    Impl,
    LinearAxisTraits<
        filter::ExponentialFilter<f32>,
        i16>>;

}// namespace kf::internal

namespace kf::drv {

/// @brief Single analog axis: ADC -> filter -> dead-zone -> [-1, +1].
struct LinearAxis : internal::LinearAxisBase<LinearAxis> {

    explicit LinearAxis(Dependencies const &deps) noexcept :
        internal::LinearAxisBase<LinearAxis>{deps}, _filter{deps.config.common.filter} {}

    [[nodiscard]] auto read_raw() const noexcept {
        return _adc_input->read();
    }

    /// @brief Calibration FSM: bring axis to min, confirm; bring to max, confirm.
    struct Tuner : tuner::Tuner<Tuner> {

        static constexpr usize default_samples{20}, min_span_divisor{4}, dead_zone_span_divisor{10};
        static constexpr Traits::RawValue dead_zone_margin{10};

        enum struct State {
            WaitStart,
            CollectingStart,
            WaitEnd,
            CollectingEnd,
            Done,
        };

        explicit constexpr Tuner(LinearAxis &axis, usize samples = default_samples) noexcept :
            _axis{axis}, _samples{samples} {}

        /// @brief Advance FSM. Ignored outside WaitStart/WaitEnd.
        void confirm() noexcept {
            _latest = static_cast<Traits::RawValue>(_axis.read_raw());

            switch (_state) {
                case State::WaitStart:
                    _captured_min = _latest;
                    _collected = 0;
                    _state = State::CollectingStart;
                    return;

                case State::WaitEnd:
                    _captured_max = _latest;
                    _collected = 0;
                    _state = State::CollectingEnd;
                    return;

                default:
                    return;
            }
        }

        [[nodiscard]] auto state() const noexcept -> State {
            return _state;
        }

        [[nodiscard]] auto current_raw() const noexcept -> Traits::RawValue {
            return _latest;
        }

        [[nodiscard]] auto calibrated() const noexcept -> bool {
            return _axis._config.specific.calibrated;
        }

    private:
        LinearAxis &_axis;
        usize _samples;
        State _state{State::Done};
        Traits::RawValue _latest{0}, _captured_min{0}, _captured_max{0};
        usize _collected{0};

        KF_IMPL_TUNER(Tuner);

        void reset_impl() noexcept {
            _state = State::WaitStart;
            _latest = 0;
            _captured_min = 0;
            _captured_max = 0;
            _collected = 0;
        }

        void poll_impl(units::Milliseconds now) noexcept {
            (void) now;

            _latest = static_cast<Traits::RawValue>(_axis.read_raw());

            switch (_state) {
                case State::CollectingStart:
                    _captured_min = math::min(_captured_min, _latest);
                    _collected += 1;
                    if (_collected >= _samples) {
                        _state = State::WaitEnd;
                    }
                    return;

                case State::CollectingEnd:
                    _captured_max = math::max(_captured_max, _latest);
                    _collected += 1;
                    if (_collected >= _samples) {
                        calculate();
                        _state = State::Done;
                    }
                    return;

                default:
                    return;
            }
        }

        bool running_impl() const noexcept {
            return _state != State::Done;
        }

        void calculate() noexcept {
            using R = Traits::RawValue;
            auto &c = _axis._config.specific;
            auto const span = static_cast<R>(_captured_max - _captured_min);
            auto const min_span = static_cast<R>(gpio::AdcInput::max_value() / min_span_divisor);

            if (span < min_span) {
                c.calibrated = false;
                return;
            }

            c.center = static_cast<R>(_captured_min + span / 2);
            c.range_positive = static_cast<R>(_captured_max - c.center);
            c.range_negative = static_cast<R>(c.center - _captured_min);
            c.dead_zone = static_cast<R>(span / dead_zone_span_divisor + dead_zone_margin);
            c.calibrated = true;
        }
    };

private:
    using G = gpio::AdcInput;

    Traits::Filter _filter;
    G *_adc_input{nullptr};

    KF_IMPL_SENSOR(LinearAxis);

    auto init_impl(Pool<G> &adc_pool) noexcept -> Traits::ActionResult {
        auto const result = G::create(adc_pool, this->_config.specific.gpio_num);
        if (result.is_error()) {
            return error(Traits::Error::GpioCreateFailed);
        }
        _adc_input = &result.ok();
        return ok();
    }

    void deinit_impl(Pool<G> &adc_pool) noexcept {
        _adc_input->destroy(adc_pool);
    }

    [[nodiscard]] auto read_impl() noexcept -> Traits::Measurement {
        auto const &c = this->_config.specific;

        if (not c.calibrated) {
            return 0;
        }

        auto const deviation = read_raw() - c.center;
        if (math::abs(deviation) < c.dead_zone) {
            return 0;
        }

        auto value = _filter.calc(static_cast<Traits::Measurement>(deviation));

        auto const range = static_cast<Traits::Measurement>(
            (value < 0) ? c.range_negative : c.range_positive);

        if (range <= 0) {
            return 0;
        }
        value /= range;

        if (c.inverted) {
            value = -value;
        }

        return math::clamp(value, -1.0f, +1.0f);
    }
};

}// namespace kf::drv
