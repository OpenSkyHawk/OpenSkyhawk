# System Overview

OpenSkyhawk uses a three-tier firmware architecture to connect physical cockpit panels to
DCS. Each tier has a distinct role: the top tier handles USB and HID, the middle tier
manages the CAN bus cluster, and the bottom tier drives panel hardware directly.

## Architecture Diagram

![OpenSkyhawk System Architecture](../assets/images/diagrams/system-architecture.svg)

## Firmware Tiers

### SimGateway (RP2040)

The top tier runs on a Raspberry Pi Pico or compatible RP2040 module. It has two
simultaneous roles over a single USB connection to the PC:

- **DCS-BIOS serial**: receives cockpit state updates from DCS (switch positions, gauge
  values) and forwards them over UART to PanelBridge as `ControlPacket` structs
- **HID joystick**: presents flight stick axes and buttons directly to the PC as a USB
  HID device — no DCS-BIOS involvement for flight controls

Cockpit inputs travel in the reverse direction: PanelGroup → CAN → PanelBridge → UART →
SimGateway → `sendDcsBiosMessage()` → DCS.

### PanelBridge (STM32, CAN master)

The middle tier runs on an STM32F103CBT6 and owns the CAN bus. It:

- Receives `ControlPacket` frames from SimGateway over UART and broadcasts them to all
  PanelGroup nodes over CAN (`CONTROL_BROADCAST`)
- Collects input events from PanelGroup nodes over CAN and forwards them upstream to
  SimGateway over UART

NODE_ID is always 0 (reserved for the master).

### PanelGroup (STM32, CAN sub-nodes)

The bottom tier. Each physical panel group has its own STM32F103CBT6 running the
PanelGroup library. Nodes receive `CONTROL_BROADCAST` frames and dispatch them to
registered output objects (LEDs, stepper gauges). Input objects (switches, pots) fire
input events back over CAN.

Each node has a unique `NODE_ID` (1–63) assigned at compile time via `platformio.ini`.
See [NODE_ID registry](../claude/firmware-node-ids.md) for assignments.

## CAN Bus

500 kbps, two-wire differential (CANH/CANL), daisy-chained across all boards. 120 Ω
termination resistors at the two physical endpoints only. The SN65HVD230 transceiver
(SOIC-8, 3.3 V) is used on every STM32 board.

**Key finding:** Blue Pill STM32 clones require SJW = 4TQ for reliable operation —
see [Design Decisions](../architecture/design-decisions.md#can-bus-timing--sjw-must-be-4tq-on-blue-pill-clones).
