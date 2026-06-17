# 11 — Open Issues

**Owns:** resolved-design index and historical review notes.
**Does not own:** active requirements. Active requirements live in the owning contract files.

---

## Status

✅ **No active open issues.**

All previously listed items have been resolved and moved into the documents that own the
actual contract. This file is now a pointer map so resolved issues do not get re-opened by
accident during later review passes.

---

## Resolved Decisions

| Item | Resolution | Owner |
|------|------------|-------|
| PanelBridge heartbeat on CAN | **Implemented in #93.** PanelBridge transmits `HB_0` / `canIdHb(0)` every 500 ms as a master-liveness beacon; PanelGroup accepts it (`filterAcceptId(canIdHb(0))`) and raises WARNING if it stops for > 1500 ms after a master has been seen. | `02-can-protocol.md`, `06-panelbridge-api.md`, `08-hardware-firmware-contracts.md` |
| `ERROR_PASSIVE` status | No separate public `CanStatus` for now. `TX_ERROR` maps to STM32Board `CAN_ERROR`; `BUS_OFF` maps to `BUS_OFF`. EPVF remains available in heartbeat flags. (Re-confirmed #93 — deferred again.) | `02-can-protocol.md`, `STM32Board TechSpec` |
| CAN batching | `ControlPacketPair` batching is part of CANProtocol, not a conditional optimization. PanelBridge and PanelGroup submit individual `ControlPacket`s; single-packet frames use slot B `controlId = 0x0000` as the null sentinel. | `02-can-protocol.md`, `06-panelbridge-api.md` |
| New node state request | READY and dead/unseen → alive recovery both trigger a `SYNC_REQ` broadcast so the node re-polls and sends its current input state. | `06-panelbridge-api.md`, `09-startup-resync-diagnostics.md` |
| `SwitchWithCover2Pos` | Required for at least one panel; lower priority than the first core input/output pass. Generator skips unsupported controls until the class and input-map shape are implemented. | `05-panelgroup-api.md`, `10-implementation-plan.md`, `A4ECGenerator TechSpec` |
| `GENERATOR_GAPS.md` | A4EC generator always emits this file alongside generated headers. All skipped controls must be listed with a reason. | `A4ECGenerator TechSpec`, `10-implementation-plan.md` |
| RotarySwitch boot position | Passive DCS-BIOS-compatible behavior: initialize software position to 0 and re-sync when the user turns to an end stop. | `05-panelgroup-api.md` |
| DRV8833 `~SLEEP` | `~SLEEP` is HIGH from setup for homing torque and remains HIGH. | `05-panelgroup-api.md`, `08-hardware-firmware-contracts.md` |
| PS3/PS4 thumbstick prototype | Allowed for early stick tests; replace with AS5600/MT6701 for final hardware. | This file, prototyping note below |
| Stale `DCSInput` references | Cleanup task during implementation: scan headers and remove stale names. | `10-implementation-plan.md` |
| Accel encoder value encoding | Four compact CAN values map to DCS-BIOS arg strings in PanelBridge. Threshold confirmed at 175 ms. | `04-dcs-bios-integration.md`, `05-panelgroup-api.md` |
| SimGateway boot sequence | TinyUSB silently drops HID before enumeration; no special handling required. | `07-simgateway-api.md`, `09-startup-resync-diagnostics.md` |
| DIAG destination | DiagSerial only on each STM32 board; no SimGateway diagnostic multiplexing. | `09-startup-resync-diagnostics.md` |
| AngleSensorInput range | HID-routed axes use 0–65535 and SimGateway converts to signed joystick range. | `05-panelgroup-api.md`, `07-simgateway-api.md` |

---

## PS3/PS4 Thumbstick Module — Prototyping Note

A PS3/PS4 replacement thumbstick module (Alps RKJXU/RKJXV series) can substitute for
AS5600/MT6701 sensors for initial stick sub-node testing.

**Pinout (5-pin, left to right with pins facing down):** VCC · VRx · VRy · SW · GND

- Run at **3.3 V** — output range 0–3.3 V maps directly to STM32 ADC.
- Resting position outputs ~VCC/2 (~1.65 V = ~32767 out of 65535). Sketch dead-band must
  account for mid-scale centre, not zero.
- Wire VRx / VRy as `AnalogInput` via `PinRef(STM32_ADC_PIN)`. SW as `Switch2Pos`.
- **Limitation:** mechanical wear over time. Replace with AS5600/MT6701 for the final build.
