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
- **`NODE_TELEM_*` + the TELEM_n page vocabulary (#216)** — `NODE_TELEM_MSG_NAME` (`_NODE_TELEM`) and
  `NODE_TELEM_PROTO_VERSION` (1, independent of `_NODE_STATUS`); `TelemPage` (global, append-only
  page ids — PDU_RAIL_12V / PDU_RAIL_5V / PDU_STATUS); `PduRailFlag` and `PduStatusFlag` (the page
  flag bytes). Here, not in a PDU header, so the client's `sync-a4ec.ts` generates them from the same
  contract file (SkyHawkClient#61).

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
#define NODE_TELEM_PROTO_VERSION  1      // bump on a _NODE_TELEM framing / existing-page layout change
#define NODE_TELEM_MSG_NAME       "_NODE_TELEM"

enum class NodeHealthFlag : uint8_t { OVERHEAT = 0x01, DEGRADED = 0x02 };

enum class NodeFaultCode : uint8_t {
    NONE = 0, I2C_PERIPHERAL = 1, OVER_VOLTAGE = 2, UNDER_VOLTAGE = 3,
    SHORT_CIRCUIT = 4, HOST_LINK_LOST = 5,
    FUSE_OPEN = 6, OVER_TEMPERATURE = 7, SENSOR_FAULT = 8,  // PDU (#216); 0x09+ reserved
};

namespace OpenSkyhawk {
class FaultSource {                                      // a fault-producing object implements this
public:
    virtual NodeFaultCode faultCode() const { return NodeFaultCode::NONE; }  // cached state; NONE = healthy
    virtual const char*   faultDetail() const { return ""; } // DiagSerial only — never on the wire
    static FaultSource*   head();                            // self-registered list, walked by the
    FaultSource*          next() const;                      //   node aggregator (aggregateFaults)
protected:
    FaultSource();                                          // registers into the list (permanent)
    ~FaultSource() = default;                               // protected, non-virtual: a base/mixin
};

// Node fault aggregator: primary NodeFaultCode = first non-NONE FaultSource in registry order.
// detailOut (if given) receives that source's faultDetail(), NEVER null — normalized to "" when a
// source returns null or when none faulted. NONE when the node is healthy.
NodeFaultCode aggregateFaults(const char** detailOut = nullptr);
} // namespace OpenSkyhawk
```

**Lifetime:** a `FaultSource` must have **static/global lifetime** (same rule as `OutputBase`/
`InputBase`). Registration is permanent — there is no unregister — so a stack/local `FaultSource`
would leave a dangling pointer in the registry the aggregator walks. `faultCode()` returns the
typed `NodeFaultCode`; the cast to `uint8_t` happens only at the CAN/DCS-BIOS packing boundary
(HEALTH_n / `_NODE_STATUS`).

**Aggregation & priority:** `aggregateFaults()` returns the **first non-`NONE`** `faultCode()` in
**registry iteration order, which is REVERSE construction order** — the intrusive list pushes new
sources at `head()`, so the *last-constructed* source is visited first. One fault is on the wire at
a time; multi-fault priority is a later concern. Each node's health-TX calls it, packs the result
into `HEALTH_n` (via `makeNodeHealthPayload`, which derives DEGRADED), and edge-logs `faultDetail()`
via the shared `STM32Board::logNodeFaultEdge(tag, fault, detail)` helper (one prev-fault static;
DiagSerial only, on change). **Degraded drives no status-LED** — frame + DiagSerial only
(WARNING-latch arbitration deferred).

**Model (D14):** fault sources feed a node-level aggregator; no producer "owns" node health.
`DrumDisplay` is *one* `FaultSource` (I2C); a PDU rail monitor and a PanelBridge host-link watchdog
are others. Implementers report **cached** state only (cheap/const, no blocking I/O) — the aggregator
runs on the periodic health path. **Adding a fault code:** append a `NodeFaultCode` value + mirror
the client label. **Changing the `_NODE_STATUS` wire shape:** bump `NODE_STATUS_PROTO_VERSION`.
**Adding a TELEM page:** append a `TelemPage` value (never renumber) and document its layout in
FirmwarePlan 02 — additive, no bump; changing an existing page's layout bumps
`NODE_TELEM_PROTO_VERSION`.

---

## Test

`Firmware/Tests/NodeStatus/` (`test_aggregate`) covers `aggregateFaults()` over static mock
`FaultSource`s: healthy → `NONE` + `""`, one/two faults → primary in reverse-construction order +
its detail, null `faultDetail()` normalized to `""`, `nullptr` detailOut accepted. Also verified
indirectly: `DrumDisplay` breaker test asserts `faultCode()`/`faultDetail()` via the `FaultSource`
override; `CANProtocol` `health_payload`/`health_flags` tests assert the faultId/DEGRADED packing
(incl. overheat+degraded coexistence); `PanelBridge` node_status test exercises the `_NODE_STATUS`
emission; `PanelBridge` node_telem test exercises `_NODE_TELEM` forwarding; `PowerMonitor` tests
cover the PDU fault sources and page flags; the client `reference.test.ts` asserts the proto-version
contract.

---

## Dependencies

`<stdint.h>` only. `platforms: *`; consumers resolve it via `lib_extra_dirs` + LDF (a project whose
*test source* references the vocabulary directly lists `NodeStatus` in `lib_deps` so it is on the
src include path — see `Firmware/Tests/DrumDisplay/platformio.ini`).
