# KiraFlux Toolkit

**Experimental header-only C++20 library for ESP32 (Arduino framework)**
A pragmatic blend of static polymorphism (CRTP) and dynamic polymorphism for embedded development.

## Overview

KiraFlux Toolkit provides modular components for ESP32, built around:

- **CRTP** — for zero-cost abstractions (GPIO, buses, drivers, filters, mixins).
- **Dynamic polymorphism** — where runtime flexibility is required (UI widgets, type-erased callables via `kf::Function`).

The library is **header-only**, **C++20**, targets **ESP32 + Arduino framework**. Many core modules are platform-agnostic and compile natively.

**Key principles:**
- No exceptions — error handling via `kf::Result` and `kf::Option` return values.
- `[[nodiscard]]`, `constexpr`, `noexcept` where applicable.
- Minimal hardware-specific dependencies; algorithmic code has no `<Arduino.h>`.
- Algorithmic code testable natively (x86) with Unity.

## Status

**Experimental** — APIs may change between minor versions. Feedback and contributions welcome.

## Modules

### Core
[`kf/`](src/kf/)

| Group      | Module                              | Description                                                                                 |
| ---------- | ----------------------------------- | ------------------------------------------------------------------------------------------- |
| Foundation | [Core](src/kf/core.hpp)             | Primitive types, `NoneType`, concepts, and type traits shared by the whole library          |
|            | [Units](src/kf/units.hpp)           | Physical units used across the library (time, angles, distances, pixels)                    |
|            | [Math](src/kf/math.hpp)             | Scalar helpers, linear interpolation, checksum, ranges, vectors, quaternions                |
| Errors     | [Option](src/kf/Option.hpp)         | Optional value type for expressing absence without pointers                                 |
|            | [Result](src/kf/Result.hpp)         | Value-or-error type for explicit error handling without exceptions                          |
| Memory     | [Arena](src/kf/Arena.hpp)           | Linear allocator for fast, deterministic allocation with O(1) reset                         |
| Containers | [Array](src/kf/Array.hpp)           | Fixed-size owning container with sequence iteration                                         |
|            | [Slice](src/kf/Slice.hpp)           | Non-owning view over contiguous memory, with const conversion                               |
|            | [Bytes](src/kf/Bytes.hpp)           | Mutable byte slice alias                                                                    |
|            | [BytesView](src/kf/BytesView.hpp)   | Read-only byte slice alias                                                                  |
|            | [Stack](src/kf/Stack.hpp)           | LIFO container over caller-provided memory                                                  |
|            | [Queue](src/kf/Queue.hpp)           | FIFO container over caller-provided memory                                                  |
|            | [Registry](src/kf/Registry.hpp)     | Arena-backed registry for storing and looking up objects                                    |
| Strings    | [String](src/kf/String.hpp)         | Buildable string with compile-time format validation                                        |
|            | [StringView](src/kf/StringView.hpp) | Non-owning string view with trim, search, and split                                         |
| Iteration  | [Sequence](src/kf/Sequence.hpp)     | CRTP base that gives containers iteration, indexing, and slicing                            |
|            | [Iterator](src/kf/Iterator.hpp)     | CRTP base for custom iterators                                                              |
| Callables  | [Function](src/kf/Function.hpp)     | Type-erased callable with small-buffer optimisation; never empty                            |
| Utilities  | [Timer](src/kf/Timer.hpp)           | Elapsed-time and expiration tracking                                                        |
|            | [Logger](src/kf/Logger.hpp)         | Formatted logging with level prefixes and optional buffering                                |
| Control    | [PID](src/kf/PID.hpp)               | PID controller with derivative filtering and integral anti-windup                           |
| I/O        | [GPIO](src/kf/gpio.hpp)             | Digital, analog, and PWM pin abstractions with Arduino backend                              |
|            | [UART](src/kf/UART.hpp)             | Buffered UART with the same binary I/O interface as other transports                        |
| Networking | [MacAddress](src/kf/MacAddress.hpp) | Six-byte MAC address wrapper                                                                |
| App        | [Main](src/kf/main.hpp)             | Application entry point that bundles arena, logger, and standard I/O into an `Init` context |

### Buses
[`kf/bus/`](src/kf/bus/)

| Module                    | Description                                                              |
| ------------------------- | ------------------------------------------------------------------------ |
| [Bus](src/kf/bus/Bus.hpp) | CRTP base for buses that hand out node objects                           |
| [I2C](src/kf/bus/I2C.hpp) | I2C bus and node abstraction with a typed error enum and Arduino backend |
| [SPI](src/kf/bus/SPI.hpp) | SPI bus and node abstraction with Arduino backend                        |

