# PlatformIO Setup

OpenSkyhawk firmware builds with [PlatformIO](https://platformio.org/). Each board is its own
project. Start from the templates in `Firmware/Templates/` — **not** from an existing panel's
`platformio.ini`, which may carry stale dependencies.

## Start from a template

`Firmware/Templates/` has a ready project per tier: `PanelGroup/`, `PanelBridge/`, and
`SimGateway/`. Copy the one you need into place (a PanelGroup goes under
`Firmware/Panels/<YourPanel>/`) and edit its `platformio.ini`.

## A PanelGroup platformio.ini, annotated

```ini
[env:PanelGroup]
platform = ststm32@^20.0.0        ; major held on purpose — see note below
board = genericSTM32F103C8        ; STM32F103C8 for nodes (see board note below)
framework = arduino
board_build.f_cpu = 72000000L

build_flags =
    -DNODE_ID=1                   ; unique per board, 1–63 — set this!
    -DHAL_CAN_MODULE_ENABLED      ; enable the bxCAN peripheral
    -DUSB_NONE                    ; inert — USB is off because USBCON is never defined
    -DHSE_VALUE=8000000           ; external 8 MHz crystal, required for CAN timing

lib_extra_dirs = ${PROJECT_DIR}/../../Libraries
lib_ldf_mode = deep+              ; needed so Adafruit BusIO finds framework headers
lib_deps =
    HIDControls
    STM32Board
    PanelGroup
    CANProtocol
    A4EC
    blemasle/MCP23017@^2.0.0
    adafruit/Adafruit ADS1X15@^2.0.0

upload_protocol = stlink
monitor_speed = 115200
```

The two build flags people forget: **`-DNODE_ID`** (every board needs a unique value — see
[NODE_ID & CAN Addressing](node-id.md)) and **`-DHSE_VALUE=8000000`** (declares the 8 MHz
crystal frequency to HAL — needed for correct CAN timing, since the internal RC oscillator
isn't accurate enough for 500 kbps CAN). Note `-DHSE_VALUE` does *not* by itself select HSE;
`STM32Board`'s `SystemClock_Config` does that (see [Design Decisions](../architecture/design-decisions.md)).

`-DUSB_NONE` is a label, not a switch — nothing in STM32duino or PlatformIO reads it. USB really is
off, and CAN really does own PA11/PA12 (they're the F103's USB D+/D− pins), but what keeps USB off
is that `USBCON` is never defined. Leave the flag in place as documentation of the decision.

For every flag in this file — plus the ones that aren't, their defaults, and where each is read —
see [Build Flags](build-flags.md).

!!! warning "Keep the version constraint"
    The `@^20.0.0` is deliberate — don't drop it back to a bare `platform = ststm32`. An
    unconstrained platform resolves to whatever is newest at build time, so an upstream release
    lands in your build with no change on our side. That is exactly how the weekly firmware build
    went red for two weeks: `ststm32` 20.0.0 arrived with STM32 Arduino core **3.0.0**, which
    renamed the `HardwareSerial` class (see below), and every STM32 project stopped compiling.

    The caret accepts 20.x — new boards and Cube HAL patches still flow in — but blocks the next
    major, which is where a break of that kind comes from. Crossing a major is a deliberate change:
    do it in a PR, with a full build behind it.

!!! note "Declaring your own UART: use `Uart`, not `HardwareSerial`"
    Core 3.0.0 adopted ArduinoCore-API. The concrete STM32 serial class is now **`Uart`**, and the
    name `HardwareSerial` refers to the abstract `arduino::HardwareSerial` base — you cannot
    instantiate it. A second UART is declared `Uart Diag(PA10, PA9);`. Most sketches never need
    this: `STM32Board::diagSerial()` already hands you USART1 on the DiagSerial header.
    On the RP2040 side (SimGateway) the arduino-pico core is unaffected — `HardwareSerial` is
    still the concrete class there.

!!! note "Which STM32 variant"
    Per the [variant policy](../getting-started/prerequisites.md), **all** STM32 boards default to
    **`genericSTM32F103C8`** (64 KB) — both PanelGroup nodes and **PanelBridge**. PanelBridge
    carries the full DCS-BIOS input map yet still compiles to ~26 KB flash, so it fits the C8
    comfortably. **`genericSTM32F103CB`** (128 KB) is a drop-in fallback (identical LQFP48
    footprint) for any future build that ever exceeds 64 KB — no board currently needs it. Note:
    the in-tree `Templates/PanelGroup` should target `genericSTM32F103C8`.

## Flashing

STM32 boards flash over **SWD with an ST-Link** (`upload_protocol = stlink`). PlatformIO drives
the probe directly — `pio run -t upload`. SimGateway (RP2040) flashes over USB by UF2 or
straight from PlatformIO; no ST-Link.

If an upload fails — old ST-Link firmware, or a clone's non-standard tap ID — see
[Flashing → ST-Link troubleshooting](../guides/flashing.md#st-link-troubleshooting).

## Serial monitor

`monitor_speed = 115200` matches the **DiagSerial** debug stream (USART1, PA9/PA10). Attach a
USB-to-TTL adapter to the board's 3-pin debug header and `pio device monitor`. See
[Debugging on STM32](debugging.md).
