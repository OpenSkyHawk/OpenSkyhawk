# 04 — DCS-BIOS Integration

**Owns:** `controlId` address space, `DCSIN_*` constant rationale, `DcsBiosInputEntry` struct,
input map generator rules, value encoding by type, output address constants.
**Does not own:** CAN frame IDs (→ 02), HID frame wire format (→ 03), PanelBridge boot/dispatch
orchestration (→ 06), PanelGroup input class specs (→ 05).

---

## controlId Address Space

The `controlId` field in a `ControlPacket` determines routing. Routing decisions happen at two
points: **PanelBridge** (DCS-BIOS inputs) and **SimGateway** (HID inputs).

| Range | Type | Routed by | Destination |
|-------|------|-----------|-------------|
| `0x0010`–`0x00FF` | HID axes and buttons | SimGateway | `HIDAxis` / `HIDButton` → USB HID report |
| `0x8000`–`0x86FF` | DCS-BIOS compact command IDs (`DCSIN_*`) | PanelBridge | Input map lookup → `sendDcsBiosMessage()` |
| `0xFFFF` | Reserved: TEST_SEQ trigger | — | See `02-can-protocol.md` |

---

## Node-Status Reporting (#86)

PanelBridge surfaces every board on the bus — connected nodes and, since v3 (#344), the bridge
itself — with its health and board kind to the host (OpenSkyhawk Client)
**over the existing DCS-BIOS protocol**, using reserved A-4E-C-namespace identifiers. No
bespoke sideband; SimGateway relays both directions verbatim (the request is binary export it
forwards host→device; the response is ASCII it forwards device→host). PanelBridge owns it.

| Reserved ID | Direction | Purpose |
|-------------|-----------|---------|
| Export address `0x86FE` (`NODE_STATUS_REQ_ADDR`) | host→device | Client writes it (value ignored) to request the roster. Above every real A-4E-C output (~`0x8554`), so DCS never exports it. Excluded from the CAN broadcast in `handleDcsBiosExport()`. |
| Command name `_NODE_STATUS` (`NODE_STATUS_MSG_NAME`) | device→host | One DCS-BIOS command message per node (the bridge's own node-0 row included). Leading underscore — no A-4E-C control collides; DCS-BIOS ignores it as an unknown control if a copy leaks. |
| Command name `_NODE_STATUS_END` (`NODE_STATUS_END_MSG_NAME`) | device→host | Burst terminator for request/boot replies; argument = row count, node 0 included. Lets the client know a roster reply is complete and reconcile/prune. |

**Request** — client writes a DCS-BIOS export frame to `0x86FE`. PanelBridge's
`NodeStatusReqListener` (an `ExportStreamListener`) fires → emits the full roster.

**Response** — `_NODE_STATUS <hex>` where `<hex>` is **28 chars** (proto **v3**, #344; v2 was 26 chars
without `nodeKind`) — a `NODE_STATUS_ROW_BYTES` (14) byte row, **each field its numeric value as
fixed-width uppercase hex (most-significant nibble first)**:
`nodeId(2) present(2) flags(2) uptime(4) rxCount(4) esr(4) dieTempC(2) hFlags(2) faultMask(2) faultId(2) nodeKind(2)`
(the 8-byte `HeartbeatPayload` plus `present`, then the node's cached `HEALTH_n` fields). Each field's
byte offset in the row is named by `NodeStatusField` in `NodeStatus.h` (hex index = 2 × offset).
`present`: `01` alive, `00` removed. `flags`: `HeartbeatFlag` — bit0 BOFF, bit1 EPVF. `esr`: low byte
TEC, high byte REC (`HB_ESR_TEC_SHIFT` / `HB_ESR_REC_SHIFT`). `nodeId` 0–63: `GATEWAY_NODE_ID` (0) is
the bridge's own row, 1–63 the CAN nodes. `uptime`/`rxCount` are uint16 —
wrap at 65535 (~18 h / 65 k frames); treat as health indicators, not monotonic counters.

**Node-0 row (v3).** Every roster reply starts with the bridge's own row: `present` `01`, its
heartbeat state (`flags`/`uptime`/`esr` read live; `rxCount` `0000` — the bridge keeps no RX
count), and the health fields of the `HEALTH_0` it last sent, so `nodeKind` `03` (bridge). Before
its first `HEALTH_0` (~1 s after boot — e.g. the boot seed) the row reads not-yet-seen like any
node: `dieTempC` `80`, `nodeKind` `00`. The row only appears in roster replies, never as a live
delta: the bridge is the one reporting, so it is present whenever anything arrives.

The last five fields come from the node's `HEALTH_n` frame (see `02-can-protocol.md`):
- `dieTempC`: int8 two's-complement whole °C (internal MCU sensor — UNCALIBRATED, die not ambient).
  `80` = `DIE_TEMP_UNAVAILABLE` (`INT8_MIN`) = not-yet-seen → render unknown.
- `hFlags`: node health bits — bit0 overheat, bit1 degraded.
- `faultId`: `NodeFaultCode` — `00` = no fault, else the node's primary fault, populated per node by
  `aggregateFaults()` (#163); the client maps it to a label (no strings on the wire).
- `faultMask`: fault source/domain bitmap — stays `00`, reserved for future fault-domain bits.
- `nodeKind` (#344): `NodeKind` — `00` unknown, `01` panel group, `02` PDU, `03` bridge (append-only).
  Announced by the board's firmware role, never derived from `nodeId`. `00` before the node's first
  `HEALTH_n` (same rule as `dieTempC` `80`) and from pre-#344 firmware. The client adopts a new node
  only on `01` or `02`. A `faultId` of `09` (`DUPLICATE_NODE_ID`) means two boards share this
  `nodeId`; every other field in the row is then a blend of both boards.

**Emission semantics:**
- A single bare `_NODE_STATUS` is a **live delta** — emitted on each node alive/dead transition
  (`present` 01/00), and (v3) when an alive node's cached `nodeKind` changes. The alive delta goes
  out on a node's first `HB_n`, before its first `HEALTH_n`, so it reads kind `00`; the kind delta
  follows ~1 s later, when that `HEALTH_n` arrives. A node re-flashed to another role pushes one
  too. A cached kind is kept when a node times out and comes back. The host applies a delta
  immediately.
- A **request/boot reply** is the node-0 row, then one `_NODE_STATUS` per alive node, then
  `_NODE_STATUS_END <count>`. That set is the **authoritative present-roster** — the host
  reconciles (prunes nodes absent from it). `count` includes the node-0 row: `count=1` = the
  bridge alone, no other nodes connected.
- **Silent death** (node yanked / CAN bus-off) is reported as `present=00` by PanelBridge's
  existing 3 s heartbeat timeout (`checkNodeTimeouts`). A periodic client request reconciles any
  delta lost on the wire.

Both directions ride the serial/CDC, so this works in the client's **Bridge mode only** (no
serial in Monitor/Replay). Reserved constants + the wire format live in `NodeStatus.h` (the
canonical contract source the client syncs against; `NODE_STATUS_PROTO_VERSION` bumps on any wire
change). The whole feature is gated behind `-DPANELBRIDGE_NODE_STATUS` (default off).

### Node telemetry — `_NODE_TELEM` (#216)

Same transport, separate contract (`NODE_TELEM_*` in `NodeStatus.h`, `NODE_TELEM_PROTO_VERSION` 1 —
independent of `_NODE_STATUS`, now v3). PanelBridge forwards **every** `TELEM_n` CAN frame
(`0x181`–`0x1BF`, see `02-can-protocol.md`) to the host **unread**, one DCS-BIOS command message per
frame:

```
_NODE_TELEM <hex>      18 hex chars: nodeId(2) + the 8 TELEM frame bytes (16), in wire order
```

The row is `NODE_TELEM_ROW_BYTES` (9) bytes; `NodeTelemField` names the two offsets (`ID` 0,
`FRAME` 1).

- **Pushed as frames arrive** — no request, no cache, no terminator. The host keeps the latest value
  per `(nodeId, page)` and treats a page as stale after a few periods (PDU: one frame per second,
  rotating three pages, so each page every 3 s).
- **Byte 2 of the hex payload is the `TelemPage`;** the host decodes by page. Unlike `_NODE_STATUS`,
  the 8 bytes are raw frame bytes, so multi-byte page fields are **little-endian**.
- PanelBridge drops a frame whose page byte is `0x00` or whose length is not 8; a `TELEM_n` frame
  never changes liveness (`HB_n` owns that).
- Bridge mode only, gated by the same `-DPANELBRIDGE_NODE_STATUS` flag. Like `_NODE_STATUS`, a copy
  that reaches DCS through the client's byte-for-byte relay is ignored as an unknown command.

Client side: OpenSkyhawk/SkyHawkClient#61.

---

## DCSIN_* — Compact Transport Aliases

`DCSIN_*` constants are **compact transport aliases for DCS-BIOS command strings**. They exist
because a classic CAN frame carries only 8 data bytes — far too little to hold a DCS-BIOS ASCII
command like `"LIGHT_EXT_MASTER 2\n"`. The constants are generated sequentially from `0x8001`
by the build step.

They are **not** physical device IDs. Multiple PanelGroup input objects may share the same
`DCSIN_*` constant if they should trigger the same DCS command — PanelBridge does not track
which node sent the EVT.

**Rationale (why DCSIN_* exist, not DCS output addresses):**
DCS output addresses identify values flowing *from* DCS to the cockpit. Input controls send
commands *to* DCS. The two are different namespaces in DCS-BIOS. `DCSIN_*` constants are the
compact stand-in for the ASCII command name, required purely by CAN payload size constraints.

---

## Output Address Constants

Output objects (`LED`, `AnalogOutput`, etc.) use constants from the generated
`A4EC_OutputIds.h` header:

- `A_4E_C_*` — 16-bit DCS-BIOS output address (e.g. `A_4E_C_MASTER_CAUTION`)
- `A_4E_C_*_AM` — 16-bit bitmask for the relevant bits within that address word

Example: `OpenSkyhawk::LED warn(A_4E_C_MASTER_CAUTION, A_4E_C_MASTER_CAUTION_AM, pin);`

Note: DCS-BIOS's own `Addresses.h` uses an `_A` suffix convention for the same addresses.
`A4EC_OutputIds.h` intentionally omits the `_A` suffix and adds the `_AM` mask companion —
use `A4EC_OutputIds.h` for all OpenSkyhawk output objects.

These output addresses serve a completely different purpose and are **never** used as input
`controlId` values.

---

## Generated Headers (Phase 1)

The build step parses the DCS-BIOS A-4E-C JSON definition files and emits two headers:

| Header | Included by | Contents |
|--------|-------------|----------|
| `A4EC_CmdIds.h` | PanelGroup sketches | `#define DCSIN_*` constants only |
| `A4EC_InputMap.h` | PanelBridge only | Sorted `DcsBiosInputEntry[]` table keyed by `cmdId` |

PanelGroup sketches do not include the full dispatch table — only the constant definitions
they need for control declarations.

---

## DcsBiosInputEntry Struct

```cpp
struct DcsBiosInputEntry {
    uint16_t    cmdId;     // DCSIN_* compact command ID
    const char* name;      // DCS-BIOS control name for sendDcsBiosMessage()
};
```

The table is sorted by `cmdId`. Lookup is binary search: ~8–9 comparisons for the full
A-4E-C control set (~150 input entries). Flash cost: ~3–4 KB (names only). RAM cost: zero.

---

## Dispatch form by CAN frame (#147)

The dispatch **form is sourced from the PanelGroup input class** — specifically the CAN frame the
node emits on — **not** inferred from the JSON or stored in the map. The map carries only
`controlId → name`; PanelBridge reads the payload as the frame dictates and formats the argument:

| Frame | PanelGroup sends | PanelBridge formats | DCS-BIOS interface |
|-------|-----------------|---------------------|--------------------|
| `EVT_n` (`canIdEvt`, **ABS**) | absolute `uint16` | `sendDcsBiosMessage(name, "%u")` | `set_state` |
| `EVT_REL_n` (`canIdEvtRel`, **REL**) | signed `±step` (`int16`) | `sendDcsBiosMessage(name, "%+d")` | `variable_step` |
| `EVT_DIR_n` (`canIdEvtDir`, **DIR**) | signed `±1` (`int16`) | `sendDcsBiosMessage(name, "INC"/"DEC")` | `fixed_step` |
| `EVT_ACTION_n` (`canIdEvtAction`, **ACTION**) | selector; `0` only | `sendDcsBiosMessage(name, "TOGGLE")` | `action` |

> **Why the form is the frame, not a map field:** the *same* DCS control can be driven by different
> physical classes — a `fixed_step+set_state` selector is ABS as a switch, DIR as an encoder — so the
> form cannot be inferred from the JSON; it is a property of the class the sketch instantiates. The
> emitting node declares the form by which frame it sends on, so the bridge needs only the name.
> Strings stay off the wire: PanelGroup cannot send `"+3200"`/`"INC"` over the 4-byte CAN payload, so
> it sends the numeric value and the bridge formats per frame. This retired the generator's
> `classify_input` type inference, the per-control `arg*` columns, and the `InputType` codes.

**Class-to-frame mapping:**

| Input class | Frame / form |
|-------------|--------------|
| `Switch2Pos`, `Switch3Pos`, `SwitchMultiPos`, `AnalogMultiPos`, `AnalogInput` | `EVT_n` — ABS (`%u` → set_state) |
| `RotaryEncoder` (REL mode) | `EVT_REL_n` — `±step` (`%+d` → variable_step) |
| `RotaryEncoder` (DIR mode) | `EVT_DIR_n` — `±1` (INC/DEC → fixed_step) |
| `ActionButton` | `EVT_ACTION_n` — selector `0` (TOGGLE → action) |
| `RotaryAcceleratedEncoder` | `EVT_REL_n` — a larger `±step` at speed (REL subsumes acceleration) |

---

## Generator mapping (collapsed, #147)

`gen_a4ec.py` no longer infers a dispatch type from the JSON interfaces. Every input control (any
control with an `inputs` array) maps to a single `{ DCSIN_<name>, "<name>" }` row — `controlId →
name`, sorted by `cmdId` for the bridge's binary search. There is no per-control type or argument,
and no "unsupported interface" gap (name-only is interface-agnostic). `DCSIN_*` IDs are assigned
from the committed append-only `id_ledger.json`, so a snapshot refresh never renumbers existing
controls — it only appends new ones at `max+1` and retires removed ones.

---

## No Sketch Declarations for DCS-BIOS Controls

The map covers all A-4E-C controls automatically. PanelBridge dispatches any
`controlId` in the generated DCS-BIOS input range (`0x8000`-`0x86FF`) without sketch-level
configuration. Values outside that range, including reserved IDs such as `0xFFFF`, are not
DCS-BIOS input commands. No per-control declarations are needed in any sketch.