### Drivers
[`kf/driver/`](src/kf/driver/)

| Group    | Module                                                            | Description                                                                |
| -------- | ----------------------------------------------------------------- | -------------------------------------------------------------------------- |
| Core     | [Driver](src/kf/driver/Driver.hpp)                                | CRTP base for drivers with a uniform init step                             |
| Actuator | [ActuatorDriver](src/kf/driver/actuator/ActuatorDriver.hpp)       | CRTP base for actuators that set an output and stop                        |
|          | [DRV8871](src/kf/driver/actuator/DRV8871.hpp)                     | H-bridge motor driver with dead-zone compensation                          |
|          | [PwmPositionServo](src/kf/driver/actuator/PwmPositionServo.hpp)   | Servo driver that maps angle to PWM pulse width with a safe range          |
| Display  | [DisplayDriver](src/kf/driver/display/DisplayDriver.hpp)          | CRTP base for displays: framebuffer, orientation, and transfer to hardware |
|          | [Orientation](src/kf/driver/display/Orientation.hpp)              | Enumeration of display orientation modes                                   |
|          | [SSD1306](src/kf/driver/display/SSD1306.hpp)                      | Monochrome I2C OLED driver                                                 |
|          | [ST7735](src/kf/driver/display/ST7735.hpp)                        | RGB565 SPI TFT driver with all six orientations                            |
| Sensor   | [SensorDriver](src/kf/driver/sensor/SensorDriver.hpp)             | CRTP base for sensors that read a measurement                              |
|          | [Joystick](src/kf/driver/sensor/Joystick.hpp)                     | Two-axis analog joystick with calibration and unit-circle normalisation    |
|          | [NormalizedAdcInput](src/kf/driver/sensor/NormalizedAdcInput.hpp) | Single analog axis with dead-zone filtering and calibration                |
|          | [QuadratureEncoder](src/kf/driver/sensor/QuadratureEncoder.hpp)   | Quadrature encoder with 4× decoding and unit conversion                    |
|          | [Sharp](src/kf/driver/sensor/Sharp.hpp)                           | Analog Sharp IR distance sensor                                            |

### Filters
[`kf/filter/`](src/kf/filter/)

| Group     | Module                                                       | Description                                 |
| --------- | ------------------------------------------------------------ | ------------------------------------------- |
| Core      | [Filter](src/kf/filter/Filter.hpp)                           | CRTP base for single-sample filters         |
| Smoothing | [ExponentialFilter](src/kf/filter/ExponentialFilter.hpp)     | Exponential moving average for noisy inputs |
|           | [LowFrequencyFilter](src/kf/filter/LowFrequencyFilter.hpp)   | First-order low-pass                        |
| Fusion    | [ComplementaryFilter](src/kf/filter/ComplementaryFilter.hpp) | Sensor fusion between slow and fast sources |

### Graphics
[`kf/gfx/`](src/kf/gfx/)

| Module                            | Description                                                     |
| --------------------------------- | --------------------------------------------------------------- |
| [Canvas](src/kf/gfx/Canvas.hpp)   | Drawing surface with primitives, text, and sub-canvas splitting |
| [Font](src/kf/gfx/Font.hpp)       | Monospaced bitmap font descriptor                               |
| [Palette](src/kf/gfx/Palette.hpp) | ANSI 16-colour palette for any pixel format                     |
| [Fonts](src/kf/gfx/fonts/)        | Bundled bitmap fonts                                            |

### Images
[`kf/image/`](src/kf/image/)

| Module                                          | Description                                                |
| ----------------------------------------------- | ---------------------------------------------------------- |
| [Image](src/kf/image/Image.hpp)                 | CRTP base for image buffers                                |
| [StaticImage](src/kf/image/StaticImage.hpp)     | Compile-time-sized image with an embedded buffer           |
| [DynamicImage](src/kf/image/DynamicImage.hpp)   | Runtime-sized view with validated sub-regions              |
| [ViewportImage](src/kf/image/ViewportImage.hpp) | Image with optional transposition for orientation handling |

### Pixel Formats
[`kf/pixel/`](src/kf/pixel/)

| Module                                              | Description                                          |
| --------------------------------------------------- | ---------------------------------------------------- |
| [Pixel](src/kf/pixel/Pixel.hpp)                     | CRTP base describing how pixels are stored and drawn |
| [MonochromePixel](src/kf/pixel/MonochromePixel.hpp) | 1-bit monochrome pixel format                        |
| [Rgb565Pixel](src/kf/pixel/Rgb565Pixel.hpp)         | 16-bit big-endian RGB565 pixel format                |

