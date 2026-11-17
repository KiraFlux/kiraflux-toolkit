// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'core/pool'

// Pool<T> is a fixed-capacity slot allocator over caller-provided memory.
// It is backed by two parallel slices:
//   - `slots` (Slice<T>)     — raw, uninitialized storage for T
//   - `used`  (Slice<bool>)  — per-slot occupancy flags
//
//   alloc(args...)   constructs T in a free slot → Option<T&>  (or `none` if full)
//   free(obj)        runs ~T() and releases the slot; the slot is reusable
//   contains(ptr)    checks whether `ptr` belongs to a *live* slot of this pool
//   reset()          destroys every live T and frees all slots
//
// Slot reuse is hinted by `_last_allocated` (first slot that *might* be free),
// so freeing the second half and allocating again lands in the freed region.

#include <kf/Array.hpp>
#include <kf/Option.hpp>
#include <kf/Pool.hpp>
#include <kf/Slice.hpp>
#include <kf/main.hpp>

#include <kf/mixin/ReprTo.hpp>

// --- Payload --------------------------------------------------------------
// Trivial type with a readable repr(), so logs are self-explanatory.
struct Thing : kf::mixin::ReprTo<Thing> {

    kf::usize id;

private:
    KF_IMPL_REPR_TO(Thing);
    constexpr kf::usize repr_to_impl(auto &writable) const noexcept {
        return writable.append_format("Thing@{}", id);
    }
};

// --- main -----------------------------------------------------------------

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: core/pool");

    constexpr kf::usize n = 10;

    // --- Storage: two parallel slices carved from the application arena ---
    auto const slots = init.arena.allocate<Thing>(n);
    auto const used = init.arena.allocate<bool>(n);

    kf::Pool<Thing> pool{slots, used};

    // --- Bookkeeping: pointers to currently-live Things, in allocation order ---
    kf::Array<Thing *, n> alive{};
    kf::usize alive_count = 0;

    auto const dump = [&]() {
        init.logger.debug("pool: capacity={} alive={} used={}",
                          pool.capacity(), alive_count, used);
    };

    dump();

    // --- Allocate n items; the extra 2 attempts must fail ---
    for (kf::usize i = 0; i < (n + 2); i += 1) {
        if (auto maybe = pool.alloc(Thing{.id = i}); maybe.is_some()) {
            alive[alive_count] = &maybe.unwrap();
            alive_count += 1;
            init.logger.debug("alloc({}) -> {}", i, maybe.unwrap());
        } else {
            init.logger.warn("alloc({}) -> pool is full ({} slots)", i, n);
        }
    }

    dump();

    // --- contains(): live slot vs. freed slot ---
    auto const *live = alive[0];
    auto const *freed = alive[n / 2];
    init.logger.debug("contains(live[0]) = {}", pool.contains(live));

    // --- Free the second half: indices [n/2 .. n) ---
    for (kf::usize i = n / 2; i < n; i += 1) {
        pool.free(*alive[i]);
        alive[i] = nullptr;
    }
    alive_count = n / 2;

    init.logger.debug("contains(freed[{}]) = {}", n / 2, pool.contains(freed));
    dump();

    // --- Reuse: next alloc must land in a freed slot (hint = _last_allocated) ---
    if (auto maybe = pool.alloc(Thing{.id = 999}); maybe.is_some()) {
        init.logger.info("alloc(999) reused a freed slot -> {}", maybe.unwrap());
        pool.free(maybe.unwrap());
    }

    // --- Reset: destroy every live T, free every slot in one pass ---
    pool.reset();
    alive_count = 0;

    dump();
}
