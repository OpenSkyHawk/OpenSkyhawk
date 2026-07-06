# HIDControls — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/07-simgateway-api.md#hid-controlid-allocation`, `FirmwarePlan/02-can-protocol.md`
**Depends on:** *(none — platform-agnostic constants only)*

---

## Responsibility

Header-only library defining CTRL_* controlId constants for all HID axes and buttons.
Shared between STM32 (via CANProtocol) and RP2040 (SimGateway sketches).

Contains only `#define` constants — no classes, no functions, no state. Not a compile
unit; `library.json` marks it as header-only with no platform restriction. The node-status
host contract + fault vocabulary (`NODE_STATUS_*`, `NodeHealthFlag`, `NodeFaultCode`,
`FaultSource`) live in the neutral `NodeStatus` library (see `NodeStatus.md`), not here — this
header is HID controlId constants only.

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

> The node-status host contract (`NODE_STATUS_*`) and the `NodeFaultCode` fault vocabulary are
> **not** here — they live in the neutral `NodeStatus` library (`NodeStatus.md`). This header is
> HID controlId constants only.

---

## Dependencies

None. `HIDControls.h` uses only `#pragma once` and integer literals.
