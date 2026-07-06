

# File HIDControls.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**HIDControls**](dir_8de7ffd664ed88ad14416481a318893f.md) **>** [**HIDControls.h**](HIDControls_8h.md)

[Go to the documentation of this file](HIDControls_8h.md)


```C++

#pragma once

// ── HID axes — controlId range 0x0010–0x001F (16 slots) ──────────────────────

#define CTRL_ROLL       0x0010  // Roll axis       — stick sub-node (AS5600 / pot)
#define CTRL_PITCH      0x0011  // Pitch axis      — stick sub-node (AS5600 / pot)
#define CTRL_THROTTLE   0x0012  // Throttle lever  — throttle sub-node (ADC)
#define CTRL_RUDDER     0x0013  // Rudder axis     — pedal sub-node (ADC)
#define CTRL_BRAKE_L    0x0014  // Left toe brake  — pedal sub-node (ADC)
#define CTRL_BRAKE_R    0x0015  // Right toe brake — pedal sub-node (ADC)
#define CTRL_ZOOM       0x0016  // Zoom axis       — throttle sub-node (ADC)
// 0x0017–0x001F: reserved for future axes

// ── HID hat switches — controlId range 0x0020–0x002F (16 slots) ──────────────
// value: 0 = centered, 1 = N, 2 = NE, 3 = E, 4 = SE, 5 = S, 6 = SW, 7 = W, 8 = NW

#define CTRL_HAT_0      0x0020  // Hat switch 0    — stick grip (4-way / 8-way)
// 0x0021–0x002F: reserved for additional hat switches

// ── HID buttons — controlId range 0x0030–0x00AF (128 slots) ──────────────────

#define CTRL_TRIGGER    0x0030  // Trigger (button index 0) — stick grip
// 0x0031–0x00AF: additional buttons added here as stick/throttle grip is catalogued
// 0x00B0–0x00FF: reserved for future HID expansion beyond the current USB report

// ── Range sentinels (used by CANProtocol and SimGateway for routing checks) ──

#define CTRL_ID_HID_MIN 0x0010  // First valid HID controlId
#define CTRL_ID_HID_MAX 0x00FF  // Last reserved HID controlId

// ── OpenSkyhawk reserved DCS-BIOS identifiers (node-status reporting, #86) ────
//
// Node presence/health is surfaced to the host (OpenSkyhawk Client) over the
// existing DCS-BIOS protocol, not a bespoke sideband. Owned by PanelBridge
// (SimGateway relays everything transparently). This header is the canonical
// contract source the client's sync-a4ec.ts parses — bump NODE_STATUS_PROTO_VERSION
// on any wire change so the client's sync assertion fails loudly.
//
//   NODE_STATUS_REQ_ADDR     host→device DCS-BIOS export address the client writes
//                         to request the roster. Above every real A-4E-C output
//                         (~0x8554), so DCS never exports it — no false trigger.
//   NODE_STATUS_MSG_NAME     device→host per-node status command name. Leading
//                         underscore — no A-4E-C control collides.
//   NODE_STATUS_END_MSG_NAME device→host burst terminator (request/boot replies).
//                         Argument = node count in the burst. Lets the client
//                         know a roster reply is complete and reconcile/prune.
//
// _NODE_STATUS argument: 26 chars, each field its numeric value as fixed-width
// uppercase hex (most-significant nibble first):
//   nodeId(2) present(2) flags(2) uptime(4) rxCount(4) esr(4)
//     dieTempC(2) hFlags(2) faultMask(2) faultId(2)
//   present: 01 alive, 00 removed.  flags: bit0 BOFF, bit1 EPVF.
//   esr: low byte TEC, high byte REC.  nodeId range 1–63.
//   uptime/rxCount are uint16 — wrap at 65535 (~18 h / 65 k frames). Treat as
//   health indicators, not monotonic counters.
//   The last four fields are the node's cached HEALTH_n telemetry (proto v2):
//     dieTempC: int8 two's-complement whole °C (internal MCU sensor — UNCALIBRATED,
//       die not ambient). 80 = INT8_MIN = not-yet-seen → render unknown.
//     hFlags:  node health bits — bit0 overheat, bit1 degraded.
//     faultMask/faultId: tripped-peripheral bitmap + detail id (0 until #163 populates;
//       faultId is an index into a client-side lookup table — no strings on the wire).
//
// Emission: a single bare _NODE_STATUS is a live delta (apply immediately). A
// request/boot reply is N _NODE_STATUS messages followed by _NODE_STATUS_END <count>;
// that set is the authoritative present-roster — prune nodes absent from it.
// Silent death (yank / bus-off) is reported as present=00 by PanelBridge's 3 s
// heartbeat timeout; a periodic client request reconciles any lost delta.
#define NODE_STATUS_PROTO_VERSION 2
#define NODE_STATUS_REQ_ADDR      0x86FE
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"
```


