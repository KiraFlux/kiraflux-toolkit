# KiraFlux Toolkit

**Experimental header-only C++20 library for ESP32 (Arduino framework)**
A pragmatic blend of static polymorphism (CRTP) and dynamic polymorphism for embedded development.

## Overview

KiraFlux Toolkit provides modular components for ESP32, built around:

- **CRTP** - for zero-cost abstractions (GPIO, buses, drivers, filters, mixins).
- **Dynamic polymorphism** - where runtime flexibility is required (UI widgets, type-erased callables via `kf::Function`).

The library is **header-only**, **C++20**, targets **ESP32 + Arduino framework**. Many core modules are platform-agnostic and compile natively.

**Key principles:**
- No exceptions - error handling via `kf::Result` and `kf::Option` return values.
- `[[nodiscard]]`, `constexpr`, `noexcept` where applicable.
- Minimal hardware-specific dependencies; algorithmic code has no `<Arduino.h>`.
- Algorithmic code testable natively (x86) with Unity.

## Status

**Experimental** - APIs may change between minor versions. Feedback and contributions welcome.

## Modules

### Core
[`kf/`](src/kf/)

| Group      | Module                              | Description                                                                                         |
| ---------- | ----------------------------------- | --------------------------------------------------------------------------------------------------- |
| Foundation | [Core](src/kf/core.hpp)             | Primitive aliases (`u8`/`i32`/`f32`/`usize`), `NoneType`, concepts, type traits                     |
|            | [Units](src/kf/units.hpp)           | `Milliseconds`, `Microseconds`, `Degrees`, `Pixels`, etc.                                           |
|            | [Math](src/kf/math.hpp)             | `clamp`, `linearMap`, `crc32`, `Range`, `Vector2`/`Vector3`/`Quaternion`                            |
| Errors     | [Option](src/kf/Option.hpp)         | `Option<T>` with `some()`, `none`, `someRef()`, `unwrap()`, `unwrapOr()`, `map()`, sentinel storage |
|            | [Result](src/kf/Result.hpp)         | `Result<T, E>` with `ok()`, `error()`, `map()`, `mapError()`, `KF_TRY`                              |
| Memory     | [Arena](src/kf/Arena.hpp)           | Linear allocator with O(1) reset, `allocate<T>()`, `create<T>()`                                    |
| Containers | [Array](src/kf/Array.hpp)           | Fixed-size owning array, Sequence interface, `fill()`                                               |
|            | [Slice](src/kf/Slice.hpp)           | Non-owning view (`sub`, `first`, `last`, `fromOffset`, const conversion)                            |
|            | [Bytes](src/kf/Bytes.hpp)           | `Slice<u8>` alias                                                                                   |
|            | [BytesView](src/kf/BytesView.hpp)   | `Slice<u8 const>` alias                                                                             |
|            | [Stack](src/kf/Stack.hpp)           | LIFO container on a caller-provided buffer                                                          |
|            | [Queue](src/kf/Queue.hpp)           | FIFO container on a caller-provided buffer                                                          |
|            | [Registry](src/kf/Registry.hpp)     | Arena-backed registry with `add()` / `get()`                                                        |
| Strings    | [String](src/kf/String.hpp)         | Dynamic string builder with `appendFormat`                                                          |
|            | [StringView](src/kf/StringView.hpp) | Read-only string view with trim/search/split                                                        |
| Iteration  | [Sequence](src/kf/Sequence.hpp)     | CRTP base for sequences (`begin`/`end`, `slice()`, `firstWhere`)                                    |
|            | [Iterator](src/kf/Iterator.hpp)     | CRTP iterator with `next()` / `value()`                                                             |
| Callables  | [Function](src/kf/Function.hpp)     | Type-erased callable with small-buffer optimization - never empty                                   |
| Utilities  | [Timer](src/kf/Timer.hpp)           | Monotonic timer with `start`, `expired`, `elapsed`, `remaining`                                     |
|            | [Logger](src/kf/Logger.hpp)         | `debug` / `info` / `warn` / `error` with compile-time format strings                                |
| Control    | [PID](src/kf/PID.hpp)               | PID controller with derivative filter and integral anti-windup                                      |
| I/O        | [GPIO](src/kf/gpio.hpp)             | `GpioNumber`, `DigitalInput`, `DigitalOutput`, `AdcInput`, `PwmOutput`                              |
|            | [UART](src/kf/UART.hpp)             | Buffered UART (`HardwareSerial` on Arduino, no-op stub natively)                                    |
| Networking | [MacAddress](src/kf/MacAddress.hpp) | 6-byte MAC address wrapper                                                                          |
| App        | [Main](src/kf/main.hpp)             | `kf::main(Init&)` entry point with `arena`, `logger`, `io`                                          |

