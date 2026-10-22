# Contributing to KiraFlux Toolkit

This document defines code style, architectural conventions, and review process. For project overview, module list, build instructions and CLI reference, see [README.md](README.md).

## Table of Contents
- [Contributing to KiraFlux Toolkit](#contributing-to-kiraflux-toolkit)
  - [Table of Contents](#table-of-contents)
  - [Core Principles](#core-principles)
  - [Repository Workflow](#repository-workflow)
    - [Branches](#branches)
    - [Issues and Milestones](#issues-and-milestones)
    - [Pull Requests](#pull-requests)
    - [Syncing with `dev`](#syncing-with-dev)
  - [Structure and Naming](#structure-and-naming)
    - [Folders - Namespaces](#folders---namespaces)
    - [Files](#files)
    - [Naming](#naming)
  - [Code Style](#code-style)
    - [Formatting](#formatting)
    - [Inheritance](#inheritance)
    - [Trailing Return Types](#trailing-return-types)
    - [Initialisation](#initialisation)
    - [Include Organization](#include-organization)
    - [Key Rules](#key-rules)
  - [Architectural Conventions](#architectural-conventions)
    - [CRTP](#crtp)
    - [Mixins](#mixins)
    - [Error Handling](#error-handling)
    - [Hardware Abstraction](#hardware-abstraction)
    - [Traits](#traits)
  - [Testing](#testing)
  - [Versioning](#versioning)
  - [Review Process](#review-process)
  - [AI-Generated Code Policy](#ai-generated-code-policy)
  - [Review Checklist](#review-checklist)

---

## Core Principles

- **Header-only, C++20, ESP32 (Arduino)** - no `.cpp` except tests/examples; algorithmic code compiles natively.
- **CRTP over virtual** - virtual only for UI widgets and type-erased callables.
- **No exceptions** - errors via `bool`, `Option<T>`, or `Result<T, E>` (see [Error Handling](#error-handling)).
- **No mutable global state** - pass dependencies explicitly. `mixin::Singleton` is allowed only for hardware-guaranteed singletons (e.g., `kf::esp::Espnow`); justify new uses in the PR.
- **No default constructors for resource-dependent types** - construct with all dependencies.
- **Single internal namespace** - `kf::internal` only; `kf::<module>::internal` is forbidden.
- **Minimal dependencies** - algorithmic code must not include `<Arduino.h>`; use portable abstractions (`kf::math`, `kf::clamp`, etc.).
- **Testability** - algorithmic code must be testable natively (x86) with Unity; hardware-dependent code isolated.
- **Performance** - prefer `constexpr`, `noexcept`, `[[nodiscard]]`, `alignas` for SBO.
- **SPDX headers** - every file starts with `SPDX-FileCopyrightText`, `SPDX-License-Identifier: GPL-3.0-or-later`, project URL. `./kf new` generates them; do not strip existing.
- **License** - GPL-3.0-or-later. See [LICENSE](LICENSE). No CLA: opening a PR licenses your contribution under the same terms.

---

## Repository Workflow

### Branches
- `main` - releases only; direct pushes forbidden.
- `dev` - development; all changes land here via **regular merge commits** (never squash).
- Feature branches - `feature/<name>` or `fix/<name>` from `dev`.

### Issues and Milestones
- Every task has an Issue; epics use label `epic` with linked sub-issues.
- Sub-issues become `ready` when mergeable.
- Milestones map to versions (e.g., `v0.3.5`).

### Pull Requests
1. Branch from latest `dev`.
2. Open PR targeting `dev`; list `Closes #...`.
3. `./kf test` and `./kf lint` locally; CI must pass.
4. Regular merge commit into `dev` (not squash).
5. Releases: PR `dev` -> `main`, bump version, tag.

### Syncing with `dev`
Before opening a PR, rebase onto the latest `dev`:

```bash
git fetch origin
git rebase origin/dev
```

Resolve conflicts locally. Force push is allowed only on feature branches, never on `main` or `dev`.

---

## Structure and Naming

### Folders - Namespaces
A header's namespace is its path relative to `src/kf/`, joined by `::` and prefixed with `kf`. Depth mirrors the folder structure exactly.

- `src/kf/bus/I2C.hpp` -> `kf::bus`
- `src/kf/driver/display/SSD1306.hpp` -> `kf::driver::display`
- `src/kf/mixin/Resettable.hpp` -> `kf::mixin`

Only `kf::internal` is allowed as an implementation-detail namespace.

### Files
- **One public type per file**; filename = type name. Helpers naturally belonging to the type stay in the same file.
- Platform-specific implementations get their own header (`ArduinoGPIO.hpp`).

### Naming

| Category                    | Convention               | Examples                    |
| --------------------------- | ------------------------ | --------------------------- |
| Types, template params      | `PascalCase`             | `Result`, `GpioNumber`      |
| Functions, methods          | `snake_case`             | `unwrap_or`, `read_buffer`  |
| Variables, fields           | `snake_case`             | `max_duty`, `write_count`   |
| Private / protected fields  | `_` + `snake_case`       | `_buffer`, `_active_page`   |
| Constants, macros           | `UPPER_SNAKE_CASE`       | `KF_TRY`, `KF_PLACE_IRAM`   |
| Concrete impl aliases       | `PascalCase` + `Impl`    | `RendererImpl`, `PixelImpl` |
| CRTP tag structs            | `PascalCase` + `Tag`     | `GpioTag`, `SensorTag`      |
| Config instances (examples) | `snake_case` + `_config` | `pwm_config`, `bus_config`  |
| CRTP impl methods           | `snake_case` + `_impl`   | `read_impl`, `reset_impl`   |

Naming is enforced by `.clang-tidy` (`readability-identifier-naming`). Exception lists (`MethodIgnoredRegexp`, `FunctionIgnoredRegexp`, `StructIgnoredRegexp`) live in `.clang-tidy` - do not work around them.

---

## Code Style

### Formatting
- `.clang-format` and `.clang-tidy` are authoritative.
- Before committing: `./kf format` and `./kf lint` (add `--all` for the whole tree).
- PRs are rejected by CI if `./kf lint --all` reports violations.

### Inheritance
Each base class on its own line, blank lines around the list:

```cpp
struct MyClass :

    Base1,
    Base2

{
    // ...
};
```

### Trailing Return Types
Use trailing return for complex return types:

```cpp
auto read_buffer_impl(Bytes buffer) noexcept -> Result<BytesView, Error>;
auto map(F &&f) const & noexcept -> Option<decltype(f(std::declval<T>()))>;
```

### Initialisation
Designated initialisers, one field per line. Hardware handles are constructor arguments, not config fields:

```cpp
gpio::PwmOutput::Config pwm_config{
    .frequency_hz = 30'000,
    .resolution_bits = 10,
};

gpio::PwmOutput pwm_output{pwm_config, gpio::G26};
```

### Include Organization
Alphabetic within each group; blank line between groups:

| #   | Group                                     |
| --- | ----------------------------------------- |
| 1   | Std (`<...>`)                             |
| 2   | Toolkit (`<kf/...>`)                      |
| 3   | Toolkit mixins (`<kf/mixin/...>`)         |
| 4   | Protect interface (`"..."`) - `.cpp` only |

`#pragma once` in all headers.

### Key Rules
- `[[nodiscard]]` where applicable.
- `constexpr` / `noexcept` whenever possible.
- No `using namespace` in header global scope; type aliases and `using std::...` inside `kf` only.
- `<...>` for std headers, `"..."` for own headers.
- Doxygen for public API; brief "why" comments; `kf::internal` is exempt from full docs.

---

## Architectural Conventions

### CRTP

```cpp
template<typename Impl> struct Base {
    void do_something() { static_cast<Impl *>(this)->do_something_impl(); }
    static void class_method() { Impl::class_method_impl(); }
};

struct MyBase : Base<MyBase> {
private:
    friend struct Base<MyBase>;
    void do_something_impl() { /* ... */ }
    static void class_method_impl() { /* ... */ }
};
```

Always define a tag struct (`MyBaseTag`) for interface checking. Impl methods use the `_impl` suffix, so the public method and its impl do not collide.

### Mixins

CRTP mixins in `kf/mixin/`. Each provides a `KF_IMPL_*` macro; place it in the `private` section of the derived class.

| Group                 | Mixins                                                                                        | What it adds                                      |
| --------------------- | --------------------------------------------------------------------------------------------- | ------------------------------------------------- |
| Type contract         | `NonCopyable`, `Initable`                                                                     | Copy deletion, `init()`, `deinit()`               |
| Type state            | `Resettable`, `DefaultResettable`, `Invariant`                                                | `reset()`, Some/None-style checks                 |
| Scheduling            | `Poll`, `Flush`, `Drain`                                                                      | `poll(now)`, `flush()`, `drain()`                 |
| Injected dependencies | `Configured`, `Callbacked<Args...>`, `ValueCallbacked<T>`                                     | Config reference, optional callbacks              |
| Presentation          | `Labeled`, `Styled`, `Representable`, `ReprTo`                                                | Label, style, `repr()` / `repr_to()`              |
| Container interface   | `Equatable`, `Indexable`, `Length`, `Match`                                                   | `==`, `[]`, `length()`, key matching              |
| Data streams          | `Readable`, `Writable`, `ReadAvailable`, `WriteAvailable`, `BinaryReadable`, `BinaryWritable` | Text and binary I/O with availability             |
| Object identity       | `Singleton`, `MacAddressed`                                                                   | Meyer's singleton access, owned MAC address       |
| Arena                 | `ExtraAllocationLength`                                                                       | Extra allocation length for arena-created objects |

Do not duplicate mixin functionality in your own types.

### Error Handling

| Mechanism      | When                              | Example            |
| -------------- | --------------------------------- | ------------------ |
| `bool`         | Success/failure with no context   | `send_buffer(buf)` |
| `Option<T>`    | One obvious failure reason        | `parse_int(str)`   |
| `Result<T, E>` | Multiple distinguishable failures | `create_node(...)` |

Rules:
- Never throw. No `std::optional` / `std::variant` / `std::expected`.
- Always check return values.
- Prefer `KF_TRY` for early propagation.
- `kf::some()` / `kf::none` for `Option`; `kf::ok()` / `kf::error()` for `Result`.
- `unwrap()` is safe when a checked invariant guarantees success:

  ```cpp
  while (queue.available_for_read() > 0) {
      char c = queue.read().unwrap();  // safe
  }
  ```

### Hardware Abstraction
- Guard hardware code with `#ifdef ARDUINO` in the same header.
- Algorithmic code must not include `<Arduino.h>`.
- ISRs: `IRAM_ATTR` (via `KF_PLACE_IRAM`); shared vars `volatile`.

### Traits
- `static_assert` for type properties.
- Concepts from `kf/core.hpp`: `implements<Impl, Tag>`, `arithmetic<T>`, `trivial<T>`, `callable<F, Sig>`, `c_string<T>`, `type_like<T, U>`, `integer_type<T>`, `float_type<T>`, `enum_type<T>`.

---

## Testing

- Native unit tests in `test/unit/` (Unity), run via `./kf test`.
- Hardware tests not yet implemented (`test/hardware/` planned).
- Every new algorithmic module needs tests; new examples must build (`./kf build --all`).
- Before opening a PR: `./kf lint` and `./kf format` must pass locally.

---

## Versioning

- **0.x** - MINOR may include breaking changes, PATCH for fixes.
- Version lives in `library.json` and must stay in sync with:
  - `src/kf/main.hpp` - startup banner (`"KiraFlux Toolkit vX.Y.Z"`).
  - `README.md` - `lib_deps = ...#X.Y.Z`.

Release sequence: bump all three -> update `CHANGELOG.md` -> PR `dev` -> `main` -> tag `vX.Y.Z`.

---

## Review Process

When commenting on a violation, reference the relevant section (concise and actionable):

> **Error Handling** - use `bool`; only one failure reason here.
> **Include Organization** - Toolkit group before Protect interface.
> **Structure and Naming** - `kf::foo::internal` forbidden; use `kf::internal`.
> **Structure and Naming** - method should be `snake_case`; `.clang-tidy` will flag it.

If the issue is not covered by this document: explain it fully in the comment, then propose a doc update (Issue or PR).

---

## AI-Generated Code Policy

1. **You are responsible** for correctness and compliance of AI-generated code.
2. Ensure it follows this document and passes tests.
3. **Attribution**:
   - Do not use another contributor's name for AI-generated code.
   - Use your own name, or omit authorship.
   - Mark generation in a comment.
4. Flag AI-generated files in the PR description.
5. GPL-3.0-or-later applies; SPDX headers must be correct.

Recommended header:

```cpp
// Copyright (c) 2026 <Your Name>
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit
//
// FIXME: This file was generated via <Tool Name>.
// TODO: REFACTOR WANTED
```

---

## Review Checklist

- [ ] `./kf build --all` passes.
- [ ] `./kf test` passes; new algorithmic code is covered.
- [ ] `./kf lint` (and `./kf lint --all` for cross-file changes) reports no violations.
- [ ] `./kf format` reports no diffs.
- [ ] No mutable global state; `mixin::Singleton` justified.
- [ ] Correct error mechanism used (`bool` / `Option` / `Result`).
- [ ] Mixins used where applicable.
- [ ] Namespaces mirror folders; no `kf::<module>::internal`.
- [ ] Filename matches primary type.
- [ ] Naming follows the rules.
- [ ] CRTP impl methods use the `_impl` suffix.
- [ ] Include order follows the 4-group rule.
- [ ] Public API has Doxygen; `kf::internal` exempt.
- [ ] SPDX headers present and correct.
- [ ] AI-generated code attributed and flagged.
- [ ] PR links issues (`Closes #...`).

---

Thank you for contributing to KiraFlux Toolkit!
