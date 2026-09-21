# Firmware Overview

OpenSkyhawk firmware runs across three tiers — a [SimGateway](../architecture/sim-gateway.md)
on an RP2040, a [PanelBridge](../architecture/firmware-tiers.md) STM32 acting as CAN master,
and one [PanelGroup](../architecture/firmware-tiers.md) STM32 per panel group. This section is
the practical reference for working on that firmware: how to set up a project, how addressing
works, what control types exist, and how to debug a board on the bench.

If you haven't yet, read [Firmware Tiers](../architecture/firmware-tiers.md) for the tier model
and [CAN Bus Protocol](../architecture/can-bus.md) for the wire protocol — this section
assumes them.

## Libraries

All firmware is built from shared libraries under `Firmware/Libraries/` — the **authoritative
source** for any constant or API. When a doc and a header disagree, the header wins.

| Library | MCU | Role |
|---------|-----|------|
| `CANProtocol` | STM32 | Packet structs (`ControlPacket`, `ControlPacketPair`), CAN IDs, the `controlId` namespace, TX/RX queues |
| `STM32Board` | STM32 | Hardware init — CAN, UART, the bi-color status LED. Used by both STM32 tiers |
| `HIDControls` | shared | HID `controlId` allocations (`CTRL_*`), shared between SimGateway and CANProtocol |
| `PanelGroup` | STM32 | CAN sub-node: `PinRef`, MCP23017 management, input and output control classes |
| `PanelBridge` | STM32 | CAN master + DCS-BIOS processor |
| `SimGateway` | RP2040 | USB byte relay + HID (`HIDAxis`, `HIDButton`, `HIDHatSwitch`) |
| `A4EC` | shared | Generated A-4E-C DCS-BIOS headers (`DCSIN_*` command IDs, `A_4E_C_*` output addresses) |

## Implementation status

Phases 0–3 are complete. For the A-4E-C, Phase 4's input pass is complete too: every input control
in the DCS-BIOS export has a class. Phase 5 needs one more output family (`Dimmer` +
`IntegerOutput`) for the five light-intensity outputs. The first tagged release, v0.1.0, closes
those gaps; v1.0.0 then freezes the sketch API and CAN wire format **before** the first full panel
(Phase 6, Right_Navigation) is built.

[Control Types](control-types.md) is the authority on per-class status, including which
classes are hardware-verified. The summary below is a pointer, not a second source.

!!! note "What's implemented"
    - PlatformIO templates for all three tiers
    - `CANProtocol`, `STM32Board`, `HIDControls`
    - PanelBridge backbone (DCS-BIOS integration, `SYNC_REQ`, `TEST_SEQ`)
    - SimGateway (HID demux: `HIDAxis`, `HIDButton`, `HIDHatSwitch`)
    - `PinRef` abstraction, PanelGroup core, MCP23017 management
    - Helpers — `ShiftBus` (shift-register expansion), `I2cMux`, `I2cHealth`
    - **Seven input classes** — `Switch2Pos`, `Switch3Pos`, `SwitchMultiPos`,
      `AnalogMultiPos`, `AnalogInput`, `RotaryEncoder` (REL/DIR), `ActionButton`
    - **Three output classes** — `LED`, `DrumDisplay`, `NeedleGauge`

!!! warning "Not yet implemented"
    - **For v0.1.0** — `RotaryAcceleratedEncoder` (a `RotaryEncoder` subclass with the DCS-BIOS
      momentum filter and speed-up), the `AnalogOutput` family (`Dimmer` for PWM backlight,
      `IntegerOutput` for a user callback), `AngleSensorInput` (an `AnalogInput` subclass for
      magnetic angle sensors) and `SwitchWithCover2Pos`. Every control class is locked in v0.1.0.
    - **After v1.0** — the `ServoMotor` gauge backend.
    - **Phase 6** — the Right_Navigation PanelGroup sketch and end-to-end integration.
    - `RotarySwitch` is **not** planned: `RotaryEncoder` in DIR mode drives bounded selectors
      without losing track of the sim's position at boot.

## In this section

- **[PlatformIO Setup](platformio-setup.md)** — start a firmware project from the templates
- **[Build Flags](build-flags.md)** — all 32 `-D` flags: ours, the framework's, and the test seams
- **[NODE_ID & CAN Addressing](node-id.md)** — the NODE_ID scheme, registry, and how to claim one
- **[DCS-BIOS Integration](dcsbios-integration.md)** — `DCSIN_*` IDs, the A4EC headers, output addresses
- **[HID Controls](hid-controls.md)** — the `CTRL_*` axis/button/hat allocations
- **[Control Types](control-types.md)** — every input and output class, with Phase status
- **[Debugging on STM32](debugging.md)** — DiagSerial, the status LED, and bench gotchas
