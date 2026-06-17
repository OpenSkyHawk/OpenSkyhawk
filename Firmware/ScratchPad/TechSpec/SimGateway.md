# SimGateway — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/07-simgateway-api.md`, `FirmwarePlan/03-uart-usb-hid-protocol.md`, `FirmwarePlan/09-startup-resync-diagnostics.md`
**Depends on:** `HIDControls.md` — CTRL_* controlId constants used in sketch declarations

---

## Responsibility

RP2040 firmware library. Owns:

- **USB CDC ↔ UART relay** — forwards the DCS-BIOS byte stream transparently in both directions; never parses or interprets DCS-BIOS content
- **HID frame demultiplexer** — intercepts binary HID frames (identified by `0xAA 0x55` magic) on the UART RX path; routes controlIds to `HIDAxis`, `HIDButton`, and `HIDHatSwitch` registered objects
- **`HIDAxis`, `HIDButton`, `HIDHatSwitch` classes** — declared in the sketch; self-register into linked lists at construction
- **`OsJoystick.send()` batching** — called once per `loop()` iteration after draining all UART bytes, only if any HID setter fired
- **TinyUSB HID descriptor** — 8 axes, 128 buttons, 4 hat switches; embedded in `SimGateway.cpp` under `#ifndef SIMGATEWAY_TEST`
- **Status-LED state machine** — drives the two board-mounted `Gateway_Bridge` LEDs (GREEN = GP2, RED = GP3, active-high) with a non-blocking `millis()` animator ticked from `loop()`; reports USB / data / fault state (issue #94)

Does **not** run the DCS-BIOS library. Does **not** parse DCS-BIOS addresses or control names. Has no knowledge of CAN frame formats, PanelBridge internals, or the CAN cluster topology.

Platform: **RP2040 only.** Not used on STM32 boards.

---

## File Layout

```
Firmware/Libraries/SimGateway/
├── SimGateway.h     ← SimGateway namespace; HIDAxis, HIDButton, HIDHatSwitch declarations
├── SimGateway.cpp   ← relay loop, parser state machine, linked list dispatch,
│                    TinyUSB HID descriptor + report struct (production only)
└── library.json     ← platform: raspberrypi; dep: adafruit/Adafruit TinyUSB Library

Firmware/SimGateway/
├── platformio.ini
└── src/
    └── main.cpp     ← HIDAxis/HIDButton/HIDHatSwitch declarations + setup()/loop()
```

### library.json

```json
{
  "name": "SimGateway",
  "version": "0.1.0",
  "frameworks": "arduino",
  "platforms": "raspberrypi",
  "dependencies": {
    "adafruit/Adafruit TinyUSB Library": ">=3.0.0"
  }
}
```

Included in the SimGateway sketch:

```cpp
#include <SimGateway.h>    // pulls in HIDAxis, HIDButton, and the SimGateway namespace
#include <HIDControls.h>   // CTRL_ROLL, CTRL_PITCH, etc. — from HIDControls library
```

### Test project

```
Firmware/Tests/SimGateway/
├── platformio.ini
└── tests/
    ├── _support/
    │       (no shim files — test builds use #ifdef SIMGATEWAY_TEST stubs in SimGateway.cpp)
    ├── hid_parser.cpp       — valid 6-byte HID frame (0xAA 0x55 + controlId + value) parsed
    │                          and dispatched; DCS-BIOS bytes (≤ 0x7F) do not trigger setters;
    │                          Joystick.Send() called exactly once after drain if any setter fired
    ├── parser_resync.cpp    — 0xAA followed by non-0x55: both bytes forwarded to CDC;
    │                          valid frame immediately following is parsed correctly; state
    │                          machine self-heals at next byte boundary
    ├── hid_dispatch.cpp     — HIDAxis dispatch called with correct value on controlId match;
    │                          HIDButton index correct on match; non-matching controlId not
    │                          dispatched; Joystick.Send() not called when no setter fires
    ├── cdc_forwarding.cpp   — every non-HID UART byte is forwarded to USB CDC in order;
    │                          valid HID frames are consumed and not forwarded; 0xAA followed
    │                          by non-0x55 forwards both bytes before resuming scan
    ├── uart_loopback.cpp    — public SimGateway::loop() path through the configured RP2040
    │                          UART TX/RX pins; requires GP0→GP1 jumper by default
    └── led_state.cpp        — status-LED state machine: state→colour/animation mapping,
                               priority precedence, STREAMING↔USB_IDLE 500 ms decay, INIT→NO_HOST
                               2 s window, FAULT latch/recovery, animation phase timing.
                               Pure logic via SIMGATEWAY_TEST hooks — no GPIO/TinyUSB/registers
```

Tests are intended to run on a real RP2040 board as on-device harness tests. They simulate
PanelBridge-originated UART data by feeding synthetic bytes into the SimGateway parser; they do
not read local ADC pins, I2C ADCs, sensors, CAN, or any physical cockpit controls.

Two test styles are valid:

- **Parser/dispatch harnesses** call a `SIMGATEWAY_TEST`-only `SimGateway::feedByte()` helper with
  synthetic HID-frame bytes. This verifies frame parsing, resync, controlId matching, value
  mapping, and setter/send counters without requiring a physical UART connection.
- **UART loopback harnesses** may jumper `Serial1` TX to `Serial1` RX on the RP2040 and write the
  same synthetic HID frames into `Serial1`, then call `SimGateway::loop()`. This verifies the
  public drain path and `Joystick.Send()` batching with real UART hardware, but still does not
  require PanelBridge hardware.

The default SimGateway UART pins are RP2040 `GP0` TX and `GP1` RX (`Serial1` / UART0), crossed to
PanelBridge STM32 `PA3` RX and `PA2` TX. Test harnesses that exercise the physical UART path must
use these configured pins unless a board revision explicitly passes alternate `txPin` / `rxPin`
arguments to `SimGateway::setup()`. For loopback testing, jumper the configured TX pin to the
configured RX pin; the default jumper is `GP0 -> GP1`.

Test builds may expose small `SIMGATEWAY_TEST` instrumentation counters or accessors for
assertions, such as last dispatched controlId/value, last axis/button index, mapped axis value,
button pressed state, `Joystick.Send()` count, and a bounded capture of bytes written to USB CDC
by the parser forwarding path. The CDC capture is used only to assert pass-through behavior; the
production path still writes through `Serial.write()` directly via the same helper. These hooks are
compiled out of production builds.

Test builds define `SIMGATEWAY_TEST`, which activates no-op stubs for all TinyUSB HID helpers
directly inside `SimGateway.cpp` (under `#else` of the `#ifndef SIMGATEWAY_TEST` block). No
external shim file is needed. Production builds compile the full `Adafruit_USBD_HID` descriptor
and report struct in the same file, keeping the library self-contained.

CDC forwarding tests must cover:

1. Printable DCS-BIOS command bytes from UART, for example `"ARM_MASTER 1\n"`, are captured in the
   same order and do not trigger any HID setter.
2. A valid HID frame beginning with `0xAA 0x55` is consumed by HID dispatch and contributes no bytes
   to the CDC forwarding capture.
3. A bad magic prefix such as `0xAA 0x42` forwards exactly `0xAA, 0x42`, returns to `IDLE`, and the
   next ordinary DCS-BIOS byte forwards normally.
4. Ordinary DCS-BIOS bytes after a valid HID frame still forward normally, proving the parser returns
   to `IDLE` after frame dispatch.

UART pin loopback tests must cover:

1. `SimGateway::setup(Serial1)` configures the standard UART pins (`GP0` TX, `GP1` RX) before
   `Serial1.begin(250000)`.
2. Bytes written to `Serial1` TX and received back on `Serial1` RX through a physical jumper are
   drained by `SimGateway::loop()`, proving the public UART path reaches the same parser and CDC
   forwarding capture as `feedByte()`.
3. Valid HID frames sent through the loopback are consumed by HID dispatch and not forwarded to CDC.
4. Bad magic sent through the loopback still forwards `0xAA` plus the mismatched byte and returns to
   scanning.

No USB host is required. USB CDC is used only as the on-device test report console.

**`platformio.ini`:**

```ini
[env_base]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = rpipico
board_build.core = earlephilhower
framework = arduino
build_flags =
    -DSIMGATEWAY_TEST
    -Itests/_support
lib_extra_dirs = ${PROJECT_DIR}/../../Libraries
lib_deps =
    SimGateway
    HIDControls

[env:test_hid_parser]
extends = env_base
build_src_filter = -<*> +<hid_parser/hid_parser.cpp>

[env:test_parser_resync]
extends = env_base
build_src_filter = -<*> +<parser_resync/parser_resync.cpp>

[env:test_hid_dispatch]
extends = env_base
build_src_filter = -<*> +<hid_dispatch/hid_dispatch.cpp>

[env:test_cdc_forwarding]
extends = env_base
build_src_filter = -<*> +<cdc_forwarding/cdc_forwarding.cpp>

[env:test_uart_loopback]
extends = env_base
build_src_filter = -<*> +<uart_loopback/uart_loopback.cpp>

[env:test_led_state]
extends = env_base
build_src_filter = -<*> +<led_state/led_state.cpp>
```

---

## Public API

```cpp
// SimGateway.h

#pragma once
#include <Arduino.h>

namespace OpenSkyhawk {

/**
 * @brief HID axis handler. Declared at sketch scope for each joystick axis.
 *
 * Self-registers into a static linked list at construction. SimGateway::loop() walks
 * the list and calls Joystick.SetAxis() when a HID frame with a matching controlId arrives.
 * The 0–65535 value from PanelGroup is mapped to ±32767 internally (value - 32768).
 * The sketch has no knowledge of the Joystick API or value scaling.
 */
class HIDAxis {
public:
    /**
     * @brief Register a HID axis handler.
     * @param controlId  CTRL_* constant from HIDControls.h (0x0010–0x001F range).
     * @param axisIndex  TinyUSB HID axis index (0–7: X/Y/Z/Rx/Ry/Rz/Slider/Dial).
     */
    HIDAxis(uint16_t controlId, uint8_t axisIndex);

    // ── Linked list traversal — used internally by SimGateway ────────────────
    static HIDAxis* head();       ///< First element of the registered axis list.
    uint16_t controlId() const;   ///< controlId this handler is registered for.
    void dispatch(uint16_t value);///< Calls _hidSetAxis(axisIndex, value - 32768) internally.
    HIDAxis* next() const;        ///< Next axis in the linked list; nullptr at end.

private:
    static HIDAxis* _head;
    HIDAxis*        _next;
    uint16_t        _controlId;
    uint8_t         _axisIndex;
};

/**
 * @brief HID button handler. Declared at sketch scope for each button.
 *
 * Self-registers into a static linked list at construction.
 * value != 0 → button pressed; value == 0 → button released.
 */
class HIDButton {
public:
    /**
     * @brief Register a HID button handler.
     * @param controlId   CTRL_* button constant from HIDControls.h (0x0030–0x00AF range).
     * @param buttonIndex TinyUSB HID button index (0–127).
     */
    HIDButton(uint16_t controlId, uint8_t buttonIndex);

    // ── Linked list traversal — used internally by SimGateway ────────────────
    static HIDButton* head();
    uint16_t controlId() const;
    void dispatch(uint16_t value);
    HIDButton* next() const;

private:
    static HIDButton* _head;
    HIDButton*        _next;
    uint16_t          _controlId;
    uint8_t           _buttonIndex;
};

} // namespace OpenSkyhawk

namespace SimGateway {

    static constexpr uint8_t DEFAULT_UART_TX_PIN = 0; ///< RP2040 UART0 TX to PanelBridge RX.
    static constexpr uint8_t DEFAULT_UART_RX_PIN = 1; ///< RP2040 UART0 RX from PanelBridge TX.

    /**
     * @brief Initialise USB identity, Joystick, and UART link to PanelBridge. Call from setup().
     *
     * Owns all hardware initialisation — the sketch calls nothing else before this:
     *   - Sets USB identity: Manufacturer "OpenSkyhawk", Product "A-4E Skyhawk", VID/PID 0x2E8A/0x4134
     *   - Configures UART pins and calls uart.begin(250000)
     *   - Configures MGS-Pico-Joystick for manual send mode
     *
     * USB identity must be set before the USB stack enumerates, so this must be the
     * first call in the sketch's setup().
     *
     * @param uart   Hardware UART connected to PanelBridge (Serial1 / UART0).
     * @param txPin  RP2040 UART TX pin. Defaults to GP0, wired to STM32 PA3.
     * @param rxPin  RP2040 UART RX pin. Defaults to GP1, wired to STM32 PA2.
     */
    void setup(SerialUART& uart, uint8_t txPin = DEFAULT_UART_TX_PIN, uint8_t rxPin = DEFAULT_UART_RX_PIN);

    /**
     * @brief Relay bytes and dispatch HID frames. Call once per loop() iteration.
     *
     * Each call:
     *   1. Forward all bytes from USB CDC (Serial) to UART — raw DCS-BIOS stream to PanelBridge.
     *   2. Drain all bytes from UART:
     *        byte ≤ 0x7F       → forward to USB CDC immediately (DCS-BIOS byte from PanelBridge).
     *        byte == 0xAA, next == 0x55 → HID frame: read 4 more bytes, parse controlId + value,
     *                                     walk HIDAxis, HIDButton, HIDHatSwitch linked lists, dispatch matches.
     *        byte == 0xAA, next != 0x55 → forward 0xAA + mismatched byte to USB CDC; resume.
     *   3. If any HID setter fired this iteration, call Joystick.Send() exactly once.
     *
     * @note The parser state machine is persistent across calls — a HID frame split
     *       across loop() iterations is assembled correctly.
     */
    void loop();

    // ── Status LEDs (Gateway_Bridge: GREEN = GP2, RED = GP3, active-high) ─────
    enum class LedState : uint8_t { FAULT, NO_HOST, STREAMING, USB_IDLE, INIT };
    enum class Anim     : uint8_t { OFF, SOLID, SLOW, FAST, ALT, PULSE };

    void statusLedBegin(); ///< Configure GP2/GP3 outputs (both off); called by setup().
    void statusTick();     ///< Advance the state machine + animation; called by loop().

} // namespace SimGateway
```

---

## Key Data Structures

### Parser State Machine

```cpp
enum class ParserState : uint8_t { IDLE, GOT_AA, IN_FRAME };
```

| State | Trigger | Action | Next state |
|-------|---------|--------|------------|
| `IDLE` | byte ≤ 0x7F | Forward byte to USB CDC | `IDLE` |
| `IDLE` | byte == 0xAA | — | `GOT_AA` |
| `GOT_AA` | byte == 0x55 | Arm 4-byte frame buffer, reset buffer index | `IN_FRAME` |
| `GOT_AA` | byte != 0x55 | Forward 0xAA + this byte to USB CDC | `IDLE` |
| `IN_FRAME` | each byte | Store in frame buffer | `IN_FRAME` |
| `IN_FRAME` | 4th byte stored | Parse controlId + value; dispatch; clear buffer | `IDLE` |

The state machine is a private member of the SimGateway implementation, persisted between `loop()` calls.

### HID Frame Wire Format

```
Byte 0:   0xAA          — magic byte 0 (bit 7 set — cannot appear in DCS-BIOS ASCII stream)
Byte 1:   0x55          — magic byte 1
Byte 2:   controlId[7:0]
Byte 3:   controlId[15:8]
Byte 4:   value[7:0]
Byte 5:   value[15:8]
```

Parsed as:

```cpp
uint16_t controlId = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
uint16_t value     = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
```

(buf[0–3] are the four post-magic bytes collected in `IN_FRAME` state)

---

## Implementation Notes

### Relay is non-buffering

Every byte from USB CDC (`Serial`) is written to UART immediately — no local buffer, no coalescing. Every UART byte that is not part of a HID frame is forwarded to USB CDC immediately. DCS-BIOS stream latency is USB CDC round-trip only — no additional SimGateway delay.

### OsJoystick.send() batching

A local `bool _anyFired = false` flag is reset at the top of `loop()`. Any matching HIDAxis, HIDButton, or HIDHatSwitch dispatch sets it `true`. After UART is fully drained, `_hidSend()` is called once if `_anyFired`. This prevents redundant USB HID reports when multiple HID frames arrive in the same iteration.

### USB enumeration race

TinyUSB on RP2040 silently drops HID reports if USB is not yet enumerated — no crash. HID frames arriving on UART before enumeration is complete are parsed and dispatched normally; the resulting `Joystick.Send()` calls are no-ops until the host is ready. Boot-time HID frame loss is safe: axes update on the next physical movement; buttons are not held at power-on.

### HIDAxis / HIDButton linked list construction order

Both classes self-register in their constructors into static linked lists. Declare all `HIDAxis` and `HIDButton` objects at file scope in the sketch (not inside functions) to ensure they are constructed before `SimGateway::loop()` is first called. C++ guarantees construction order within a single translation unit is top-to-bottom.

### TinyUSB HID backend (Adafruit_TinyUSB_Arduino)

**Selected.** `Adafruit_USBD_HID` with a custom descriptor embedded in `SimGateway.cpp` under
`#ifndef SIMGATEWAY_TEST`. Supports 8 axes, 128 buttons, and 4 hat switches — covering all 7
OpenSkyhawk axes with room to spare.

**Buffer size constraint:** `CFG_TUD_HID_EP_BUFSIZE = 64` is hardcoded unconditionally in
`tusb_config_rp2040.h` (framework-arduinopico). Build-flag overrides are silently ignored —
the `#define` in the header wins. `sendReport()` returns `false` without sending if
`sizeof(report) > 64`. Production report = 34 bytes. Never expand the report struct above
64 bytes. See `FirmwarePlan/07-simgateway-api.md#hid-platform-compatibility` for full
platform limits.

HID report layout (34 bytes):
- 16 bytes: 128 buttons, 1-bit each
- 2 bytes: 4 hat switches, 4-bit nibble each (0xF = centered/null)
- 16 bytes: 8 axes, 16-bit signed each (X/Y/Z/Rx/Ry/Rz/Slider/Dial)

Internal helpers (file-scope, not exposed to sketch):

```cpp
_hidSetAxis(index, value);    // axis index 0–7; value int16_t
_hidSetButton(index, pressed);// button index 0–127
_hidSetHat(index, direction); // hat index 0–3; direction 0–8
_hidSend();                   // flush HID report — called once per drain cycle
```

**Axis value mapping:** PanelGroup sends 0–65535 (unsigned 16-bit). `HIDAxis::dispatch()` converts:

```cpp
_hidSetAxis(_axisIndex, (int16_t)(value - 32768));
```

`value − 32768` maps: 0 → −32768 (full negative), 32768 → 0 (centre), 65535 → +32767 (full positive).

In test builds (`SIMGATEWAY_TEST`), all helpers are no-ops. No TinyUSB code is compiled or linked.
The sketch has no knowledge of the HID backend or value scaling.

### Status-LED state machine (issue #94)

Drives the two `Gateway_Bridge` board LEDs (GREEN = GP2, RED = GP3, active-high) with a
non-blocking `millis()` animator. `SimGateway::setup()` calls `statusLedBegin()`; `loop()` calls
`statusTick()` after the relay/drain. SimGateway-**local** engine — shares the animation
vocabulary with `STM32Board` (issue #93) but not the code (the shared `StatusLedAnimator` was not
adopted). The RP2040 module's onboard WS2812 is not used.

States (priority highest-first): `FAULT` (red fast 4 Hz) > `NO_HOST` (red solid) >
`STREAMING` (green solid) > `USB_IDLE` (green slow 1 Hz) > `INIT` (red slow, brief). Vocabulary:
OFF / SOLID / SLOW 1 Hz / FAST 4 Hz / ALT 500 ms / PULSE ~50 ms (PULSE disabled by default).

Signal sources:

- **STREAMING / USB_IDLE** — CDC→UART forward (relay step 1) timestamps the last PC byte;
  STREAMING while `now − lastCdcRx ≤ 500 ms`.
- **NO_HOST / INIT** — `TinyUSBDevice.mounted()` polled each tick; a sticky ever-mounted flag
  splits INIT (never mounted, `now < 2000 ms`) from NO_HOST (unplugged after a mount, or init
  window expired).
- **FAULT** — read from the `uart0` PL011 error register (`uart_get_hw(uart0)->rsr`, bits
  OE/BE/PE/FE), cleared on every read (write-to-clear). The arduino-pico `SerialUART` RX IRQ drops
  framing/parity bytes and ignores overrun without touching RSR, so RSR is the only reliable error
  source. `Serial1 == uart0` on this board.

**FAULT latch / recovery:** latched on any error and re-stamped while errors persist; clears only
when **both** (a) ≥ 2 s have elapsed since the last error and (b) an error-free byte arrived on
uart0 RX *after* the fault. A silent bus holds FAULT until clean data resumes.

**Design / testability:** the state-selection, animation-phase, and latch logic are pure functions
taking `now` as a parameter (no `millis()`, GPIO, TinyUSB, or register access inside) — only
`_applyLed()` touches the pins, and the mount poll + RSR read sit behind `#ifndef SIMGATEWAY_TEST`.
`SIMGATEWAY_TEST` builds add hooks (`statusInject` / `statusResolve` / `statusFaultStep` +
`statusState` / `statusAnim` / `statusRedLevel` / `statusGreenLevel` / `statusResetForTest`) that
drive the machine with injected inputs and read back the resolved state + captured pin levels,
mirroring the `feedByte` / `cdcCapture` pattern. Verified by `tests/led_state/`.

### HIDControls.h — standalone library

`HIDControls.h` defines CTRL_* controlId constants. It is extracted from `CANProtocol` into a dedicated platform-agnostic library so both STM32 (CANProtocol) and RP2040 (SimGateway sketches) can depend on it:

```
Firmware/Libraries/HIDControls/
├── HIDControls.h    ← CTRL_ROLL, CTRL_PITCH, … CTRL_ZOOM, CTRL_TRIGGER, etc.
└── library.json     ← "platforms": "*"
```

Both `CANProtocol` and SimGateway sketches list `file://../../Libraries/HIDControls` in their `lib_deps`. `CANProtocol.h` includes `<HIDControls.h>` so existing STM32 sketches that include `<CANProtocol.h>` gain access without change.

---

## Boot Sequence

The sketch owns only HID registrars and the `setup()`/`loop()` calls. All hardware
initialisation is encapsulated in `SimGateway::setup()`.

```cpp
#include <SimGateway.h>
#include <HIDControls.h>

OpenSkyhawk::HIDAxis roll(CTRL_ROLL, 0);
// ... other HIDAxis / HIDButton declarations

void setup() {
    SimGateway::setup(Serial1);
    // Internally: USB identity set, Serial1.begin(250000), TinyUSB HID enumerated
}

void loop() {
    SimGateway::loop();
}
```

SimGateway has no boot handshake with the CAN cluster. It is stateless with respect to PanelBridge or PanelGroup boot state.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| earlephilhower/arduino-pico | PlatformIO platform | TinyUSB CDC (`Serial`), `HardwareSerial`, RP2040 Arduino core |
| Adafruit TinyUSB Library | `adafruit/Adafruit TinyUSB Library` | Custom HID descriptor; 8 axes, 128 buttons, 4 hats; embedded in SimGateway.cpp |
| HIDControls | `Firmware/Libraries/HIDControls` | Standalone platform-agnostic library; CTRL_* constants for sketch declarations |
