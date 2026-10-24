// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

#pragma once

#include "kf/core.hpp"
#include "kf/units.hpp"

#include "kf/tuner/Tuner.hpp"

namespace kf::tuner {

/// @brief CRTP base class for tuners that collect a fixed number of samples before calculating.
/// @tparam Impl Derived class implementing the actual tuning logic.
/// @tparam T    Configuration type that will be updated after calculation.
/// @note The derived class must provide:
///
///       - `void reset_impl() noexcept`   – called when collection begins.
///
///       - `void poll_impl(kf::units::Milliseconds now) noexcept`    – called for each sample; should process the current sample (e.g., update min/max/sum).
///
///       - `void calculate_impl(T&) noexcept` – called after all samples are collected; updates the configuration.
///
/// @note This class manages a state machine (Idle -> Running -> Calculating -> Idle).
///          After `reset()`, the caller must repeatedly call `poll()` until `running()` returns false.
///          The `poll()` implementation of this class will invoke the derived class's `poll_impl()`
///          for each sample and, once the required number of samples is reached, transition to
///          the Calculating state and call `calculate_impl()`.
template<typename Impl, typename T> struct SampleCollectingTuner : Tuner<SampleCollectingTuner<Impl, T>> {
    using ConfigType = T;

    enum class State : u8 {
        Idle,
        Running,
        Calculating,
    };

    /// @brief Total number of samples to collect before calculating.
    usize const samples_total;

    explicit constexpr SampleCollectingTuner(ConfigType &config, usize samples) noexcept :
        _config{config}, samples_total{samples} {}

private:
    ConfigType &_config;
    usize _samples_processed{0};
    State _state{State::Idle};

    KF_IMPL_TUNER(SampleCollectingTuner<Impl, T>);

    constexpr void reset_impl() noexcept {
        _state = State::Running;
        _samples_processed = 0;
        impl().reset_impl();
    }

    void poll_impl(kf::units::Milliseconds now) noexcept {
        switch (_state) {
            case State::Idle://
                return;

            case State::Running: {
                impl().poll_impl(now);
                _samples_processed += 1;

                if (_samples_processed >= samples_total) {
                    _state = State::Calculating;
                }
            }
                return;

            case State::Calculating: {
                impl().calculate_impl(_config);
                _state = State::Idle;
            }
                return;
        }
    }

    [[nodiscard]] bool running_impl() const noexcept {
        return _state != State::Idle;
    }

    // CRTP
    Impl &impl() noexcept {
        return *static_cast<Impl *>(this);
    }

    Impl const &impl() const noexcept {
        return *static_cast<Impl const *>(this);
    }
};

}// namespace kf::tuner

#define KF_IMPL_SAMPLE_COLLECTING_TUNER(__impl__, ...)                       \
    friend struct ::kf::tuner::SampleCollectingTuner<__impl__, __VA_ARGS__>; \
    KF_IMPL_RESETTABLE(__impl__);                                            \
    KF_IMPL_POLL(__impl__)