### Buses
[`kf/bus/`](src/kf/bus/)

| Module                    | Description                                                  |
| ------------------------- | ------------------------------------------------------------ |
| [Bus](src/kf/bus/Bus.hpp) | CRTP base with `createNode()`                                |
| [I2C](src/kf/bus/I2C.hpp) | `I2C` bus and node, `I2C::Error`, Arduino `TwoWire` backend  |
| [SPI](src/kf/bus/SPI.hpp) | `SPI` bus and node, `SPI::Error`, Arduino `SPIClass` backend |

### Drivers
[`kf/driver/`](src/kf/driver/)

| Group    | Module                                                            | Description                                               |
| -------- | ----------------------------------------------------------------- | --------------------------------------------------------- |
| Core     | [Driver](src/kf/driver/Driver.hpp)                                | CRTP driver base                                          |
| Actuator | [ActuatorDriver](src/kf/driver/actuator/ActuatorDriver.hpp)       | CRTP base for actuators (`set`, `stop`, `init`)           |
|          | [DRV8871](src/kf/driver/actuator/DRV8871.hpp)                     | H-bridge motor driver with dead-zone compensation         |
|          | [PwmPositionServo](src/kf/driver/actuator/PwmPositionServo.hpp)   | PWM servo with angle↔pulse mapping and safe range         |
| Display  | [DisplayDriver](src/kf/driver/display/DisplayDriver.hpp)          | CRTP display base (framebuffer, `send`, `orientation`)    |
|          | [Orientation](src/kf/driver/display/Orientation.hpp)              | Display orientation enum                                  |
|          | [SSD1306](src/kf/driver/display/SSD1306.hpp)                      | 128×64 monochrome I2C OLED driver                         |
|          | [ST7735](src/kf/driver/display/ST7735.hpp)                        | 128×160 RGB565 SPI TFT driver                             |
| Sensor   | [SensorDriver](src/kf/driver/sensor/SensorDriver.hpp)             | CRTP sensor base (`read`, `init`)                         |
|          | [Joystick](src/kf/driver/sensor/Joystick.hpp)                     | Two-axis joystick with calibration and unit-circle output |
|          | [NormalizedAdcInput](src/kf/driver/sensor/NormalizedAdcInput.hpp) | Single analog axis with filtering and dead-zone           |
|          | [QuadratureEncoder](src/kf/driver/sensor/QuadratureEncoder.hpp)   | Quadrature encoder with 4× decoding                       |
|          | [Sharp](src/kf/driver/sensor/Sharp.hpp)                           | Sharp IR distance sensor (analog, mm)                     |

### Filters
[`kf/filter/`](src/kf/filter/)

| Group     | Module                                                       | Description                              |
| --------- | ------------------------------------------------------------ | ---------------------------------------- |
| Core      | [Filter](src/kf/filter/Filter.hpp)                           | CRTP filter base (`calc()`)              |
| Smoothing | [ExponentialFilter](src/kf/filter/ExponentialFilter.hpp)     | Exponential moving average               |
|           | [LowFrequencyFilter](src/kf/filter/LowFrequencyFilter.hpp)   | First-order low-pass                     |
| Fusion    | [ComplementaryFilter](src/kf/filter/ComplementaryFilter.hpp) | Sensor fusion (prediction + measurement) |

### Graphics
[`kf/gfx/`](src/kf/gfx/)

| Module                            | Description                                       |
| --------------------------------- | ------------------------------------------------- |
| [Canvas](src/kf/gfx/Canvas.hpp)   | Lines, rects, circles, glyphs, text, sub-canvases |
| [Font](src/kf/gfx/Font.hpp)       | Monospaced 1-bit font descriptor                  |
| [Palette](src/kf/gfx/Palette.hpp) | ANSI 16-colour palette for any pixel format       |
| [Fonts](src/kf/gfx/fonts/)        | Built-in `gyver_5x7_en`                           |

### Images
[`kf/image/`](src/kf/image/)

| Module                                          | Description                                       |
| ----------------------------------------------- | ------------------------------------------------- |
| [Image](src/kf/image/Image.hpp)                 | CRTP base (`width`, `height`, `stride`, `buffer`) |
| [StaticImage](src/kf/image/StaticImage.hpp)     | Compile-time dimensions, embedded buffer          |
| [DynamicImage](src/kf/image/DynamicImage.hpp)   | Runtime dimensions, sub-region support            |
| [ViewportImage](src/kf/image/ViewportImage.hpp) | Optional transpose (width/height swap)            |

