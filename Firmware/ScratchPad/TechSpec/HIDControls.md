# HIDControls — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/07-simgateway-api.md#hid-controlid-allocation`, `FirmwarePlan/02-can-protocol.md`
**Depends on:** `<stdint.h>` (fixed-width base type for the `NodeFaultId` enum) — otherwise platform-agnostic

---

## Responsibility

Header-only library defining CTRL_* controlId constants for all HID axes and buttons.
Shared between STM32 (via CANProtocol) and RP2040 (SimGateway sketches).

Contains `#define` constants plus the `NodeFaultId` enum (node fault codes, #163) — no
functions, no state. Includes `<stdint.h>` for the enum's `uint8_t` base. Not a compile
unit; `library.json` marks it as header-only with no platform restriction.

Also hosts the **NODE_STATUS wire contract** (`NODE_STATUS_PROTO_VERSION`, `_REQ_ADDR`,
`_MSG_NAME`, `_END_MSG_NAME`) — the canonical source the client syncs against — and
`NodeFaultId`, the canonical `HEALTH_n` `faultId` dictionary (see below).

---

## File Layout

```
Firmware/Libraries/HIDControls/
├── HIDControls.h    ← all CTRL_* constants
└── library.json     ← header-only, platforms: *
```

### library.json

```json
{
  "name": "HIDControls",
  "version": "0.1.0",
  "frameworks": "arduino",
  "platforms": "*",
  "build": {
    "srcFilter": "-<*>"
  }
}
```

`"platforms": "*"` makes this library available on both `ststm32` (CANProtocol, PanelGroup
sub-node sketches) and `raspberrypi` (SimGateway sketch) without any per-project workaround.

### No test project

HIDControls contains only compile-time constants. Correctness is verified indirectly:

- `CANProtocol` tests confirm CTRL_* values appear in range `CTRL_ID_HID_MIN`–`CTRL_ID_HID_MAX`
- `SimGateway` tests confirm HIDAxis dispatches on the expected controlId values

---

## Public API

```cpp
// HIDControls.h

#pragma once

// ── HID axes — controlId range 0x0010–0x001F (16 slots) ─────────────────────

#define CTRL_ROLL       0x0010  // Roll axis      — stick sub-node (AS5600 / pot)
#define CTRL_PITCH      0x0011  // Pitch axis     — stick sub-node (AS5600 / pot)
#define CTRL_THROTTLE   0x0012  // Throttle lever — throttle sub-node (ADC)
#define CTRL_RUDDER     0x0013  // Rudder axis    — pedal sub-node (ADC)
#define CTRL_BRAKE_L    0x0014  // Left toe brake — pedal sub-node (ADC)
#define CTRL_BRAKE_R    0x0015  // Right toe brake — pedal sub-node (ADC)
#define CTRL_ZOOM       0x0016  // Zoom axis      — throttle sub-node (ADC)
// 0x0017–0x001F: reserved for future axes

// ── HID hat switches — controlId range 0x0020–0x002F (16 slots) ─────────────

#define CTRL_HAT_0      0x0020  // Hat switch 0    — stick grip (4-way / 8-way)
// 0x0021–0x002F: reserved for future hat switches

// ── HID buttons — controlId range 0x0030–0x00AF (128 slots) ─────────────────

#define CTRL_TRIGGER    0x0030  // Trigger (button index 0) — stick grip
// 0x0031–0x00AF: additional buttons added here as stick/throttle grip is catalogued
// 0x00B0–0x00FF: reserved for future HID expansion beyond the current USB report
```

**Adding new constants:** append to the appropriate range in `HIDControls.h`. Assign the
next unused value in the axis (0x0010–0x001F), hat (0x0020–0x002F), or button
(0x0030–0x00AF) range. Update
`HIDAxis` or `HIDButton` declarations in the relevant sketch.

### Node-status + fault contract (PanelBridge → host)

HIDControls.h also owns the node-status wire contract and the fault-code dictionary — the
canonical source the client (`sync-a4ec.ts`) mirrors:

```cpp
#define NODE_STATUS_PROTO_VERSION 2          // bump on any _NODE_STATUS wire change
#define NODE_STATUS_REQ_ADDR      0x86FE     // host→device roster-request export address
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"

// HEALTH_n faultId values (#163). Coarse, one active at a time; the exact device is logged
// on the node's DiagSerial, not the wire. Client maps id → human label (SkyHawkClient#40).
enum class NodeFaultId : uint8_t {
    NONE           = 0x00,
    I2C_PERIPHERAL = 0x01,   // an I2C device (OLED/mux/expander) tripped its I2cHealth breaker
    // 0x02–0xFF reserved for future fault sources
};
```

Add new fault codes by appending an enum value (no wire/proto change — `faultId` is a byte);
mirror the label in the client's fault table.

---

## Dependencies

`<stdint.h>` only (fixed-width base for the `NodeFaultId` enum). Otherwise `#pragma once`,
integer literals, string literals, and one enum — no other headers, no platform restriction.
