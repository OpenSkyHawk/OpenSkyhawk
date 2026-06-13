# 01 — System Overview

**Owns:** project goal, hardware topology diagram, library roles table, data flow narrative.
**Does not own:** any protocol details, API specs, or implementation decisions — those are in
the files linked from each section.

---

## Project Goal

OpenSkyhawk firmware connects a physical DCS A-4E Skyhawk home cockpit to DCS World via
DCS-BIOS. The firmware runs across three categories of nodes:

| Node type | MCU | Count |
|-----------|-----|-------|
| SimGateway | RP2040 | 1 (per cockpit) |
| PanelBridge | STM32F103CBT6 | 1 (per cockpit) |
| PanelGroup | STM32F103CBT6 | up to ~20 |

---

## Hardware Topology

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
 ├── ...
 └── PanelGroup #N   (up to ~20 nodes)
```

Each PanelGroup board may serve one primary panel directly (switches/LEDs wired to STM32 GPIO
or MCP23017) and one or more breakout sub-panels connected via JST-XH I²C harness.

---

## Libraries and Roles

| Library | MCU | Role |
|---------|-----|------|
| `STM32Board` | STM32F103 | Hardware init (CAN, UART, status LED). Used by PanelGroup and PanelBridge. |
| `CANProtocol` | Both | Shared packet structs (`ControlPacket`, `ControlPacketPair`), CAN ID constants, `controlId` namespace. |
| `PanelBridge` | STM32F103 | CAN master + DCS-BIOS processor. Runs the DCS-BIOS library on UART. Broadcasts all DCS outputs over CAN. Receives CAN EVTs and routes to DCS-BIOS or HID. |
| `PanelGroup` | STM32F103 | CAN sub-node. Dispatches output objects from DCS-BIOS state received over CAN. Polls input objects and fires CAN EVTs. |
| `SimGateway` | RP2040 | USB byte relay + HID. Forwards raw DCS-BIOS stream between USB CDC and UART. Intercepts HID frames for HID dispatch. Does not run the DCS-BIOS library. |

---

## Data Flows

### DCS → Cockpit (output path)

DCS state changes → DCS-BIOS binary stream → USB CDC → SimGateway (byte relay) → UART →
PanelBridge (runs DCS-BIOS library, fires ExportStreamListener) → CAN CTRL_BCAST → all
PanelGroup nodes → LEDs, gauge needles, backlight.

### Cockpit → DCS (input path — DCS-BIOS controls)

Physical change → PanelGroup (input class fires CAN EVT with `0x8000 <= controlId <= 0x86FF`) →
PanelBridge (binary search in generated input map → `sendDcsBiosMessage()`) → ASCII on UART →
SimGateway (byte relay) → USB CDC → DCS.

### Cockpit → HID (input path — flight controls)

Physical change → PanelGroup (input class fires CAN EVT with `controlId < 0x8000`) →
PanelBridge (wraps in HID frame → UART) → SimGateway (intercepts HID frame →
HIDAxis/HIDButton → Joystick setters → `Joystick.send()`) → USB HID → DCS.

---

## Key Design Decisions

See `00-decisions.md` for the rationale behind each architectural choice. The most important
decisions affecting this overview:

- SimGateway is a pure byte relay — it does not parse DCS-BIOS. PanelBridge owns all
  DCS-BIOS processing.
- The `controlId` address space splits cleanly: `0x8000`-`0x86FF` → DCS-BIOS path;
  `< 0x8000` → HID path; reserved IDs such as `0xFFFF` are invalid for input routing.
  See `04-dcs-bios-integration.md`.
- CAN frame types, IDs, and queue policy are in `02-can-protocol.md`.
- The UART+HID frame protocol is in `03-uart-usb-hid-protocol.md`.
