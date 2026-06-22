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

Be honest about what's built. Firmware is at the end of **Phase 3**, hardware-verified on
2026-06-12 (11/11 tests pass).

!!! note "What's implemented"
    - PlatformIO templates for all three tiers
    - `CANProtocol`, `STM32Board`, `HIDControls`
    - PanelBridge backbone (DCS-BIOS integration, `SYNC_REQ`, `TEST_SEQ`)
    - SimGateway (HID demux: `HIDAxis`, `HIDButton`, `HIDHatSwitch`)
    - `PinRef` abstraction, PanelGroup core, MCP23017 management
    - **LED** output and **Switch2Pos** input (both confirmed on PinRef)

!!! warning "Not yet implemented"
    - **Phase 4 input types** — Switch3Pos, SwitchMultiPos, ActionButton, AnalogInput,
      RotaryEncoder, RotaryAcceleratedEncoder, RotarySwitch, AnalogMultiPos, AngleSensorInput,
      SwitchWithCover2Pos
    - **Phase 5 output types** — AnalogOutput, IntegerOutput. (`NeedleGauge` is
      **implemented** and supersedes the former SwitecX25Output / AccelStepperOutput / ServoOutput.)
    - **Phase 6** — the Center_Armament PanelGroup sketch rewrite and end-to-end integration

    See [Control Types](control-types.md) for the full list with status.

## In this section

- **[PlatformIO Setup](platformio-setup.md)** — start a firmware project from the templates
- **[NODE_ID & CAN Addressing](node-id.md)** — the NODE_ID scheme, registry, and how to claim one
- **[DCS-BIOS Integration](dcsbios-integration.md)** — `DCSIN_*` IDs, the A4EC headers, output addresses
- **[HID Controls](hid-controls.md)** — the `CTRL_*` axis/button/hat allocations
- **[Control Types](control-types.md)** — every input and output class, with Phase status
- **[Debugging on STM32](debugging.md)** — DiagSerial, the status LED, and bench gotchas