### Pixel Formats
[`kf/pixel/`](src/kf/pixel/)

| Module                                              | Description                                                     |
| --------------------------------------------------- | --------------------------------------------------------------- |
| [Pixel](src/kf/pixel/Pixel.hpp)                     | CRTP base (`bufferSize`, `setPixel`, `fill`, `copy`, `fromRgb`) |
| [MonochromePixel](src/kf/pixel/MonochromePixel.hpp) | 1-bit monochrome (8 pixels per byte)                            |
| [Rgb565Pixel](src/kf/pixel/Rgb565Pixel.hpp)         | 16-bit big-endian RGB565                                        |

### Listeners
[`kf/listener/`](src/kf/listener/)

| Module                                                           | Description                             |
| ---------------------------------------------------------------- | --------------------------------------- |
| [Listener](src/kf/listener/Listener.hpp)                         | CRTP base (`set` / `poll` / callback)   |
| [LogicalLevelListener](src/kf/listener/LogicalLevelListener.hpp) | Debounced button press                  |
| [JoystickListener](src/kf/listener/JoystickListener.hpp)         | Direction with repeat and initial delay |

### Tuners
[`kf/tuner/`](src/kf/tuner/)

| Module                                                          | Description                                  |
| --------------------------------------------------------------- | -------------------------------------------- |
| [Tuner](src/kf/tuner/Tuner.hpp)                                 | CRTP tuner base (`reset`, `poll`, `running`) |
| [SampleCollectingTuner](src/kf/tuner/SampleCollectingTuner.hpp) | Fixed-sample collection helper               |

### UI
[`kf/ui/`](src/kf/ui/)

| Group     | Module                                                          | Description                                                                                             |
| --------- | --------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| Framework | [UI](src/kf/ui/UI.hpp)                                          | Main UI framework: pages, widgets, events, rendering                                                    |
|           | [UiTraits](src/kf/ui/UiTraits.hpp)                              | Widget base and adjusters (`ArithmeticAdjuster`, `ArithmeticPositiveOnlyAdjuster`, `GeometricAdjuster`) |
| Semantics | [Color](src/kf/ui/Color.hpp)                                    | Semantic color tags (Normal, Primary, Success, …)                                                       |
|           | [Style](src/kf/ui/Style.hpp)                                    | Widget style (foreground/background colors)                                                             |
|           | [Block](src/kf/ui/Block.hpp)                                    | Semantic block types (Standard, Alternative)                                                            |
|           | [Layout](src/kf/ui/Layout.hpp)                                  | Layout hints (Vertical, Horizontal, Grid)                                                               |
|           | [Decoration](src/kf/ui/Decoration.hpp)                          | Semantic decoration elements (arrows, colon, space)                                                     |
| Events    | [Event](src/kf/ui/Event.hpp)                                    | Packed `Event<N>` (2-bit kind + N-bit signed value)                                                     |
|           | [Request](src/kf/ui/Request.hpp)                                | `Nothing` / `Redraw` / `Rebuild`                                                                        |
| Renderers | [Renderer](src/kf/ui/render/Renderer.hpp)                       | CRTP renderer base                                                                                      |
|           | [TextualRenderer](src/kf/ui/render/TextualRenderer.hpp)         | Plain-text terminal renderer                                                                            |
|           | [ColoredTextRenderer](src/kf/ui/render/ColoredTextRenderer.hpp) | ANSI-colour terminal renderer                                                                           |
| Widgets   | [Widget set](src/kf/ui/widget/)                                 | `Widget`, `Button`, `CheckBox`, `ComboBox`, `Display`, `Fold`, `Labeled`, `Slider`, `SpinBox`           |

### RTOS
[`kf/rtos/`](src/kf/rtos/)

| Module                         | Description                                    |
| ------------------------------ | ---------------------------------------------- |
| [Clock](src/kf/rtos/Clock.hpp) | Monotonic `now()` in milliseconds              |
| [Task](src/kf/rtos/Task.hpp)   | `sleep(duration)` (Arduino `delay` / `usleep`) |

### Mixins
[`kf/mixin/`](src/kf/mixin/)

Reusable CRTP mixins. Each provides a `KF_IMPL_*` macro to place in the `private` section of the derived class.

