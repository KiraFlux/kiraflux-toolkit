// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

// KiraFlux-Toolkit Example 'core/stack'

// Demonstrates LIFO container (Stack) with fixed-size buffer.
// Shows push, pop, iteration, and availability checks.

#include <kf/Stack.hpp>
#include <kf/main.hpp>

void kf::main(kf::Init &init) {
    init.logger.info("KiraFlux-Toolkit Example: core/stack");

    // --- Stack overview ---
    // Stack<T> is a non-owning LIFO container that operates on a fixed-size buffer.
    // It inherits from Sequence, so it supports iteration, indexing, and slicing.
    // Memory must be provided by the user (e.g., a local array or kf::Array).

    constexpr usize n = 8;
    auto buffer = init.arena.allocate<int>(n);

    // Construct an empty stack using the entire buffer.
    // Optionally, you can specify an initial length if the buffer already contains data.
    kf::Stack<int> my_stack{buffer};// equivalent to Stack{buffer, 0}

    // --- Error handling: reading from an empty stack returns none ---
    if (auto const value = my_stack.read(); value.is_none()) {
        init.logger.error("read from empty stack - returns None");
    }

    // --- Writing (push) ---
    // Stack::write() adds an element to the top. Returns true if successful,
    // false if the stack is full (capacity reached).

    for (int i = 0; i < (n + 2); i += 1) {
        if (my_stack.write(i)) {
            init.logger.debug("write: {} -> ok", i);
        } else {
            init.logger.warn("write: {} -> full, ignored", i);
        }
    }

    // Only the first n elements (0..7) were stored.
    // The remaining (8,9) were rejected.

    // --- Availability ---
    // available_for_read() returns the number of elements currently in the stack.
    // available_for_write() returns the number of free slots (capacity - length).
    init.logger.info("available_for_read: {} (elements in stack)", my_stack.available_for_read());
    init.logger.info("available_for_write: {} (free slots)", my_stack.available_for_write());

    // --- Accessing elements ---
    // top() returns Option<T&> - mutable reference to the top element.
    if (auto top = my_stack.top(); top.is_some()) {
        init.logger.info("top element: {}", top.unwrap());
    }

    // --- Iteration (inherited from Sequence) ---
    // Stack provides begin()/end() and supports range-based for.
    // Iteration order is from bottom (oldest) to top (newest).

    init.logger.info("stack contents (bottom to top):");
    for (int const &value: my_stack) {
        init.logger.debug("  {}", value);
    }

    // --- Reading (pop) ---
    // read() removes and returns the top element as Option<T>.
    // Returns None if the stack is empty.

    init.logger.info("popping elements:");
    for (int i = 0; i < (n + 2); i += 1) {
        if (auto value = my_stack.read(); value.is_some()) {
            init.logger.debug("read: {} -> ok", value.unwrap());
            // Show updated availability after each pop.
            init.logger.debug("  remaining: {}, free: {}", my_stack.available_for_read(), my_stack.available_for_write());
        } else {
            init.logger.warn("read: {} -> empty, stopped", i);
            break;
        }
    }

    // After reading all elements, the stack is empty again.

    // --- Other utility methods ---
    init.logger.info("empty: {}", my_stack.empty());      // true
    init.logger.info("length: {}", my_stack.length());    // 0
    init.logger.info("capacity: {}", my_stack.capacity());// n

    // Reset discards all elements (length becomes 0).
    my_stack.reset();

    // Data pointer and slice access.
    int *raw = my_stack.data();                     // pointer to underlying buffer
    auto slice = my_stack.slice();                  // Slice<int> view of the stack content
    Slice<int const> const_slice = my_stack.slice();// const version

    // Indexing (inherited from Sequence)
    // my_stack[0] - bottom element (if length > 0)

    // --- Performance note ---
    // All operations are O(1). No dynamic allocations.
    // Stack is suitable for real-time / embedded contexts.
}