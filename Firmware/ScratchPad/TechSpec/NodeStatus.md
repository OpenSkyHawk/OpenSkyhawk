# NodeStatus — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/04-dcs-bios-integration.md#node-status-reporting`, `FirmwarePlan/00-decisions.md#d14`
**Depends on:** `<stdint.h>` (enum base type) — otherwise platform-agnostic

---

## Responsibility

The neutral **node-status contract** every OpenSkyhawk node shares — PanelGroup, PanelBridge, and
the PDU are all nodes. Split out of `HIDControls.h` (HID controlId constants only) and `CANProtocol.h`
(CAN frame transport) because node status is a separate, cross-node concern — and broader than
"health": it owns the `_NODE_STATUS` DCS-BIOS reporting layer.

Owns:
- **`NODE_STATUS_*`** — the reserved DCS-BIOS identifiers for the `_NODE_STATUS` host report (proto
  version, request address, message names). **The canonical contract source the client's
  `sync-a4ec.ts` parses**; `reference.test.ts` asserts the proto version so a firmware bump fails
  the client loudly.
- **`NodeHealthFlag`** — HEALTH_n flag bits (OVERHEAT, DEGRADED) mirroring `NodeHealthPayload.flags`.
- **`NodeFaultCode`** — the compact `faultId` dictionary (cross-node: I2C_PERIPHERAL, OVER_VOLTAGE,
  UNDER_VOLTAGE, SHORT_CIRCUIT, HOST_LINK_LOST, …). Client maps id → label (SkyHawkClient#40).
- **`FaultSource`** — the interface a fault-producing object implements + a self-registering list.

`NodeHealthPayload` (the CAN HEALTH_n frame struct) stays in `CANProtocol.h`. Fault **detail
strings** stay local (DiagSerial) and never go on the wire or into any shared/HID header.

---

## File Layout

```
Firmware/Libraries/NodeStatus/
├── NodeStatus.h    ← NODE_STATUS_* + NodeHealthFlag + NodeFaultCode + FaultSource
├── NodeStatus.cpp  ← FaultSource intrusive-list statics
└── library.json    ← platforms: *
```

---

## Public API

```cpp
#define NODE_STATUS_PROTO_VERSION 2      // bump on ANY _NODE_STATUS wire change
#define NODE_STATUS_REQ_ADDR      0x86FE
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"

enum class NodeHealthFlag : uint8_t { OVERHEAT = 0x01, DEGRADED = 0x02 };

enum class NodeFaultCode : uint8_t {
    NONE = 0, I2C_PERIPHERAL = 1, OVER_VOLTAGE = 2, UNDER_VOLTAGE = 3,
    SHORT_CIRCUIT = 4, HOST_LINK_LOST = 5,  // 0x06+ reserved
};

namespace OpenSkyhawk {
class FaultSource {                                      // a fault-producing object implements this
public:
    virtual NodeFaultCode faultCode() const { return NodeFaultCode::NONE; }  // cached state; NONE = healthy
    virtual const char*   faultDetail() const { return ""; } // DiagSerial only — never on the wire
    static FaultSource*   head();                            // self-registered list, walked by the
    FaultSource*          next() const;                      //   node aggregator (PR-3)
protected:
    FaultSource();                                          // registers into the list (permanent)
    ~FaultSource() = default;                               // protected, non-virtual: a base/mixin
};
} // namespace OpenSkyhawk
```

**Lifetime:** a `FaultSource` must have **static/global lifetime** (same rule as `OutputBase`/
`InputBase`). Registration is permanent — there is no unregister — so a stack/local `FaultSource`
would leave a dangling pointer in the registry the aggregator walks. `faultCode()` returns the
typed `NodeFaultCode`; the cast to `uint8_t` happens only at the CAN/DCS-BIOS packing boundary
(HEALTH_n / `_NODE_STATUS`).

**Model (D14):** fault sources feed a node-level aggregator; no producer "owns" node health.
`DrumDisplay` is *one* `FaultSource` (I2C); a PDU rail monitor and a PanelBridge host-link watchdog
are others. Implementers report **cached** state only (cheap/const, no blocking I/O) — the aggregator
runs on the periodic health path. **Adding a fault code:** append a `NodeFaultCode` value + mirror
the client label. **Changing the `_NODE_STATUS` wire shape:** bump `NODE_STATUS_PROTO_VERSION`.

---

## Test

No standalone project. Verified indirectly: `DrumDisplay` breaker test asserts `faultCode()`/
`faultDetail()` via the `FaultSource` override; `PanelBridge` node_status test exercises the
`_NODE_STATUS` emission; the client `reference.test.ts` asserts the proto-version contract.

---

## Dependencies

`<stdint.h>` only. `platforms: *`; consumers resolve it via `lib_extra_dirs` + LDF (a project whose
*test source* references the vocabulary directly lists `NodeStatus` in `lib_deps` so it is on the
src include path — see `Firmware/Tests/DrumDisplay/platformio.ini`).
