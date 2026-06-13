---
name: full-stack
description: OpenSkyhawk end-to-end system integration — how a control flows across SimGateway, PanelBridge, PanelGroup, CAN, the PCB, and DCS. Use for cross-layer questions, onboarding/orientation, "how does X reach DCS", tracing a signal across layers, debugging that spans firmware + hardware, or deciding which layer owns a behaviour. Hands off to pcb-design / firmware / panel-mapping for depth.
---

# Full-Stack Integration

The glue between disciplines. Use this to understand how the pieces connect; defer to the
domain skills (`pcb-design`, `firmware`, `panel-mapping`, `cad`) for depth.

## The pipeline (two directions)

**Input — physical control → DCS:**
```
switch/pot on panel
  → wired to MCP23017 GPIO or ADS1115 channel        (pcb-design owns the wiring)
  → read by a PanelGroup input class on a PinRef      (firmware)
  → ControlPacket on CAN @ 500k, NODE_ID-tagged       (02-can-protocol)
  → PanelBridge dispatches to a DCS-BIOS input         (06-panelbridge-api, 04-dcs-bios)
  → UART 250k → SimGateway → USB CDC → DCS
```

**Output — DCS → panel indicator:**
```
DCS-BIOS export stream → SimGateway → UART → PanelBridge
  → broadcast on CAN → PanelGroup output class (LED / stepper / gauge) → hardware
```

**HID (flight controls only)** bypass DCS-BIOS: stick/throttle/pedals → SimGateway → USB HID
joystick directly. Throttle *panel* switches are DCS-BIOS (Left Console); the throttle *axis*
is the HID `CTRL_THROTTLE` lever.

## Layer ownership (who owns what)

| Concern | Owner |
|---|---|
| Panel geometry, control inventory & types, DCS-BIOS IDs | `panel-mapping` |
| Wiring, GPIO/ADC allocation, board layout & verification | `pcb-design` |
| Reading/writing controls, CAN protocol, NODE_ID, DCS-BIOS dispatch | `firmware` |
| Panel/bezel 3D models, mechanical fit | `cad` |

## Read for the big picture

- `Firmware/ScratchPad/FirmwarePlan/01-system-overview.md` — topology + data-flow narrative
- `Firmware/ScratchPad/FirmwarePlan/00-decisions.md` — why the architecture is shaped this way
- Published: `docs/getting-started/system-overview.md`, `docs/architecture/*`

Key fixed facts: UART 250000 baud (RP2040 GP0/GP1 ↔ STM32 PA3/PA2); CAN 500 kbps; HID frame
6 bytes (magic `0xAA 0x55`, controlId+value uint16 LE); NODE_ID 0 = PanelBridge, 1–63 = nodes.
When a detail matters, jump to the owning skill rather than restating it here.
