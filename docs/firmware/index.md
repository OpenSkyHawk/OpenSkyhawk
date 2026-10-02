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
| `STM32Board` | STM32 | Hardware init — CAN, UART, the bi-color status LED, SWD-only debug (JTAG pins freed as GPIO). Used by both STM32 tiers |
| `HIDControls` | shared | HID `controlId` allocations (`CTRL_*`), shared between SimGateway and CANProtocol |
| `PanelGroup` | STM32 | CAN sub-node: `PinRef`, MCP23017 management, input and output control classes |
| `PanelBridge` | STM32 | CAN master + DCS-BIOS processor |
| `SimGateway` | RP2040 | USB byte relay + HID (`HIDAxis`, `HIDButton`, `HIDHatSwitch`) |
| `A4EC` | shared | Generated A-4E-C DCS-BIOS headers (`DCSIN_*` command IDs, `A_4E_C_*` output addresses) |

## Implementation status

Phases 0–5 are complete. For the A-4E-C that means every input *and* output control in the
DCS-BIOS export has a class — the last gap, the five light-intensity outputs, closed with the
`AnalogOutput` family (`Dimmer` + `IntegerOutput`). The class set is now complete, down to
`SwitchWithCover2Pos` for DCS-BIOS parity, and the first tagged release, v0.1.0, locks it; v1.0.0
then freezes the sketch API and CAN wire format **before** the first full panel (Phase 6,
Right_Navigation) is built.

[Control Types](control-types.md) is the authority on per-class status, including which
classes are hardware-verified. The summary below is a pointer, not a second source.

!!! note "What's implemented"
    - PlatformIO templates for all three tiers
    - `CANProtocol`, `STM32Board`, `HIDControls`
    - PanelBridge backbone (DCS-BIOS integration, `SYNC_REQ`, `TEST_SEQ`)
    - SimGateway (HID demux: `HIDAxis`, `HIDButton`, `HIDHatSwitch`)
    - `PinRef` abstraction, PanelGroup core, MCP23017 management
    - Helpers — `ShiftBus` (shift-register expansion), `I2cMux`, `I2cHealth`
    - **Ten input classes** — `Switch2Pos`, `Switch3Pos`, `SwitchWithCover2Pos`,
      `SwitchMultiPos`, `AnalogMultiPos`, `AnalogInput`, `AngleSensorInput`,
      `RotaryEncoder` (REL/DIR), `RotaryAcceleratedEncoder`, `ActionButton`
    - **Five output classes** — `LED`, `DrumDisplay`, `NeedleGauge`, `Dimmer` (PWM backlight)
      and `IntegerOutput` (your own callback), the last two over the `AnalogOutput` family base

!!! warning "Not yet implemented"
    - **After v1.0** — the `ServoMotor` gauge backend.
    - **Phase 6** — the Right_Navigation PanelGroup sketch and end-to-end integration.
    - `RotarySwitch` is **not** planned: `RotaryEncoder` in DIR mode drives selectors that have
      no marked position, without losing track of the sim's position at boot. Selectors with a
      pointer use `SwitchMultiPos` or `AnalogMultiPos`.

## In this section

- **[PlatformIO Setup](platformio-setup.md)** — start a firmware project from the templates
- **[Build Flags](build-flags.md)** — all 32 `-D` flags: ours, the framework's, and the test seams
- **[NODE_ID & CAN Addressing](node-id.md)** — the NODE_ID scheme, registry, and how to claim one
- **[DCS-BIOS Integration](dcsbios-integration.md)** — `DCSIN_*` IDs, the A4EC headers, output addresses
- **[HID Controls](hid-controls.md)** — the `CTRL_*` axis/button/hat allocations
- **[Control Types](control-types.md)** — every input and output class, with Phase status
- **[Debugging on STM32](debugging.md)** — DiagSerial, the status LED, and bench gotchas