| Group                 | Mixins                                                                                        | Purpose                                                      |
| --------------------- | --------------------------------------------------------------------------------------------- | ------------------------------------------------------------ |
| Lifecycle             | `NonCopyable`, `Initable`, `Resettable`, `DefaultResettable`, `Quitable`                      | Construction, initialization, reset, and release hooks       |
| Periodic              | `Poll`, `Flush`, `Drain`                                                                      | Recurring or on-demand operations                            |
| Config                | `Configured`                                                                                  | Stores a `const&` to a configuration object                  |
| Callbacks             | `Callbacked<Args...>`, `ValueCallbacked<T>`                                                   | Optional callbacks and value-triggered callbacks             |
| Labels / styling      | `Labeled`, `Styled`                                                                           | Text label and semantic style (foreground/background colors) |
| Invariant             | `Invariant`                                                                                   | Adds `isSome()` / `isNone()` to optional-like types          |
| Comparison / indexing | `Equatable`, `Indexable`, `Length`                                                            | Equality, `operator[]`, and `length()`                       |
| Streams               | `Readable`, `Writable`, `ReadAvailable`, `WriteAvailable`, `BinaryReadable`, `BinaryWritable` | Text and binary I/O with availability queries                |
| Matching              | `Match`                                                                                       | Key-value matching via `match(value)`                        |
| Stringification       | `Representable` (`repr()`), `ReprTo` (`reprTo()`)                                             | Return a string, or write to a `char` writable sink          |
| Arena helpers         | `ExtraAllocationLength`                                                                       | Report extra allocation length for `Arena::create<T>()`      |
| Identity              | `MacAddressed`                                                                                | Owns a MAC address with read-only access                     |
| Singleton             | `Singleton`                                                                                   | Meyer's singleton access (hardware-guaranteed services only) |

### ESP-specific
[`kf/esp/`](src/kf/esp/)

