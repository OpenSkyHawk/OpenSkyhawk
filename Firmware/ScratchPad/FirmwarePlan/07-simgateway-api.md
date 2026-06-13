# 07 — SimGateway API

**Owns:** `HIDAxis`, `HIDButton`, and `HIDHatSwitch` class specs, SimGateway relay contract, HID
dispatch logic, `OsJoystick.send()` batching rule, USB identity, UART initialisation, TinyUSB HID
descriptor and report layout.
**Does not own:** HID frame wire format (→ 03), CAN protocol (→ 02), boot sequence (→ 09).

---

## Role

SimGateway is the single RP2040 module that bridges the PC to the CAN cluster:

- **USB CDC ↔ UART relay:** raw DCS-BIOS byte stream forwarded transparently in both
  directions. SimGateway does **not** run the DCS-BIOS library — it never parses or
  interprets the DCS-BIOS stream.
- **HID dispatch:** intercepts HID frames on UART RX (identified by `HID_MAGIC` — see
  `03-uart-usb-hid-protocol.md`); routes controlIds to `HIDAxis`, `HIDButton`, or `HIDHatSwitch`.

SimGateway has no knowledge of DCS-BIOS control names, addresses, or input map entries.

---

## HIDAxis

Declared in the SimGateway sketch. Receives 16-bit value (0–65535) from PanelGroup via
CAN → PanelBridge → HID frame → UART.

The constructor takes a `controlId` and a joystick **axis index** (0–7). The library maps the
incoming 0–65535 value to ±32767 (`value − 32768`) and writes it to the TinyUSB HID report
internally. The sketch has no knowledge of the HID backend or the scaling.

```cpp
// SimGateway sketch only:
OpenSkyhawk::HIDAxis roll    (CTRL_ROLL,     0);
OpenSkyhawk::HIDAxis pitch   (CTRL_PITCH,    1);
OpenSkyhawk::HIDAxis throttle(CTRL_THROTTLE, 2);
OpenSkyhawk::HIDAxis rudder  (CTRL_RUDDER,   3);
OpenSkyhawk::HIDAxis brakeL  (CTRL_BRAKE_L,  4);
OpenSkyhawk::HIDAxis brakeR  (CTRL_BRAKE_R,  5);
OpenSkyhawk::HIDAxis zoom    (CTRL_ZOOM,     6);
```

---

## HIDButton

Declared in the SimGateway sketch. `value != 0` → pressed; `value == 0` → released.
TinyUSB HID descriptor supports 128 buttons (index 0–127).

```cpp
// SimGateway sketch only:
OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER, 0);  // button index 0
```

---

## HIDHatSwitch

Declared in the SimGateway sketch. `value` encodes direction: 0 = centered, 1 = N, 2 = NE,
3 = E, 4 = SE, 5 = S, 6 = SW, 7 = W, 8 = NW. Values > 8 treated as centered.
TinyUSB HID descriptor supports 4 hat switches (index 0–3).

```cpp
// SimGateway sketch only:
OpenSkyhawk::HIDHatSwitch hat(CTRL_HAT_0, 0);  // hat index 0
```

---

## HID controlId Allocation

Defined in `HIDControls.h` (platform-agnostic library shared with CANProtocol):

| Range | Use | HID descriptor capacity |
|-------|-----|-------------------------|
| `0x0010`–`0x001F` | Axes (16 slots) | 8 axes (X/Y/Z/Rx/Ry/Rz/Slider/Dial) |
| `0x0020`–`0x002F` | Hat switches (16 slots) | 4 hat switches |
| `0x0030`–`0x00AF` | Buttons (128 slots) | 128 buttons |
| `0x00B0`–`0x00FF` | Reserved HID expansion | Not exposed by current USB report |

```cpp
// Axes
#define CTRL_ROLL     0x0010
#define CTRL_PITCH    0x0011
#define CTRL_THROTTLE 0x0012
#define CTRL_RUDDER   0x0013
#define CTRL_BRAKE_L  0x0014
#define CTRL_BRAKE_R  0x0015
#define CTRL_ZOOM     0x0016

// Hat switches (0x0020–0x002F reserved; no constants defined until needed)

// Buttons
#define CTRL_TRIGGER  0x0030   // first button slot
```

---

## HID Platform Compatibility

Verified via `hid_stress` test sketch (VID 0x2E8A / PID 0x4135). Production descriptor
(8 axes / 128 buttons / 4 hats / 34 bytes) is within all platform limits.

### TinyUSB report size hard limit

`CFG_TUD_HID_EP_BUFSIZE = 64` is hardcoded unconditionally in
`tusb_config_rp2040.h` (framework-arduinopico package). Build-flag overrides are silently
ignored. **Maximum HID report = 64 bytes.** Production report = 34 bytes.

Exceeding 64 bytes causes `Adafruit_USBD_HID::sendReport()` to return `false` without
sending — no error, no crash, just silent drop. Never expand the report struct above 64 bytes.

### Chrome Gamepad API (macOS)

