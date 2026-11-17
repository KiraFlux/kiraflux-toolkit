// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'core/result'

// Comprehensive kf::Result<T, E> tour:
//   creation:       kf::ok(v), kf::error(e), kf::ok_ref(ref), kf::ok()
//   queries:        is_ok(), is_error()
//   extraction:     ok(), error()
//   transforms:     map(f), map_error(f)
//   reference:      Result<T&, E>
//   void:           Result<void, E>
//   propagation:    kf_try, kf_try_map_err
//   stringification: mixin::ReprTo (built into Result)

#include <kf/Result.hpp>
#include <kf/core.hpp>
#include <kf/main.hpp>

// --- Error type -----------------------------------------------------------

enum struct MathError : kf::u8 {
    ZeroDivision,
};

[[nodiscard]] constexpr kf::StringView repr(MathError e) noexcept {
    switch (e) {
        case MathError::ZeroDivision: return "MathError::ZeroDivision";
    }
    return "MathError::?";
}

using MathResult = kf::Result<int, MathError>;

// --- Helpers used by the demos --------------------------------------------

[[nodiscard]] constexpr MathResult safe_div(int a, int b) noexcept {
    if (b == 0) { return kf::error(MathError::ZeroDivision); }
    return kf::ok(a / b);
}

[[nodiscard]] constexpr kf::Result<int &, MathError> find_positive(int &a, int &b) noexcept {
    if (a > 0) { return kf::ok_ref(a); }
    if (b > 0) { return kf::ok_ref(b); }
    return kf::error(MathError::ZeroDivision);
}

// --- kf_try: propagate the first error to the caller ----------------------

[[nodiscard]] MathResult div_twice(int a, int b, int c) noexcept {
    auto r = kf_try(safe_div(a, b));
    auto r2 = kf_try(safe_div(r, c));
    return kf::ok(r2);
}

// --- kf_try_map_err: propagate, replacing the error value -----------------

enum struct PipelineError : kf::u8 {
    ComputeFailed,
};

[[nodiscard]] constexpr kf::StringView repr(PipelineError e) noexcept {
    switch (e) {
        case PipelineError::ComputeFailed: return "PipelineError::ComputeFailed";
    }
    return "PipelineError::?";
}

[[nodiscard]] kf::Result<int, PipelineError> pipeline_div(int a, int b) noexcept {
    auto r = kf_try_map_err(safe_div(a, b), PipelineError::ComputeFailed);
    return kf::ok(r);
}

// --- main -----------------------------------------------------------------

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: core/result");

    // 1. Creation, queries, extraction
    {
        MathResult ok_r = kf::ok(42);
        MathResult err_r = kf::error(MathError::ZeroDivision);

        init.logger.info("[basic] ok:  is_ok={} is_error={} value={}",
                         ok_r.is_ok(), ok_r.is_error(), ok_r.ok());

        init.logger.info("[basic] err: is_ok={} is_error={} error={}",
                         err_r.is_ok(), err_r.is_error(), err_r.error());

        // NOTE: ok() / error() abort when called on the wrong variant.
        // Always guard with is_ok() / is_error() in production code.
    }

    // 2. map / map_error
    {
        MathResult ok_r = kf::ok(10);
        MathResult err_r = kf::error(MathError::ZeroDivision);

        auto doubled = ok_r.map([](int x) { return x * 2; });
        init.logger.info("[map] ok(10).map(*2) = {}", doubled.ok());

        auto still_err = err_r.map([](int x) { return x * 2; });
        init.logger.info("[map] error.map(*2).is_error = {}", still_err.is_error());

        // void-returning map: side effect only, result becomes Result<void, E>
        auto side = ok_r.map([&init](int x) { init.logger.debug("  side effect: {}", x); });
        init.logger.info("[map] side.is_ok = {}", side.is_ok());

        // map_error: transform E into a different error type
        auto remapped = err_r.map_error([](MathError) { return 404; });
        init.logger.info("[map_error] error.map_error(->404) = {}", remapped.error());
    }

    // 3. Result<T&, E>: reference success value
    {
        int x = -1, y = 7;
        auto r = find_positive(x, y);
        init.logger.info("[ref] find_positive(-1, 7).is_ok = {}", r.is_ok());
        r.ok() = 100;// mutate through the reference
        init.logger.info("[ref] via ref: y = {}", y);

        x = -5;
        y = -6;
        init.logger.info("[ref] find_positive(-5, -6).is_error = {}",
                         find_positive(x, y).is_error());
    }

    // 4. kf_try: propagate the first error, unwrap successes
    {
        init.logger.info("[kf_try] div_twice(20, 4, 5).ok = {}",
                         div_twice(20, 4, 5).ok());
        init.logger.info("[kf_try] div_twice(20, 0, 5).is_error = {}",
                         div_twice(20, 0, 5).is_error());
        init.logger.info("[kf_try] div_twice(20, 4, 0).is_error = {}",
                         div_twice(20, 4, 0).is_error());
    }

    // 5. kf_try_map_err: propagate and remap the error
    {
        init.logger.info("[kf_try_map_err] pipeline_div(10, 2).ok = {}",
                         pipeline_div(10, 2).ok());

        auto err = pipeline_div(10, 0);
        init.logger.info("[kf_try_map_err] pipeline_div(10, 0): is_error = {} error = {}",
                         err.is_error(), err.error());
    }

    // 6. ReprTo stringification (built into Result)
    {
        init.logger.info("[repr] ok:    {}", MathResult{kf::ok(42)});
        init.logger.info("[repr] error: {}", MathResult{kf::error(MathError::ZeroDivision)});
        init.logger.info("[repr] void:  {}", kf::Result<void, MathError>{kf::ok()});
    }
}
