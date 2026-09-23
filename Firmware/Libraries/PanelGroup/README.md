# PanelGroup

CAN sub-node domain layer for OpenSkyhawk panel boards.

`PanelGroup` turns an STM32F103 into a CAN sub-node that receives the DCS-BIOS output stream
(wrapped as `ControlPacket` frames) from PanelBridge and dispatches it to registered output
objects. Inputs are polled, debounced and sent back over CAN as events. Heartbeats go out every
500 ms.

The design mirrors DCS-BIOS: input and output objects are declared at global scope in the sketch,
self-register through static linked lists, and are dispatched by `PanelGroup::loop()`. Class names
follow `dcs-bios-arduino-library` wherever it has an equivalent, and related classes form
**families** — a base class plus thin subclasses that change only the source or sink (FirmwarePlan
D16).

## Dependencies

- [STM32Board](../STM32Board/README.md)
- [CANProtocol](../CANProtocol/) — `ControlPacket`, CAN IDs, batching
- `A4EC` — generated `DCSIN_*` input IDs and `A_4E_C_*` output addresses

## Node ID

Each node's ID comes from the build, not a strap: `build_flags = -DNODE_ID=<1–63>` in the sketch's
`platformio.ini` (never `#define` it in `main.cpp` — library translation units need the value
too). See `docs/firmware/node-id.md` for the registry.

## Usage

```cpp
#include <OpenSkyhawk.h>   // PanelGroup + every concrete class + A4EC headers

// Output: LED from one bit of a DCS-BIOS word
OpenSkyhawk::LED gearLight(A_4E_C_GEAR_LIGHT, A_4E_C_GEAR_LIGHT_AM, PinRef(PB0));

// Input: debounced switch → EVT on CAN → PanelBridge → DCS-BIOS
OpenSkyhawk::Switch2Pos masterArm(DCSIN_ARM_MASTER, PinRef(PB5));

void setup() {
    STM32Board::setDebug(true);   // optional
    PanelGroup::setup();
}

void loop() {
    PanelGroup::loop();
}
```

## Input classes

| Class | DCS-BIOS equivalent | Status |
|---|---|---|
| `Switch2Pos` | `Switch2Pos` | implemented |
| `Switch3Pos` | `Switch3Pos` | implemented |
| `SwitchMultiPos` | `SwitchMultiPos` | implemented (`MultiPosInput` family) |
| `AnalogMultiPos` | `AnalogMultiPos` | implemented (`MultiPosInput` family) |
| `AnalogInput` | `Potentiometer` | implemented |
| `RotaryEncoder` | `RotaryEncoder` | implemented — REL (`variable_step`) and DIR (`fixed_step`) modes |
| `ActionButton` | `ActionButton` | implemented |
| `RotaryAcceleratedEncoder` | `RotaryAcceleratedEncoder` | implemented (hardware-verified) — `RotaryEncoder` subclass: momentum filter + fast-detent step |
| `AngleSensorInput` | — | planned (#294) — `AnalogInput` subclass |
| `SwitchWithCover2Pos` | `SwitchWithCover2Pos` | planned (#293) — `Switch2Pos` subclass |

`RotarySwitch` is deliberately not ported: `RotaryEncoder` in DIR mode drives bounded selectors
without losing the sim's position at boot.

## Output classes

| Class | DCS-BIOS equivalent | Status |
|---|---|---|
| `LED` | `LED` | implemented — `(value & mask) != 0` → pin on |
| `NeedleGauge` | `ServoOutput` (closest) | implemented — any pointer gauge, over the `Drivers/` `MotorDriver` layer |
| `AnalogOutput` | — | implemented — family base: matching, decoding, change dedup |
| `Dimmer` | `Dimmer` | implemented — PWM duty on a GPIO timer pin (checked at `configure()`) |
| `IntegerOutput` | `IntegerBuffer` | implemented — user callback |

`DrumDisplay` (OLED rolling-drum readouts) is a separate opt-in library:
[`../DrumDisplay`](../DrumDisplay/).

## controlId routing

Routing is decided by the `controlId` an input is constructed with, not by its class:

| Range | Destination |
|---|---|
| `DCSIN_*` — `0x8000`–`0x86FF` | PanelBridge → `sendDcsBiosMessage()` → DCS |
| `CTRL_*` — `< 0x8000` | SimGateway → USB HID report |

For outputs (DCS → panel), the `controlId` is the DCS-BIOS output address — no translation table.

## API

See [`PanelGroup.h`](PanelGroup.h) for the full Doxygen documentation.

| Function | Description |
|---|---|
| `PanelGroup::setup()` | Configure registered inputs/outputs and expanders, start CAN |
| `PanelGroup::loop()` | Drain CAN, dispatch outputs, poll inputs, send heartbeat |
| `PanelGroup::registerExpander(chip, intaPin, intbPin)` | Register an MCP23017 with interrupt pins (per sketch) |
| `PanelGroup::registerExpander(chip)` | Register an MCP23017 in polling mode |
| `PanelGroup::registerADC(adc, addr, wire)` | Register an ADS1115 |

Per-class reference: `docs/firmware/control-types.md`; authoritative specs:
`Firmware/ScratchPad/TechSpec/PanelGroup/`.
