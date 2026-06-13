# OpenSkyhawk Firmware Plan

This directory contains the firmware specification for the OpenSkyhawk physical DCS A-4E
Skyhawk cockpit. Documents are split by contract boundary — each file owns one stable interface
and references others rather than repeating them.

## System Topology

```
PC
 ↕ USB HID          (joystick axes + buttons — flight controls only)
 ↕ USB CDC @ 250000 (DCS-BIOS full-state stream, raw bytes both directions)
RP2040 — SimGateway  [one per cockpit]
 ↕ UART @ 250000   (DCS-BIOS stream + HID frames, bidirectional)
STM32F103 — PanelBridge  [one per cockpit — runs DCS-BIOS library]
 ↕ CAN bus @ 500 kbps
 ├── PanelGroup #1   (e.g. Center_Armament)
 ├── PanelGroup #2   (e.g. Left_ECM)
 └── PanelGroup #N   (up to ~20 nodes)
```

## Document Map

| File | Owns |
|------|------|
| `00-decisions.md` | Architecture decision records — *why* each choice was made |
| `01-system-overview.md` | Project goal, topology, library roles, data flow narrative |
| `02-can-protocol.md` | CAN frame IDs, `NODE_ID`, `ControlPacket` struct, TX queue policy |
| `03-uart-usb-hid-protocol.md` | UART multiplexing, HID frame wire format, parser resync, USB identity |
| `04-dcs-bios-integration.md` | `controlId` address space, `DCSIN_*` rationale, `DcsBiosInputEntry` struct, generator rules |
| `05-panelgroup-api.md` | `PinRef`, MCP23017 management, all input classes, all output classes |
| `06-panelbridge-api.md` | Node tracking, `ExportStreamListener`, input dispatch, SYNC_REQ triggers |
| `07-simgateway-api.md` | `HIDAxis`, `HIDButton`, relay contract, HID dispatch logic |
| `08-hardware-firmware-contracts.md` | STM32 pin assignments, DRV8833 ~SLEEP contract, ADS1115 assumptions |
| `09-startup-resync-diagnostics.md` | Boot sequences (all three nodes), SYNC_REQ/READY handshake, DIAG frames |
| `10-implementation-plan.md` | Phased task list (Phases 1–6) |
| `11-open-issues.md` | Deferred decisions, conditional revisit items, cross-document discrepancies |
| `examples/panelgroup-center-armament.md` | Full PanelGroup sketch with data flow |
| `examples/panelbridge.md` | Full PanelBridge sketch |
| `examples/simgateway.md` | Full SimGateway sketch with data flow |

## How to Use These Documents

**Reading order for new contributors:**
1. `00-decisions.md` — understand the *why* before the *what*
2. `01-system-overview.md` — get the big picture
3. The contract file for the layer you are working on (02–09)
4. Relevant example in `examples/`

**When adding a feature:**
- Identify which contract file owns the interface you are changing.
- Update that file first; update cross-references in other files if the interface changes.
- Do not duplicate constants or behaviour — add a "See `xx-filename.md#section`" cross-reference.

**CAN IDs are defined only in `02-can-protocol.md`.** Other files reference them with
"See `02-can-protocol.md#frame-type-table`" rather than restating values.

## Source of Truth

This directory is the authoritative firmware specification. `CLAUDE.md` and
`docs/_source/architecture.md` link here and defer to this directory for all firmware
decisions. If there is ever a conflict between a `docs/_source/` file and a FirmwarePlan
document, the FirmwarePlan wins.

`Firmware/ScratchPad/REQUIREMENTS.md` is the original monolithic PRD this directory was split
from. It is superseded by the files here.