| Module                          | Description                                                   |
| ------------------------------- | ------------------------------------------------------------- |
| [Espnow](src/kf/esp/Espnow.hpp) | ESP-NOW singleton with peer management and callbacks          |
| [NVS](src/kf/esp/NVS.hpp)       | Non-volatile storage wrapper (`getBlob`, `setBlob`, `commit`) |

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
        if (init.io.availableForRead() > 0) {
            if (auto const b = init.io.readByte(); b.isOk()) {
                init.logger.debug("got: {}", static_cast<char>(b.ok()));
            }
        }

        rtos::Task::sleep(10);
    }
}
```

`kf::main()` replaces `setup()`/`loop()` on Arduino and `main()` on native. It is called after the framework has initialized `init.logger`, `init.arena`, and `init.io` (a `UART` on Arduino, `AppIO` on native).

## Examples

Each example is a self-contained PlatformIO project under `examples/<group>/<name>/`; the "Example" link opens its `src/main.cpp`.

| Group   | Example                                                                                           | Description                                                     |
| ------- | ------------------------------------------------------------------------------------------------- | --------------------------------------------------------------- |
| Arduino | [esp.espnow-ping](examples/arduino/esp.espnow-ping/src/main.cpp)                                  | ESP-NOW peer-to-peer ping and receive callback                  |
|         | [esp.nvs](examples/arduino/esp.nvs/src/main.cpp)                                                  | NVS config storage with magic and defaults                      |
| Bus     | [i2c.scannner](examples/bus/i2c.scannner/src/main.cpp)                                            | I2C bus scanner (addresses `0x01`-`0x7F`)                       |
| Core    | [arena](examples/core/arena/src/main.cpp)                                                         | Arena linear allocator with typed allocation and sub-arena      |
|         | [array](examples/core/array/src/main.cpp)                                                         | Fixed-size `Array` container with `fill` and equality           |
|         | [bare](examples/core/bare/src/main.cpp)                                                           | Minimal `kf::main` entry point: logger, arena, echo loop        |
|         | [logger](examples/core/logger/src/main.cpp)                                                       | Logger levels and buffered vs unbuffered writers                |
|         | [option](examples/core/option/src/main.cpp)                                                       | `Option<T>`, `Option<T&>`, `Option<Function>`, sentinel storage |
|         | [queue](examples/core/queue/src/main.cpp)                                                         | FIFO `Queue` container on a fixed buffer                        |
|         | [slice](examples/core/slice/src/main.cpp)                                                         | `Slice` construction, slicing, const conversion                 |
|         | [stack](examples/core/stack/src/main.cpp)                                                         | LIFO `Stack` container on a fixed buffer                        |
|         | [string](examples/core/string/src/main.cpp)                                                       | `String` builder with `appendFormat` and `cString`              |
|         | [timer](examples/core/timer/src/main.cpp)                                                         | `Timer` with `start`, `expired`, `elapsed`, `remaining`         |
| Driver  | [actuator.motor-drv8871](examples/driver/actuator.motor-drv8871/src/main.cpp)                     | DRV8871 H-bridge motor with dead-zone compensation              |
|         | [actuator.servo-mg90s](examples/driver/actuator.servo-mg90s/src/main.cpp)                         | MG90S PWM servo with angle-to-pulse mapping                     |
|         | [display.ssd1306](examples/driver/display.ssd1306/src/main.cpp)                                   | SSD1306 I2C OLED driver across supported orientations           |
|         | [display.st7735](examples/driver/display.st7735/src/main.cpp)                                     | ST7735 SPI TFT driver across all 6 orientations                 |
|         | [sensor.joystick](examples/driver/sensor.joystick/src/main.cpp)                                   | Two-axis joystick with calibration                              |
|         | [sensor.quadrature-encoder](examples/driver/sensor.quadrature-encoder/src/main.cpp)               | Quadrature encoder with 4× decoding                             |
|         | [sensor.sharp](examples/driver/sensor.sharp/src/main.cpp)                                         | Sharp IR distance sensor (analog, mm)                           |
| GPIO    | [adc-input](examples/gpio/adc-input/src/main.cpp)                                                 | Analog-to-digital input with global resolution                  |
|         | [digital-input](examples/gpio/digital-input/src/main.cpp)                                         | Digital input with pull configuration                           |
|         | [digital-output](examples/gpio/digital-output/src/main.cpp)                                       | Digital output level toggle                                     |
|         | [pwm-output](examples/gpio/pwm-output/src/main.cpp)                                               | PWM output with sawtooth duty ramp                              |
| Math    | [objects.quaternion](examples/math/objects.quaternion/src/main.cpp)                               | Quaternion rotations, Euler angles, axis-angle                  |
|         | [objects.range](examples/math/objects.range/src/main.cpp)                                         | `Range<T>` with clamping and length                             |
|         | [objects.vector2](examples/math/objects.vector2/src/main.cpp)                                     | `Vector2` arithmetic, dot product, normalize                    |
|         | [objects.vector3](examples/math/objects.vector3/src/main.cpp)                                     | `Vector3` with cross product                                    |
| Misc    | [listener.gpio-as-button](examples/misc/listener.gpio-as-button/src/main.cpp)                     | `LogicalLevelListener` as a debounced button                    |
|         | [tuner.custom](examples/misc/tuner.custom/src/main.cpp)                                           | Custom `Tuner` computing min/max/average                        |
| UI      | [complex.colored-textual-gfx-st7735](examples/ui/complex.colored-textual-gfx-st7735/src/main.cpp) | UI with `ColoredTextRenderer` on a hardware ST7735              |
|         | [complex.textual](examples/ui/complex.textual/src/main.cpp)                                       | UI with `TextualRenderer` in a terminal                         |

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

| Command                           | Aliases | Description                                                                       |
| --------------------------------- | ------- | --------------------------------------------------------------------------------- |
| `./kf build --all`                | `b`     | Build all examples (`esp32dev` by default).                                       |
| `./kf build`                      | `b`     | Rebuild only examples that failed last time (`failed_examples.txt`).              |
| `./kf test`                       | `t`     | Run unit tests with PlatformIO (`native` by default).                             |
| `./kf lint`                       | `l`     | Run `clang-tidy` checks (`--all` for the whole tree, default changed files only). |
| `./kf format`                     | `f`     | Run `clang-format` (`--all` for the whole tree, default changed files only).      |
| `./kf snapshot`                   | `s`     | Generate `snapshot.txt` (full code dump).                                         |
| `./kf monitor`                    | `m`     | Open serial monitor (`esp32dev`, `115200` baud).                                  |
| `./kf clean`                      | `c`     | Clean PlatformIO build artifacts.                                                 |
| `./kf diff`                       | `d`     | Write `log.txt` and `diff.txt` between `main` and `dev`.                          |
| `./kf new example <group> <name>` | `n e`   | Scaffold a new example under `examples/<group>/<name>/`.                          |
| `./kf new test <group> <name>`    | `n t`   | Scaffold a new unit test under `test/unit/<group>/test_<name>/`.                  |

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

GPL-3.0-or-later - see [LICENSE](LICENSE) file.