| Control | Production descriptor | Chrome sees |
|---|---|---|
| GD axes (0x30–0x37) | 8 | 8 ✓ |
| Hats | 4 | 1 (hat 0 as virtual axis — see note) |
| Buttons | 128 | **32** (hard API cap) |
| Vendor axes (0xFF00 page) | 0 | 0 (ignored entirely) |

Hat insertion note: macOS IOKit inserts Hat Switch (usage 0x39) as a virtual axis at its
GD usage-number position — between Wheel (0x38) and Vx (0x40) — regardless of where the
hat appears in the descriptor. Hat null nibble (0xF = 15) normalises to
(15/7)×2−1 ≈ 3.286, outside [−1, +1]. Only hat 0 appears as a single axis.

32-button cap is not a concern: DCS uses DirectInput, not the Gamepad API.

### DirectInput / DCS (Windows)

DIJOYSTATE2 limits (confirmed in DCS A-4E-C Controls):

| Control | Production descriptor | DCS sees |
|---|---|---|
| GD axes (0x30–0x37) | 8 | **8** ✓ |
| Buttons | 128 | **128** ✓ |
| POV hats | 4 | **4** ✓ |
| Extended GD axes (0x38, 0x40–0x46) | 0 | 0 (ignored by DirectInput) |
| Vendor axes (0xFF00) | 0 | 0 |

DCS exposes each hat as 8 directional bindings: `JOY_BTN_POVn_U/D/L/R/UL/UR/DL/DR`.
Production descriptor is a full match for DIJOYSTATE2 — no limits are exceeded.

---

## Dispatch Logic

After draining all HID frames from UART in one `SimGateway::loop()` iteration:
1. Walk the `HIDAxis` linked list; for each matching `controlId` write `value − 32768` to the TinyUSB HID axis report field.
2. Walk the `HIDButton` linked list; for each matching `controlId` set/clear the corresponding button bit.
3. Walk the `HIDHatSwitch` linked list; for each matching `controlId` write the direction nibble.
4. If any setter fired, call `OsJoystick.send()` **once** to flush the HID report.

Calling `Send()` once per drain cycle (not once per frame) keeps HID report rate predictable.

---

## Relay Contract

`SimGateway::loop()` must not buffer, parse, or delay DCS-BIOS bytes. Every byte from
USB CDC is forwarded to UART immediately. Every byte from UART that is not part of a HID
frame is forwarded to USB CDC immediately.

This means `DcsBios::loop()` on PanelBridge receives the byte stream with USB CDC latency
only — no additional SimGateway processing latency.

---

## UART Pin Contract

The standard SimGateway link uses RP2040 `Serial1` / UART0:

| Direction | RP2040 pin | PanelBridge STM32 pin |
|-----------|------------|-----------------------|
| SimGateway TX → PanelBridge RX | GP0 | PA3 / UART2 RX |
| SimGateway RX ← PanelBridge TX | GP1 | PA2 / UART2 TX |

`SimGateway::setup(Serial1)` configures these defaults before starting the UART at 250000 baud.
Board revisions that move the UART must pass explicit `txPin` / `rxPin` arguments to
`SimGateway::setup()`, and their tests must exercise the same configured pins.

---

## Testing Contract

SimGateway tests run on a real RP2040 device. They simulate PanelBridge-originated UART traffic
with synthetic HID-frame bytes; they do not read local ADC pins, I2C ADCs, CAN, or physical
cockpit controls.

Parser tests may call a `SIMGATEWAY_TEST`-only byte-feed helper to inject bytes directly into
the HID demultiplexer. UART integration tests may instead jumper `Serial1` TX to `Serial1` RX and
write the same synthetic frames through the hardware UART before calling `SimGateway::loop()`.
The default loopback jumper is RP2040 `GP0 -> GP1`, matching the production UART pins above.

The values under test are already-normalised `uint16_t` payloads carried in HID frames, exactly
as PanelBridge would send after receiving CAN EVTs from PanelGroup nodes. ADC and sensor
normalisation belongs to PanelGroup/AnalogInput tests, not SimGateway tests.

---

## Sketch Structure

The sketch declares `HIDAxis`, `HIDButton`, and `HIDHatSwitch` registrars and calls
`setup()`/`loop()`. All TinyUSB HID setup, USB identity, baud rate, and value scaling are owned
by the library — not visible in the sketch.

```cpp
#include <SimGateway.h>
#include <HIDControls.h>

// HID declarations — only these are needed; DCS-BIOS routing is automatic
OpenSkyhawk::HIDAxis roll    (CTRL_ROLL,     0);
OpenSkyhawk::HIDAxis pitch   (CTRL_PITCH,    1);
OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER, 0);

void setup() {
    SimGateway::setup(Serial1);
    // SimGateway::setup() owns: USB identity, GP0/GP1 pin config,
    // Serial1.begin(250000), TinyUSB HID enumeration
}

void loop() {
    SimGateway::loop();   // relay USB CDC ↔ UART; intercept HID frames; OsJoystick.send() once
}
```

See `examples/simgateway.md` for the full sketch with data flow annotations.
