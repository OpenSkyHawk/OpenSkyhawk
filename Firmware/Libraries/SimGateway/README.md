# SimGateway

RP2040 USB HID + DCS-BIOS relay gateway library for OpenSkyhawk.

Owns:
- **USB CDC ↔ UART relay** — forwards the DCS-BIOS byte stream between PC and PanelBridge transparently
- **HID frame demultiplexer** — intercepts `0xAA 0x55` binary frames on the UART RX path; dispatches `controlId` + `value` to registered `HIDAxis` / `HIDButton` objects
- **Joystick.Send() batching** — one HID report per `loop()` iteration after draining all UART bytes

Does **not** run DCS-BIOS. Does **not** parse DCS-BIOS addresses. Has no CAN knowledge.

Platform: **RP2040 only.**

## Dependencies

- `HIDControls` — CTRL_* controlId constants (platform-agnostic, shared with STM32)
- `earlephilhower/arduino-pico` platform — TinyUSB CDC, HardwareSerial, RP2040 Arduino core
- `multigamesystem/MGS-Pico-Joystick` — 8-axis / 128-button HID joystick (built-in only supports 6 axes)

## Usage

```cpp
#include <SimGateway.h>
#include <HIDControls.h>

// Declare HID controls at file scope — constructed before setup()
OpenSkyhawk::HIDAxis  roll(CTRL_ROLL,    0);
OpenSkyhawk::HIDAxis  pitch(CTRL_PITCH,  1);
OpenSkyhawk::HIDButton trigger(CTRL_TRIGGER, 0);

void setup() {
    SimGateway::setup(Serial1); // GP0 TX / GP1 RX @ 250000 baud
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
- `0x0020–0x009F` — HID buttons → `HIDButton` dispatch → `Joystick.SetButton()`

Axis value mapping: 0–65535 (from PanelGroup) → ±32767 (`value − 32768` internally).

## API

| Function | Description |
|---|---|
| `SimGateway::setup(uart)` | Init USB identity, Joystick, UART |
| `SimGateway::loop()` | Relay CDC↔UART, parse HID frames, Send() |

See `SimGateway.h` for full Doxygen documentation.
