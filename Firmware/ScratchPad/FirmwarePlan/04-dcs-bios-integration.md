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
| `0x0010`–`0x00FF` | HID axes and buttons | SimGateway | `HIDAxis` / `HIDButton` → `Joystick.*()` |
| `0x8000`–`0x86FF` | DCS-BIOS compact command IDs (`DCSIN_*`) | PanelBridge | Input map lookup → `sendDcsBiosMessage()` |
| `0xFFFF` | Reserved: TEST_SEQ trigger | — | See `02-can-protocol.md` |

---

## Node-Status Reporting (#86)

PanelBridge surfaces connected PanelGroup nodes + their health to the host (OpenSkyhawk Client)
**over the existing DCS-BIOS protocol**, using reserved A-4E-C-namespace identifiers. No
bespoke sideband; SimGateway relays both directions verbatim (the request is binary export it
forwards host→device; the response is ASCII it forwards device→host). PanelBridge owns it.

| Reserved ID | Direction | Purpose |
|-------------|-----------|---------|
| Export address `0x86FE` (`NODE_STATUS_REQ_ADDR`) | host→device | Client writes it (value ignored) to request the roster. Above every real A-4E-C output (~`0x8554`), so DCS never exports it. Excluded from the CAN broadcast in `handleDcsBiosExport()`. |
| Command name `_NODE_STATUS` (`NODE_STATUS_MSG_NAME`) | device→host | One DCS-BIOS command message per node. Leading underscore — no A-4E-C control collides; DCS-BIOS ignores it as an unknown control if a copy leaks. |
| Command name `_NODE_STATUS_END` (`NODE_STATUS_END_MSG_NAME`) | device→host | Burst terminator for request/boot replies; argument = node count. Lets the client know a roster reply is complete and reconcile/prune. |

**Request** — client writes a DCS-BIOS export frame to `0x86FE`. PanelBridge's
`NodeStatusReqListener` (an `ExportStreamListener`) fires → emits the full roster.

**Response** — `_NODE_STATUS <hex>` where `<hex>` is 18 chars, **each field its numeric value as
fixed-width uppercase hex (most-significant nibble first)**:
`nodeId(2) present(2) flags(2) uptime(4) rxCount(4) esr(4)` (the 8-byte `HeartbeatPayload` plus
`present`). `present`: `01` alive, `00` removed. `flags`: bit0 BOFF, bit1 EPVF. `esr`: low byte
TEC, high byte REC. `nodeId` 1–63.

**Emission semantics:**
- A single bare `_NODE_STATUS` is a **live delta** — emitted on each node alive/dead transition
  (`present` 01/00); the host applies it immediately.
- A **request/boot reply** is N `_NODE_STATUS` messages followed by `_NODE_STATUS_END <count>`. That
  set is the **authoritative present-roster** — the host reconciles (prunes nodes absent from
  it). `count=0` = no panels connected.
- **Silent death** (node yanked / CAN bus-off) is reported as `present=00` by PanelBridge's
  existing 3 s heartbeat timeout (`checkNodeTimeouts`). A periodic client request reconciles any
  delta lost on the wire.

Both directions ride the serial/CDC, so this works in the client's **Bridge mode only** (no
serial in Monitor/Replay). Reserved constants + the wire format live in `HIDControls.h` (the
canonical contract source the client syncs against; `NODE_STATUS_PROTO_VERSION` bumps on any wire
change). The whole feature is gated behind `-DPANELBRIDGE_NODE_STATUS` (default off).

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
    uint8_t     type;      // SWITCH, ACTION, ENCODER, ACCEL_ENCODER, MULTIPOS
    const char* name;      // DCS-BIOS control name for sendDcsBiosMessage()
    const char* arg0;      // value=0 argument: "0", "DEC", or position string
    const char* arg1;      // value=1 argument: "1", "INC", or null for MULTIPOS
    const char* arg0fast;  // ACCEL_ENCODER only: fast decrement arg
    const char* arg1fast;  // ACCEL_ENCODER only: fast increment arg
};
```

The table is sorted by `cmdId`. Lookup is binary search: ~8–9 comparisons for the full
A-4E-C control set (~300 entries). Flash cost: ~6–8 KB. RAM cost: zero.

---

## Value Encoding by Type

| Type | PanelGroup sends | PanelBridge calls |
|------|-----------------|-------------------|
| `SWITCH` | 0 or 1 | `sendDcsBiosMessage(name, arg0_or_arg1)` |
| `ACTION` | 1 (press only) | `sendDcsBiosMessage(name, arg0)` — no release message |
| `ENCODER` | 0=CCW, 1=CW | `sendDcsBiosMessage(name, arg0_or_arg1)` (typically "DEC"/"INC") |
| `ACCEL_ENCODER` | 0=slow CCW, 1=slow CW, 2=fast CCW, 3=fast CW | `sendDcsBiosMessage(name, matched_arg)` — value maps to arg0/arg1/arg0fast/arg1fast |

> **Why 4 numeric values and not strings:** DCS-BIOS Arduino library sends argument strings
> directly (e.g. `"DEC"`, `"INC"`, `"FAST_DEC"`, `"FAST_INC"`) because it runs on the same
> MCU as the encoder. In our architecture, PanelGroup cannot send strings over CAN (4-byte
> payload limit). The 4-value encoding is a **compact CAN transport layer** — PanelBridge
> maps the received value to the appropriate argument string via the input map entry fields.
| `MULTIPOS` | position index 0–N, **or any 16-bit integer** | `sendDcsBiosMessage(name, itoa(value))` — caller-managed `char` buffer ≥ 6 bytes |

**Class-to-type mapping:**

| Input class | Dispatch type |
|-------------|---------------|
| `Switch2Pos` | `SWITCH` |
| `Switch3Pos`, `SwitchMultiPos`, `AnalogMultiPos`, `RotarySwitch` | `MULTIPOS` |
| `ActionButton` | `ACTION` — arg1 and fast args are null |
| `RotaryEncoder` | `ENCODER` |
| `RotaryAcceleratedEncoder` | `ACCEL_ENCODER` |
| `AnalogInput` (DCS-BIOS routed) | `MULTIPOS` — 16-bit ADC value sent as integer string |

---

## Generator Mapping Rules (Phase 1)

| JSON `inputs[].interface` | Generated type | Notes |
|--------------------|----------------|-------|
| `action` | `ACTION` | arg0 = JSON `argument` value; arg1/fast args null |
| `fixed_step` | `ENCODER` | arg0 = `"DEC"`, arg1 = `"INC"` |
| `set_state`, max_value == 1 | `SWITCH` | arg0 = `"0"`, arg1 = `"1"` |
| `set_state`, max_value > 1 | `MULTIPOS` | arg0 unused; value sent as `itoa(value)` |
| Two `fixed_step` interfaces (slow + fast variants) | `ACCEL_ENCODER` | arg0/arg1 = slow dec/inc; arg0fast/arg1fast = fast dec/inc |
| Paired boolean controls (guarded switch: cover + switch) | **Not generated** | Unsupported gap — skip and document in `GENERATOR_GAPS.md` |

---

## No Sketch Declarations for DCS-BIOS Controls

The map covers all A-4E-C controls automatically. PanelBridge dispatches any
`controlId` in the generated DCS-BIOS input range (`0x8000`-`0x86FF`) without sketch-level
configuration. Values outside that range, including reserved IDs such as `0xFFFF`, are not
DCS-BIOS input commands. No per-control declarations are needed in any sketch.