### Listeners
[`kf/listener/`](src/kf/listener/)

| Module                                                           | Description                                                 |
| ---------------------------------------------------------------- | ----------------------------------------------------------- |
| [Listener](src/kf/listener/Listener.hpp)                         | CRTP base for polling-based input with an optional callback |
| [LogicalLevelListener](src/kf/listener/LogicalLevelListener.hpp) | Debounced digital input treated as a button                 |
| [JoystickListener](src/kf/listener/JoystickListener.hpp)         | Joystick direction detection with repeat and hold           |

### Tuners
[`kf/tuner/`](src/kf/tuner/)

| Module                                                          | Description                                              |
| --------------------------------------------------------------- | -------------------------------------------------------- |
| [Tuner](src/kf/tuner/Tuner.hpp)                                 | CRTP base for calibration routines driven by polling     |
| [SampleCollectingTuner](src/kf/tuner/SampleCollectingTuner.hpp) | Helper for tuners that collect a fixed number of samples |

### UI
[`kf/ui/`](src/kf/ui/)

| Group     | Module                                                          | Description                                                                    |
| --------- | --------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| Framework | [UI](src/kf/ui/UI.hpp)                                          | Page-based UI framework with widgets, events, and a pluggable renderer         |
|           | [UiTraits](src/kf/ui/UiTraits.hpp)                              | Defines the UI's widget, renderer, event, and adjuster types                   |
| Semantics | [Color](src/kf/ui/Color.hpp)                                    | Semantic colour tags independent of hardware                                   |
|           | [Style](src/kf/ui/Style.hpp)                                    | Widget style (foreground and background colours)                               |
|           | [Block](src/kf/ui/Block.hpp)                                    | Semantic block types for layout and grouping                                   |
|           | [Layout](src/kf/ui/Layout.hpp)                                  | Layout hints for arranging widgets on a page                                   |
|           | [Decoration](src/kf/ui/Decoration.hpp)                          | Semantic decoration glyphs such as arrows and separators                       |
| Events    | [Event](src/kf/ui/Event.hpp)                                    | Packed UI event: kind plus a small signed value                                |
|           | [Request](src/kf/ui/Request.hpp)                                | What a widget asks the page to do after input                                  |
| Renderers | [Renderer](src/kf/ui/render/Renderer.hpp)                       | CRTP base for UI renderers                                                     |
|           | [TextualRenderer](src/kf/ui/render/TextualRenderer.hpp)         | Plain-text renderer for terminals                                              |
|           | [ColoredTextRenderer](src/kf/ui/render/ColoredTextRenderer.hpp) | ANSI-colour renderer for terminals                                             |
| Widgets   | [Widget set](src/kf/ui/widget/)                                 | Ready-made widgets (buttons, checkboxes, sliders, etc.) built on the UI traits |

### RTOS
[`kf/rtos/`](src/kf/rtos/)

| Module                         | Description                                             |
| ------------------------------ | ------------------------------------------------------- |
| [Clock](src/kf/rtos/Clock.hpp) | Monotonic time source                                   |
| [Task](src/kf/rtos/Task.hpp)   | Minimal task sleep, portable between Arduino and native |

### Mixins
[`kf/mixin/`](src/kf/mixin/)

Reusable CRTP mixins. Each provides a `KF_IMPL_*` macro to place in the `private` section of the derived class.

| Group                 | Mixins                                                                                        | Purpose                                                      |
| --------------------- | --------------------------------------------------------------------------------------------- | ------------------------------------------------------------ |
| Lifecycle             | `NonCopyable`, `Initable`, `Resettable`, `DefaultResettable`, `Quitable`                      | Construction, initialisation, reset, and release hooks       |
| Periodic              | `Poll`, `Flush`, `Drain`                                                                      | Recurring or on-demand operations                            |
| Config                | `Configured`                                                                                  | Stores a `const&` to a configuration object                  |
| Callbacks             | `Callbacked<Args...>`, `ValueCallbacked<T>`                                                   | Optional callbacks and value-triggered callbacks             |
| Labels / styling      | `Labeled`, `Styled`                                                                           | Text label and semantic style                                |
| Invariant             | `Invariant`                                                                                   | Some/None-style state checks on optional-like types          |
| Comparison / indexing | `Equatable`, `Indexable`, `Length`                                                            | Equality, indexing, and length                               |
| Streams               | `Readable`, `Writable`, `ReadAvailable`, `WriteAvailable`, `BinaryReadable`, `BinaryWritable` | Text and binary I/O with availability queries                |
| Matching              | `Match`                                                                                       | Key-value matching                                           |
| Stringification       | `Representable`, `ReprTo`                                                                     | Return a string, or write to a `char`-writable sink          |
| Arena helpers         | `ExtraAllocationLength`                                                                       | Report extra allocation length for arena-created objects     |
| Identity              | `MacAddressed`                                                                                | Owns a MAC address with read-only access                     |
| Singleton             | `Singleton`                                                                                   | Meyer's singleton access (hardware-guaranteed services only) |

