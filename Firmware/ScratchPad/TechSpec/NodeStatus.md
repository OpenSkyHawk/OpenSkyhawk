# NodeStatus — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/04-dcs-bios-integration.md#node-status-reporting`, `FirmwarePlan/02-can-protocol.md`
**Depends on:** `<stdint.h>` (fixed-width base for the `NodeFaultId` enum) — otherwise platform-agnostic

---

## Responsibility

Header-only library holding the **PanelBridge node-status host contract** and the **node fault
dictionary** — split out of `HIDControls.h` (which is HID controlId constants only) because this
is a separate concern: how PanelBridge reports connected PanelGroup nodes + their health to the
host (OpenSkyhawk Client) over DCS-BIOS.

- `NODE_STATUS_*` — the reserved DCS-BIOS identifiers (proto version, request address, message
  names) for the `_NODE_STATUS` report. **This header is the canonical contract source the
  client's `sync-a4ec.ts` parses** (`node-status.generated.ts`); `reference.test.ts` asserts the
  proto version matches, so a firmware bump fails the client loudly.
- `NodeFaultId` — the coarse `HEALTH_n` `faultId` dictionary (#163). One canonical source firmware
  emits and the client maps to human labels (SkyHawkClient#40).

Shared between STM32 (via CANProtocol / PanelBridge) and RP2040 (SimGateway relays the ASCII).
Header-only, `platforms: *`.

---

## File Layout

```
Firmware/Libraries/NodeStatus/
├── NodeStatus.h    ← NODE_STATUS_* defines + NodeFaultId enum
└── library.json    ← header-only, platforms: *
```

Consumers resolve it via `lib_extra_dirs` + PlatformIO's LDF (include-scan), matching the repo's
existing convention for `HIDControls`. Included by `PanelBridge.cpp` (NODE_STATUS_*) and
`DrumDisplay.h` (NodeFaultId). `NodeHealthPayload` (the CAN HEALTH_n frame struct) stays in
`CANProtocol.h` with the other CAN wire structs — only the host contract + fault enum live here.

---

## Public API

```cpp
// NodeStatus.h — proto v2 node-status contract + fault dictionary
#define NODE_STATUS_PROTO_VERSION 2          // bump on ANY _NODE_STATUS wire change
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

The `_NODE_STATUS` argument wire format (26 hex chars: `nodeId present flags uptime rxCount esr
dieTempC hFlags faultMask faultId`) is documented in the header comment and in
`FirmwarePlan/04-dcs-bios-integration.md`.

**Adding a fault code:** append a `NodeFaultId` value (no wire/proto change — `faultId` is a byte)
and mirror the label in the client's fault table (SkyHawkClient#40). **Changing the `_NODE_STATUS`
wire shape:** bump `NODE_STATUS_PROTO_VERSION` and update the client decoder + `SUPPORTED_NODE_PROTO`.

---

## No test project

Compile-time constants + one enum. Verified indirectly: `PanelBridge` node_status test exercises
the `_NODE_STATUS` emission; `DrumDisplay` breaker test asserts `NodeFaultId` via `faultCode()`;
the client's `reference.test.ts` asserts the proto version contract.

---

## Dependencies

`<stdint.h>` only (base type for the enum). `#pragma once`, no other headers, no platform restriction.
