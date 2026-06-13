# Architecture Overview

OpenSkyhawk connects a physical A-4E-C cockpit to DCS over a single USB cable. Behind that
cable is a three-tier firmware stack joined by a CAN bus. This section explains how that
stack is built and — just as importantly — *why* it's built that way.

The short version: a [SimGateway](sim-gateway.md) on an RP2040 owns the one USB connection
to the PC. It relays the DCS-BIOS stream to a [PanelBridge](firmware-tiers.md) (STM32), which
runs the DCS-BIOS library and acts as CAN master. The PanelBridge distributes everything to
[PanelGroup](firmware-tiers.md) nodes (STM32) over the [CAN bus](can-bus.md), one node per
panel group. Every control — switch, knob, gauge, axis — travels the bus in the same
`ControlPacket` format and is routed by its `controlId`.

If you're new here, start with [How the System Works](../getting-started/system-overview.md)
for the big picture, then come back to this section for the detail.

## What's in this section

- **[Design Decisions](design-decisions.md)** — the *why*. The failure modes and empirical
  data behind the architecture: why the STM32 USB was abandoned, why AutoRetransmission is
  disabled, why SJW is 4TQ. Read this before proposing architectural changes.
- **[Firmware Tiers](firmware-tiers.md)** — the three-tier model in detail: what each tier
  does, what it deliberately does *not* do, and the contracts between them.
- **[CAN Bus Protocol](can-bus.md)** — the full protocol reference: bus configuration, frame
  ID table, `ControlPacket` wire format, the startup/sync handshake, and the gotchas.
- **[SimGateway](sim-gateway.md)** — how the USB gateway works: composite USB device, HID
  profile, and the byte-relay contract.
- **[DCS-BIOS vs HID](dcsbios-vs-hid.md)** — the decision rule every panel author must read
  before implementing a control: when to use the DCS-BIOS path and when to use HID.
- **[Power Architecture](power.md)** — how 12 V, 5 V, and 3.3 V are distributed across boards.

These pages are reference material — read the one you need. The two you should read in full
before contributing firmware are [Design Decisions](design-decisions.md) and
[DCS-BIOS vs HID](dcsbios-vs-hid.md).