### ESP-specific
[`kf/esp/`](src/kf/esp/)

| Module                          | Description                                                |
| ------------------------------- | ---------------------------------------------------------- |
| [Espnow](src/kf/esp/Espnow.hpp) | ESP-NOW singleton for peer-to-peer messaging               |
| [NVS](src/kf/esp/NVS.hpp)       | Non-volatile storage wrapper for reading and writing blobs |

## Quick Start

### PlatformIO

```ini
lib_deps = https://github.com/KiraFlux/KiraFlux-Toolkit.git#0.3.5
```

### Minimal application

```cpp
#include <kf/main.hpp>
#include <kf/rtos/Task.hpp>

void kf::main(kf::Init &init) {
    init.logger.info("Hello from KiraFlux Toolkit!");

    // Arena: fast, deterministic allocation; discarded on reset.
    auto ints = init.arena.allocate<int>(5);
    if (not ints.empty()) {
        ints[0] = 42;
        init.logger.info("ints = {}", ints);
    }

    // Non-blocking echo loop.
    while (true) {
        if (init.io.available_for_read() > 0) {
            if (auto const b = init.io.read_byte(); b.is_ok()) {
                init.logger.debug("got: {}", static_cast<char>(b.ok()));
            }
        }

        rtos::Task::sleep(10);
    }
}
```

`kf::main()` replaces `setup()`/`loop()` on Arduino and `main()` on native. It is called after the framework has initialised `init.logger`, `init.arena`, and `init.io` (a `UART` on Arduino, `AppIO` on native).

## Examples

Each example is a self-contained PlatformIO project under `examples/<group>/<name>/`; the "Example" link opens its `src/main.cpp`.

| Group   | Example                                                                                           | Purpose                                                       |
| ------- | ------------------------------------------------------------------------------------------------- | ------------------------------------------------------------- |
| Arduino | [esp.espnow-ping](examples/arduino/esp.espnow-ping/src/main.cpp)                                  | Peer-to-peer messaging over ESP-NOW                           |
|         | [esp.nvs](examples/arduino/esp.nvs/src/main.cpp)                                                  | Storing application configuration in NVS                      |
| Bus     | [i2c.scannner](examples/bus/i2c.scannner/src/main.cpp)                                            | Discovering devices on an I2C bus                             |
| Core    | [arena](examples/core/arena/src/main.cpp)                                                         | Bump allocation and reuse of a fixed memory region            |
|         | [array](examples/core/array/src/main.cpp)                                                         | Fixed-size owning container basics                            |
|         | [bare](examples/core/bare/src/main.cpp)                                                           | Minimal `kf::main` skeleton: logger, arena, echo loop         |
|         | [logger](examples/core/logger/src/main.cpp)                                                       | Log levels, buffered vs unbuffered output                     |
|         | [option](examples/core/option/src/main.cpp)                                                       | Optional values, references to values, and optional callbacks |
|         | [queue](examples/core/queue/src/main.cpp)                                                         | FIFO semantics on a caller-provided buffer                    |
|         | [slice](examples/core/slice/src/main.cpp)                                                         | Non-owning views, slicing, and const conversion               |
|         | [stack](examples/core/stack/src/main.cpp)                                                         | LIFO semantics on a caller-provided buffer                    |
|         | [string](examples/core/string/src/main.cpp)                                                       | Building a string with formatting and C-API compatibility     |
|         | [timer](examples/core/timer/src/main.cpp)                                                         | One-shot, periodic, and stopwatch timing                      |
| Driver  | [actuator.motor-drv8871](examples/driver/actuator.motor-drv8871/src/main.cpp)                     | Running an H-bridge motor with dead-zone compensation         |
|         | [actuator.servo-mg90s](examples/driver/actuator.servo-mg90s/src/main.cpp)                         | Sweeping a PWM servo within a safe angle range                |
|         | [display.ssd1306](examples/driver/display.ssd1306/src/main.cpp)                                   | Drawing on a monochrome OLED across supported orientations    |
|         | [display.st7735](examples/driver/display.st7735/src/main.cpp)                                     | Drawing on a colour TFT across all six orientations           |
|         | [sensor.joystick](examples/driver/sensor.joystick/src/main.cpp)                                   | Reading a calibrated two-axis joystick                        |
|         | [sensor.quadrature-encoder](examples/driver/sensor.quadrature-encoder/src/main.cpp)               | Reading position from a quadrature encoder                    |
|         | [sensor.sharp](examples/driver/sensor.sharp/src/main.cpp)                                         | Reading distance from a Sharp IR sensor                       |
| GPIO    | [adc-input](examples/gpio/adc-input/src/main.cpp)                                                 | Reading an analog pin with configurable resolution            |
|         | [digital-input](examples/gpio/digital-input/src/main.cpp)                                         | Reading a digital pin with pull configuration                 |
|         | [digital-output](examples/gpio/digital-output/src/main.cpp)                                       | Toggling a digital output                                     |
|         | [pwm-output](examples/gpio/pwm-output/src/main.cpp)                                               | Generating a PWM waveform with duty and pulse-width control   |
| Math    | [objects.quaternion](examples/math/objects.quaternion/src/main.cpp)                               | Quaternion rotations and Euler-angle conversion               |
|         | [objects.range](examples/math/objects.range/src/main.cpp)                                         | Closed interval with clamping and length                      |
|         | [objects.vector2](examples/math/objects.vector2/src/main.cpp)                                     | 2D vector arithmetic and normalisation                        |
|         | [objects.vector3](examples/math/objects.vector3/src/main.cpp)                                     | 3D vector arithmetic with cross product                       |
| Misc    | [listener.gpio-as-button](examples/misc/listener.gpio-as-button/src/main.cpp)                     | Debounced button built on top of a digital pin                |
|         | [tuner.custom](examples/misc/tuner.custom/src/main.cpp)                                           | Custom calibration routine running alongside other work       |
| UI      | [complex.colored-textual-gfx-st7735](examples/ui/complex.colored-textual-gfx-st7735/src/main.cpp) | UI with a colour renderer driving a hardware TFT              |
|         | [complex.textual](examples/ui/complex.textual/src/main.cpp)                                       | UI with a plain-text renderer in a terminal                   |

