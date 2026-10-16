# NodeStatus — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/04-dcs-bios-integration.md#node-status-reporting`, `FirmwarePlan/00-decisions.md#d14`, `#d19`
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
- **`GATEWAY_NODE_ID`** (0) and the `_NODE_STATUS` row layout — `NODE_STATUS_ROW_BYTES` (14) and
  `NodeStatusField` (byte offset of each row field; hex index = 2 × offset).
- **`HeartbeatFlag`** (BOFF, EPVF) + **`HB_ESR_TEC_SHIFT` / `HB_ESR_REC_SHIFT`** — the HB_n `flags`
  bits and `esr` byte split.
- **`NodeHealthFlag`** — HEALTH_n flag bits (OVERHEAT, DEGRADED) mirroring `NodeHealthPayload.flags`;
  **`DIE_TEMP_UNAVAILABLE`** (−128 = `INT8_MIN`) — the `dieTempC` "unavailable / not seen" value.
- **`NodeFaultCode`** — the compact `faultId` dictionary (cross-node: I2C_PERIPHERAL, OVER_VOLTAGE,
  UNDER_VOLTAGE, SHORT_CIRCUIT, HOST_LINK_LOST, DUPLICATE_NODE_ID, …). Client maps id → label
  (SkyHawkClient#40).
- **`NodeKind`** (#344) — the `HEALTH_n` byte-5 board kind (UNKNOWN / PANEL_GROUP / PDU / BRIDGE),
  append-only, forwarded as `_NODE_STATUS` v3's last field. Set by each node library's role via
  `CANProtocol::serviceNodeStatus(kind, …)`, never from the NODE_ID.
- **`FaultSource`** — the interface a fault-producing object implements + a self-registering list.
- **`NODE_TELEM_*` + the TELEM_n page vocabulary (#216)** — `NODE_TELEM_MSG_NAME` (`_NODE_TELEM`) and
  `NODE_TELEM_PROTO_VERSION` (1, independent of `_NODE_STATUS`); `TelemPage` (global, append-only
  page ids — PDU_RAIL_12V / PDU_RAIL_5V / PDU_STATUS); `PduRailFlag` and `PduStatusFlag` (the page
  flag bytes); `PduRailField` / `PduStatusField` (each page's byte offsets); `NTC_TEMP_UNAVAILABLE`
  (−32768 = `INT16_MIN`); `NODE_TELEM_ROW_BYTES` (9) + `NodeTelemField`. Here, not in a PDU header, so the client's `sync-a4ec.ts` generates them from the same
  contract file (SkyHawkClient#61).

**Every wire value is a named declaration (#346)** — `enum class X : T { NAME = literal }` or
`constexpr T NAME = literal` — never only a comment, a bare literal or a format string, because the
client's `sync-a4ec.ts` can read only those two forms. Sentinels are written as literals
(`-128`, `-32768`) with a `static_assert` against `INT8_MIN` / `INT16_MIN`. Row-field enums say `ID`,
not `NODE_ID`: every node build defines `NODE_ID` as a macro.

`NodeHealthPayload` (the CAN HEALTH_n frame struct) stays in `CANProtocol.h`. Fault **detail
strings** stay local (DiagSerial) and never go on the wire or into any shared/HID header.

---

## File Layout

```
Firmware/Libraries/NodeStatus/
├── NodeStatus.h    ← NODE_STATUS_* + row/page layouts + HeartbeatFlag + NodeHealthFlag + NodeFaultCode + NodeKind + FaultSource
├── NodeStatus.cpp  ← FaultSource intrusive-list statics
└── library.json    ← platforms: *
```

---

## Public API

```cpp
#define NODE_STATUS_PROTO_VERSION 3      // bump on ANY _NODE_STATUS wire change (v3: nodeKind + node-0 row, #344)
#define NODE_STATUS_REQ_ADDR      0x86FE
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"
#define NODE_TELEM_PROTO_VERSION  1      // bump on a _NODE_TELEM framing / existing-page layout change
#define NODE_TELEM_MSG_NAME       "_NODE_TELEM"

constexpr uint8_t GATEWAY_NODE_ID       = 0;   // the bridge: HB_0 / HEALTH_0, first roster row
constexpr uint8_t NODE_STATUS_ROW_BYTES = 14;  // 28 hex
enum class NodeStatusField : uint8_t {         // row byte offsets; multi-byte fields MSB-first
    ID = 0, PRESENT = 1, HB_FLAGS = 2, UPTIME = 3, RX_COUNT = 5, ESR = 7,
    DIE_TEMP_C = 9, HEALTH_FLAGS = 10, FAULT_MASK = 11, FAULT_ID = 12, NODE_KIND = 13,
};
constexpr uint8_t NODE_TELEM_ROW_BYTES  = 9;   // 18 hex
enum class NodeTelemField : uint8_t { ID = 0, FRAME = 1 };

enum class HeartbeatFlag : uint8_t { BOFF = 0x01, EPVF = 0x02 };
constexpr uint8_t HB_ESR_TEC_SHIFT = 0;        // HB_n esr: TEC low byte
constexpr uint8_t HB_ESR_REC_SHIFT = 8;        //           REC high byte

enum class NodeHealthFlag : uint8_t { OVERHEAT = 0x01, DEGRADED = 0x02 };
constexpr int8_t DIE_TEMP_UNAVAILABLE = -128;  // == INT8_MIN, static_assert'd

enum class NodeFaultCode : uint8_t {
    NONE = 0, I2C_PERIPHERAL = 1, OVER_VOLTAGE = 2, UNDER_VOLTAGE = 3,
    SHORT_CIRCUIT = 4, HOST_LINK_LOST = 5,
    FUSE_OPEN = 6, OVER_TEMPERATURE = 7, SENSOR_FAULT = 8,  // PDU (#216)
    DUPLICATE_NODE_ID = 9,  // another board on this node's HB_n (#344); 0x0A+ reserved
};

enum class NodeKind : uint8_t {   // HEALTH_n byte 5 (#344), append-only
    UNKNOWN = 0, PANEL_GROUP = 1, PDU = 2, BRIDGE = 3,  // 0x04+ reserved
};

// TELEM_n page layouts (#216): byte offsets into the 8-byte frame, little-endian (byte 0 = TelemPage)
enum class PduRailField   : uint8_t { FLAGS = 1, V_AVG_MV = 2, V_MIN_MV = 4, I_AVG_MA = 6 };
enum class PduStatusField : uint8_t { FLAGS = 1, NTC_TEMP_DC = 2, I12_PEAK_MA = 4, I5_PEAK_MA = 6 };
constexpr int16_t NTC_TEMP_UNAVAILABLE = -32768;  // == INT16_MIN, static_assert'd

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
a time; multi-fault priority is a later concern. The one exception sits outside the registry:
`DUPLICATE_NODE_ID` is raised by `CANProtocol::serviceNodeStatus()` itself and outranks every
`FaultSource` (D19) — registry order can't guarantee a priority. Each node's health-TX calls it, packs the result
into `HEALTH_n` (via `makeNodeHealthPayload`, which derives DEGRADED), and edge-logs `faultDetail()`
via the shared `STM32Board::logNodeFaultEdge(tag, fault, detail)` helper (one prev-fault static;
DiagSerial only, on change). **Degraded drives no status-LED** — frame + DiagSerial only
(WARNING-latch arbitration deferred).

**Model (D14):** fault sources feed a node-level aggregator; no producer "owns" node health.
`DrumDisplay` is *one* `FaultSource` (I2C); a PDU rail monitor and a PanelBridge host-link watchdog
are others. Implementers report **cached** state only (cheap/const, no blocking I/O) — the aggregator
runs on the periodic health path. **Adding a fault code:** append a `NodeFaultCode` value + mirror
the client label. **Adding a node kind:** append a `NodeKind` value and pass it from the new node
library's `serviceNodeStatus()` call — no proto bump (the field already exists). **Changing the `_NODE_STATUS` wire shape:** bump `NODE_STATUS_PROTO_VERSION`.
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
(incl. overheat+degraded coexistence) and the `nodeKind` byte; `CANProtocol` duplicate_node_id test
covers `DUPLICATE_NODE_ID` raise/hold/clear; `PanelBridge` node_status test exercises the v3
`_NODE_STATUS` emission (kind `00` before HEALTH, kind delta, node-0 row); `PanelBridge` node_telem test exercises `_NODE_TELEM` forwarding; `PowerMonitor` tests
cover the PDU fault sources and page flags; the client `reference.test.ts` asserts the proto-version
contract.

---

## Dependencies

`<stdint.h>` only. `platforms: *`; consumers resolve it via `lib_extra_dirs` + LDF (a project whose
*test source* references the vocabulary directly lists `NodeStatus` in `lib_deps` so it is on the
src include path — see `Firmware/Tests/DrumDisplay/platformio.ini`).
