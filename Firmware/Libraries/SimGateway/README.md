# SimGateway

RP2040 USB HID + DCS-BIOS relay gateway library for OpenSkyhawk.

Owns:
- **USB CDC ↔ UART relay** — forwards the DCS-BIOS byte stream between PC and PanelBridge transparently
- **HID frame demultiplexer** — intercepts `0xAA 0x55` binary frames on the UART RX path; dispatches `controlId` + `value` to registered `HIDAxis` / `HIDButton` objects
- **HID report batching** — one TinyUSB HID report sent per `loop()` iteration after draining all UART bytes

Does **not** run DCS-BIOS. Does **not** parse DCS-BIOS addresses. Has no CAN knowledge.

Platform: **RP2040 only.**

## Dependencies

- `HIDControls` — CTRL_* controlId constants (platform-agnostic, shared with STM32)
- `earlephilhower/arduino-pico` platform — TinyUSB CDC, HardwareSerial, RP2040 Arduino core
- `adafruit/Adafruit TinyUSB Library` — custom HID descriptor with 8 axes, 128 buttons, 4 hat switches

## Usage

```cpp
#include <SimGateway.h>
#include <HIDControls.h>

// Declare HID controls at file scope — constructed before setup()
OpenSkyhawk::HIDAxis  roll(CTRL_ROLL,    0);
OpenSkyhawk::HIDAxis  pitch(CTRL_PITCH,  1);
OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER, 0);

void setup() {
    SimGateway::setup(Serial1); // USB identity + UART + TinyUSB HID enumeration
}

void loop() {
    SimGateway::loop(); // relay + dispatch + Send()
}
```

## USB identity

| Field | Value |
|---|---|
| Manufacturer | OpenSkyhawk |
| Product | A-4E Skyhawk |
| VID | 0x2E8A (Raspberry Pi) |
| PID | 0x4134 |

## UART pins

The standard PanelBridge link uses RP2040 `Serial1` / UART0:

| Signal | RP2040 pin | PanelBridge STM32 pin |
|---|---|---|
| SimGateway TX -> PanelBridge RX | GP0 | PA3 / UART2 RX |
| SimGateway RX <- PanelBridge TX | GP1 | PA2 / UART2 TX |

`SimGateway::setup(Serial1)` configures GP0/GP1 before starting the UART. Board revisions with
different wiring can pass explicit `txPin` / `rxPin` arguments to `setup()`.

## HID frame wire format

Sent by PanelBridge over UART to SimGateway for HID axis/button updates:

```
Byte 0:   0xAA  (magic — bit 7 set, cannot appear in DCS-BIOS ASCII stream)
Byte 1:   0x55  (magic)
Byte 2:   controlId[7:0]
Byte 3:   controlId[15:8]
Byte 4:   value[7:0]
Byte 5:   value[15:8]
```

controlId ranges (from `HIDControls.h`):
- `0x0010–0x001F` — HID axes → `HIDAxis` dispatch → `Joystick.SetAxis()`
- `0x0020–0x002F` — HID hat switches → `HIDHatSwitch` dispatch → `Joystick.SetHat()`
- `0x0030–0x00AF` — HID buttons → `HIDButton` dispatch → `Joystick.SetButton()`
- `0x00B0–0x00FF` — reserved HID expansion slots

Axis value mapping: 0–65535 (from PanelGroup) → ±32767 (`value − 32768` internally).

## API

| Function | Description |
|---|---|
| `SimGateway::setup(uart)` | Init USB identity, TinyUSB HID, UART |
| `SimGateway::loop()` | Relay CDC↔UART, parse HID frames, Send() |

See `SimGateway.h` for full Doxygen documentation.

## Platform support

Verified against all target platforms with the `hid_stress` test sketch. Production descriptor
(8 axes / 128 buttons / 4 hats / **34 bytes**) is within all platform limits.

| Platform | Axes | Buttons | Hats | Notes |
|---|---|---|---|---|
| DCS / DirectInput (Windows) | **8** ✓ | **128** ✓ | **4** ✓ | Full DIJOYSTATE2 match. Hats as `JOY_BTN_POVn_*` bindings. |
| Chrome Gamepad API (macOS) | 8 + 1 hat-as-axis | **32** | — | IOKit inserts hat 0 as axis at usage 0x39 position. 32-button cap is not a concern for DCS. |
| Windows joy.cpl | 8 | 128 | 4 | Same as DirectInput. |

**TinyUSB hard limit:** `CFG_TUD_HID_EP_BUFSIZE = 64` bytes, hardcoded in
`tusb_config_rp2040.h`. Build-flag overrides are silently ignored. `sendReport()` returns
`false` without sending if `sizeof(report) > 64`. Never expand the production report above
64 bytes.

## Testing

SimGateway tests run on an RP2040 device and inject synthetic HID-frame bytes, as if those bytes
arrived from PanelBridge over UART. They do **not** read local ADC pins, I2C ADCs, sensors, CAN,
or physical cockpit controls.

Parser/dispatch tests may use `SIMGATEWAY_TEST` hooks such as `feedByte()` to inject bytes
directly into the demultiplexer. UART integration tests may instead jumper `Serial1` TX to
`Serial1` RX and write the same frames through the hardware UART before calling
`SimGateway::loop()`. The default loopback jumper is GP0 -> GP1.

`cdc_forwarding` verifies the relay side of the parser contract: ordinary UART bytes are forwarded
to USB CDC in order, valid HID frames are consumed and not forwarded, and `0xAA` followed by a
non-`0x55` byte forwards both bytes before resuming scan.

Test builds define `SIMGATEWAY_TEST`, activating no-op stubs for all TinyUSB HID helpers
directly inside `SimGateway.cpp`. No external shim file is needed. Production builds compile
the full `Adafruit_USBD_HID` descriptor and report struct in the same file.

`uart_loopback` verifies the same parser and forwarding behavior through the configured physical
UART pins by writing bytes to `Serial1` TX and reading them back through `Serial1` RX.

ADC and sensor normalisation belongs in PanelGroup / AnalogInput tests. SimGateway receives only
already-normalised `uint16_t` HID payloads.