## Building Examples

Manage examples through the repository CLI:

```bash
./kf build --all                 # build all examples (esp32dev)
./kf build                       # rebuild only examples that failed last time
./kf build --env native          # build for the native target
```

To work on a single example, its per-example `makefile` (which `include`s `examples/common.mak`) can be used directly:

```bash
cd examples/core/bare
make build ENV=esp32dev
make upload ENV=esp32dev
make monitor ENV=esp32dev
```

## Testing

Unit tests live in `test/unit/` and run natively with Unity:

```bash
./kf test
```

Hardware-dependent code is isolated in headers guarded by `ARDUINO`; algorithmic code must remain compilable on the native target.

## CLI Reference

All repository-level operations go through `./kf`.

| Command                           | Aliases | Description                                                                 |
| --------------------------------- | ------- | --------------------------------------------------------------------------- |
| `./kf build --all`                | `b`     | Build all examples (`esp32dev` by default)                                  |
| `./kf build`                      | `b`     | Rebuild only examples that failed last time                                 |
| `./kf test`                       | `t`     | Run unit tests with PlatformIO (`native` by default)                        |
| `./kf lint`                       | `l`     | Run `clang-tidy` (`--all` for the whole tree, default changed files only)   |
| `./kf format`                     | `f`     | Run `clang-format` (`--all` for the whole tree, default changed files only) |
| `./kf snapshot`                   | `s`     | Generate `snapshot.txt` (full code dump)                                    |
| `./kf monitor`                    | `m`     | Open serial monitor (`esp32dev`, `115200` baud)                             |
| `./kf clean`                      | `c`     | Clean PlatformIO build artifacts                                            |
| `./kf diff`                       | `d`     | Write `log.txt` and `diff.txt` between `main` and `dev`                     |
| `./kf new example <group> <name>` | `n e`   | Scaffold a new example                                                      |
| `./kf new test <group> <name>`    | `n t`   | Scaffold a new unit test                                                    |

Example usage:

```bash
./kf test
./kf build --all
./kf new example input my-example
./kf new test common my-test
./kf format --all
./kf lint --all
./kf diff
```

## Documentation

Public APIs are documented with **Doxygen comments** in the headers. There is no separate generated documentation site; examples in `examples/` serve as the primary usage reference.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the workflow (branching, commit style, CI, code conventions, CLI reference).

## License

GPL-3.0-or-later — see [LICENSE](LICENSE) file.
