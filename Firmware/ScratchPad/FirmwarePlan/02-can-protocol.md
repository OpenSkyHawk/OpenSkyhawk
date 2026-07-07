# 02 — CAN Protocol

**Owns:** CAN frame IDs, `NODE_ID` scheme, `ControlPacket` wire format, CAN TX queue policy, filter strategy.
**Does not own:** DCS-BIOS routing logic (→ 04), boot sequencing (→ 09), hardware pin assignments (→ 08).

CAN frame IDs in this document are 11-bit arbitration IDs. They are separate from
payload `ControlPacket::controlId` values; equal numeric values in the two namespaces
do not conflict because they occupy different CAN frame fields.

---

## NODE_ID

Each PanelGroup board has a unique node ID set at compile time via PlatformIO `build_flags`:

```ini
; platformio.ini
build_flags = -DNODE_ID=3
```

This injects `NODE_ID` as a compiler flag visible to every translation unit — the CAN protocol
library, `main.cpp`, and all other files. Do not use `#define NODE_ID 3` in `main.cpp`; that
definition is invisible to library code in separate translation units.

Valid range: **0–63**.

| Value | Assignment |
|-------|-----------|
| `0` | Reserved — PanelBridge (CAN master, no sub-node address) |
| `1–63` | PanelGroup nodes (supports up to 63 nodes — well above the ~20 expected) |

SimGateway is an RP2040 and has no NODE_ID.

---

## ControlPacket Wire Format

Input/output routing payloads use this fixed 4-byte struct:

```cpp
struct ControlPacket {
    uint16_t controlId;  // payload routing key; not a CAN arbitration ID
    uint16_t value;      // payload — interpretation depends on controlId range
};
```

`ControlPacketPair` packs two packets into one 8-byte CAN frame:

```cpp
struct ControlPacketPair {
    ControlPacket a;
    ControlPacket b;
};
```

`ControlPacketPair` is used for `CTRL_BCAST` and `EVT_n` frames only. If only one packet is
ready, slot B uses `controlId = 0x0000` as the null sentinel and receivers ignore it.

Special/control/diagnostic frames (`SYNC_REQ`, `READY_n`, `TEST_SEQ`, `ECHO_n`, `HB_n`) use
the frame-specific DLC and payload listed in the frame table. A special frame may carry a
single 4-byte `ControlPacket` without pairing only if that frame type explicitly defines it.

---

## CAN Message ID Functions

All CAN IDs are computed from `NODE_ID`. Fixed symbolic constants (`CAN_ID_HB_1`, etc.) are
replaced by inline functions:

```cpp
constexpr uint32_t canIdHb(uint8_t n)     { return 0x100 + n; }  // 0x100–0x13F (0 = PanelBridge)
constexpr uint32_t canIdHealth(uint8_t n) { return 0x140 + n; }  // 0x140–0x17F (node health/thermal)
constexpr uint32_t canIdEvt(uint8_t n)   { return 0x200 + n; }  // 0x201–0x23F
constexpr uint32_t canIdEcho(uint8_t n)  { return 0x300 + n; }  // 0x301–0x33F
constexpr uint32_t canIdReady(uint8_t n) { return 0x400 + n; }  // 0x401–0x43F
```

---

## Frame Type Table

| Frame | CAN ID | Direction | DLC | Purpose |
|-------|--------|-----------|-----|---------|
| `CTRL_BCAST` | `0x010` | PanelBridge → All | 8 | Broadcast `ControlPacketPair` to all panels |
| `TEST_SEQ` | `0x011` | PanelBridge → All | 8 | CAN throughput / RTT test — 8-byte payload echoed back unchanged by sub-nodes |
| `SYNC_REQ` | `0x012` | PanelBridge → All | 0 | Request all nodes to re-poll inputs and emit EVTs. Sent on cold boot, on `model_time` backward, in response to `READY`, and when a node transitions from dead/unseen to alive. |
| Reserved | `0x100` | — | — | `canIdHb(0)` / PanelBridge slot. Reserved by formula, not transmitted in normal firmware. |
| `HB_n` | `0x100+n` | Node_n → PanelBridge | 8 | PanelGroup heartbeat every 500 ms for n=1-63 — see HB payload below |
| `HEALTH_n` | `0x140+n` | Node_n → PanelBridge | 8 | Internal die-temp + node-health telemetry every 1000 ms (n=0-63, incl. PanelBridge itself) — see HEALTH payload below (#213/#221) |
| `EVT_n` | `0x200+n` | Node_n → PanelBridge | 8 | Absolute input events (switches, selectors, pots) — `ControlPacketPair`, value `%u` → `set_state` |
| `ECHO_n` | `0x300+n` | Node_n → PanelBridge | 8 | TEST_SEQ echo — 8-byte payload mirrored unchanged |
| `READY_n` | `0x400+n` | Node_n → PanelBridge | 0 | One-time boot signal after initial input poll and EVT burst are complete |
| `EVT_REL_n` | `0x500+n` | Node_n → PanelBridge | 8 | Relative encoder events (`RotaryEncoder` REL) — `ControlPacketPair`, signed `%+d` → `variable_step` (#147) |
| `EVT_DIR_n` | `0x600+n` | Node_n → PanelBridge | 8 | Directional encoder events (`RotaryEncoder` DIR) — `ControlPacketPair`, `±1` → `INC`/`DEC` `fixed_step` (#147) |

> **Migration note (Phase 1):** existing `CAN_ID_HB_1 = 0x100` changes to `canIdHb(1) = 0x101`.
> Update any diagnostic tools, captured CAN logs, or test fixtures that reference the old value.

---

## Heartbeat Payload

`HB_n` frames carry 8 bytes of node health data. PanelBridge reads PanelGroup heartbeats
(`HB_1`–`HB_63`) to track node liveness and diagnostic state. `HB_0` is reserved by the ID
formula but is not transmitted in normal firmware.
CANProtocol owns the helper that fills this payload so higher layers do not read HAL CAN
registers directly.

```cpp
struct __attribute__((packed)) HeartbeatPayload {
    uint8_t  nodeId;    // 0:   node ID (redundant with CAN ID, aids logging)
    uint8_t  flags;     // 1:   bit0=BOFF, bit1=EPVF (Error Passive)
    uint16_t uptime;    // 2–3: seconds since boot, little-endian (wraps at ~18 h)
    uint16_t rxCount;   // 4–5: node-owned accepted RX count, little-endian
    uint16_t esr;       // 6–7: (CAN1->ESR >> 16), little-endian — low byte=TEC, high byte=REC
};
```

**flags bitmask:**
- bit 0 (`0x01`): BOFF — CAN controller is in bus-off state
- bit 1 (`0x02`): EPVF — Error Passive (TEC ≥ 128 or REC ≥ 128)

**esr field:** `(uint16_t)(CAN1->ESR >> 16)`. CAN_ESR register layout: TEC in bits [23:16],
REC in bits [31:24]. After the shift: low byte = TEC, high byte = REC.

**rxCount field:** node-owned receive counter for diagnostics. PanelGroup uses accepted
`CTRL_BCAST` frames received since boot.

**Host node-status reporting (#86):** this same `HeartbeatPayload` is the source for the roster +
health that PanelBridge reports to the host. No new CAN frame is added — PanelBridge caches the
last `HB_n` per node and re-serializes it into a DCS-BIOS `_NODE_STATUS` message
(see `04-dcs-bios-integration.md` and `06-panelbridge-api.md`). As of `_NODE_STATUS` proto **v2**
the message also carries the node's cached `dieTempC` + health/fault fields from `HEALTH_n` (below).

---

## Node Health Payload (#213)

`HEALTH_n` frames carry **free per-node thermal telemetry** read from the MCU's built-in
internal temperature sensor (ADC ch16) and Vrefint (ch17) — no external parts, no PCB change.
Every STM32 node sends one every 1000 ms (half the heartbeat rate — this is trend data);
PanelBridge sends its own as `HEALTH_0`. `STM32Board::readDieTempC()` owns the ADC read (it reads
Vrefint internally to reference Vsense to Vdd — Vdd itself is not transmitted);
`CANProtocol::makeNodeHealthPayload()` packs the frame.

```cpp
struct __attribute__((packed)) NodeHealthPayload {
    uint8_t  nodeId;     // 0:   node ID (redundant with CAN ID, aids logging)
    int8_t   dieTempC;   // 1:   internal die temp, whole °C (INT8_MIN = unavailable)  — #213
    uint8_t  flags;      // 2:   NodeHealthFlag bits (NodeStatus.h): OVERHEAT, DEGRADED           — #213/#163
    uint8_t  faultMask;  // 3:   fault source/domain bitmap — reserved for #163 (0 until populated)
    uint8_t  faultId;    // 4:   NodeFaultCode (NodeStatus.h) — reserved for #163 (0 until populated)
    uint8_t  rsvd[3];    // 5–7: reserved, transmit 0 — future generic health (resetCause, freeRAM, …)
};
```

This is the **shared node-health wire contract**. Distinct features populate their own fields
within the fixed 8 bytes and never collide: temperature (#213) owns `dieTempC` / `flags` bit0;
the degraded-state feature (#163) owns `flags` bit1 / `faultMask` / `faultId`, which transmit 0
until that lands. `faultId` is a `NodeFaultCode` (NodeStatus.h) the client maps to a label — no
fault strings on the wire. **Rail voltage/current is deliberately absent** — that is the PDU's dedicated power
telemetry (#202, real INA226 sensors per console), not generic per-node health; the reserved
bytes are for future *generic* health fields (reset cause, free RAM, I2C error count).

**Default-on**, compile out with `-DNODE_HEALTH_TELEM=0`. PanelBridge caches each node's fields
(under `PANELBRIDGE_NODE_STATUS`) and forwards all of them in `_NODE_STATUS` (proto v2, so the
degraded feature needs no further wire/proto change). A `HEALTH_n` frame is cache-only — it never
changes node liveness (`HB_n` owns that).

**flags bitmask:**
- bit 0 (`0x01`): overheat — `dieTempC >= NODE_OVERHEAT_C`. **Opt-in:** the flag is computed
  only when a build defines `NODE_OVERHEAT_C`; the default build ships no threshold (pure
  telemetry) because the uncalibrated sensor needs field data before a sane trip point is set.
  When enabled, a node also raises its status-LED WARNING on its own overheat.
- bit 1 (`0x02`): degraded — node is alive but a peripheral has tripped (#163). Reserved here;
  populated by the degraded-state feature.

**Calibration caveat:** the STM32F103 sensor is **UNCALIBRATED** (no factory trim / no
`VREFINT_CAL`). Conversion uses datasheet typicals (V25 = 1.43 V, Avg_Slope = 4.3 mV/°C),
referencing Vsense to the Vdd derived from Vrefint (typ 1.20 V). Absolute accuracy ~±few °C;
it reads **die** temperature (not ambient) with a self-heat offset. Use for relative trend and
overheat flagging — accurate hot-spot sensing (e.g. PDU shunt/fuse zones) still uses external NTCs.

---

## ControlPacketPair Frame Payload

`CTRL_BCAST` and `EVT_n` frames carry exactly **8 bytes** — a `ControlPacketPair`.
No other frame type is implicitly decoded as a pair:

```cpp
// Payload = two ControlPackets. Slot B may be null.
ControlPacketPair{ ControlPacket{controlId, value}, ControlPacket{controlId, value} }
```

Slot B is empty when `b.controlId == 0x0000`. Receivers must process slot A first, then slot B
only if `b.controlId != 0x0000`.

CANProtocol owns the `ControlPacketPair` batching path for `CTRL_BCAST`, `EVT_n`, and the relative
event frames `EVT_REL_n` / `EVT_DIR_n` (each with its own pending slot; #147).
PanelBridge and PanelGroup submit individual `ControlPacket`s to CANProtocol; they do not
build or queue pairs themselves.

For each batched CAN ID, CANProtocol may hold a single slot-A packet briefly while waiting
for a second packet, but must flush the half-full batch no later than two owning firmware
`loop()` iterations after slot A is queued. This is a firmware loop deadline, not a literal
MCU clock-cycle deadline. Implementations may flush sooner. Snapshot-style EVT bursts flush
their odd trailing packet immediately when the snapshot pass ends by asking CANProtocol to
flush that batched CAN ID.

The intended public API shape is:

```cpp
void CANProtocol::sendBatched(uint32_t canId, const ControlPacket& pkt); // CTRL_BCAST / EVT_n / EVT_REL_n / EVT_DIR_n
void CANProtocol::flushBatched(uint32_t canId);                          // force slot-B null
```

Calling `sendBatched()` with any CAN ID other than `CAN_ID_CTRL_BCAST`, `canIdEvt(n)`,
`canIdEvtRel(n)`, or `canIdEvtDir(n)` is a programming error and should be ignored with a
diagnostic counter/log in debug builds.

### TEST_SEQ Payload Wire Layout

`TEST_SEQ` carries exactly 8 payload bytes and `ECHO_n` mirrors those bytes unchanged:

| Byte(s) | Field | Encoding |
|---------|-------|----------|
| 0-1 | `seq` | uint16_t little-endian |
| 2-5 | `sentMs` | uint32_t little-endian `millis()` timestamp |
| 6-7 | `reserved` | uint16_t little-endian, currently 0 |

Implementations must serialize this into a `uint8_t[8]`, or use a packed struct with
`static_assert(sizeof(TestSeqPayload) == 8)`. Do not transmit `sizeof(struct)` from an
unpacked C++ struct, because natural alignment can pad this layout beyond 8 bytes.

> **Prototype note:** the existing `PanelGroup.cpp` sends 8 bytes — `ControlPacket` followed
> by a 4-byte `millis()` timestamp. The timestamp was added for prototype RTT measurement only.
> Production firmware uses the full 8 bytes as `ControlPacketPair`; bytes 4–7 are no longer
> a timestamp.

---

## PanelBridge Receive Filter

The heartbeat address family is `canIdHb(n) = 0x100 + n`, so the full HB range is
`0x100`-`0x13F` with `HB_0` reserved for PanelBridge but unused. For child-node liveness, PanelBridge
accepts PanelGroup frames across five inbound ranges: HB_1-HB_63 (`0x101`-`0x13F`),
HEALTH_1-HEALTH_63 (`0x141`-`0x17F`), EVT_1-EVT_63 (`0x201`-`0x23F`), ECHO_1-ECHO_63
(`0x301`-`0x33F`), and READY_1-READY_63 (`0x401`-`0x43F`). These cannot be expressed as a
single CAN mask filter.

**Strategy:** use a **pass-all mask filter** (accepts every ID) with software-side validation
inside the CAN RX interrupt handler. Frames whose ID does not fall in one of the accepted ranges
are silently discarded. `HB_0` (`0x100`) is not a PanelGroup liveness frame. This is safe
because PanelGroup nodes never transmit outside their assigned ranges.

---

## CAN TX Queue

When `CANProtocol::send()` is called and all 3 STM32 CAN TX mailboxes are occupied, the
frame is placed in a software ring buffer (~16 entries). The queue is drained automatically
via the TX-complete interrupt — no explicit drain call is required from `loop()`.

**Overflow policy is frame-type dependent:**

| Frame type | Policy |
|------------|--------|
| `CTRL_BCAST` DCS output broadcasts | Drop stale queued state and retain the newest frame. Coalesce by `controlId` where practical. |
| `EVT_n` input events | Bounded retry: keep the event for up to 3 send attempts, then drop it and increment the `DIAG_ERR` TX drop counter. No unbounded backlog. |
| `HB_n` heartbeat | Drop stale heartbeat; the next heartbeat replaces it. |
| `READY_n`, `SYNC_REQ`, `TEST_SEQ`, `ECHO_n` | Bounded retry up to 3 attempts, then drop and increment the `DIAG_ERR` TX drop counter. |

This prevents silent frame drops during DCS-BIOS export bursts without stalling the firmware,
while keeping user input events from being treated like disposable state snapshots. In a
healthy bus, EVT drops should be effectively zero; any non-zero count is a diagnostic signal.

---

## CAN Bus Status

CANProtocol monitors bus health and exposes it via a `CanStatus` enum and callback:

```cpp
enum class CanStatus { STARTING, NORMAL, TX_ERROR, BUS_OFF };
void CANProtocol::onStatusChange(void(*cb)(CanStatus));
```

Status is derived from `CAN1->ESR` — TEC > 0 → `TX_ERROR`; BOFF bit set → `BUS_OFF`.
The registered callback fires on every transition. STM32Board registers this callback to
drive the bi-color status LED. See `STM32Board TechSpec` for the LED state mapping.

---

## CAN HAL Settings — Required

Two HAL settings are correctness requirements, not implementation preferences. Both were
validated in the 2026-05-20 prototype soak and fault-injection tests:

| Setting | Required value | Consequence if wrong |
|---------|---------------|----------------------|
| `AutoRetransmission` | **DISABLE** | A single unACKed frame fills all 3 TX mailboxes → immediate bus-off; TX queue never drains |
| `AutoBusOff` | **ENABLE** | Hardware auto-recovery in ~3 ms (1408 recessive bits); with DISABLE firmware must restart the controller manually |

With `AutoRetransmission=DISABLE`, TEC rises +8 per failed attempt. At TEC=128 the node
enters Error Passive and limps along — CAN's intended graceful degradation. It self-heals
when the fault clears without any firmware action.

---

## CAN Bus Configuration

- **Baud rate:** 500 kbps
- **Bit timing:** Prescaler=4, BS1=13TQ, BS2=4TQ, **SJW=4TQ** (see note below)
- **Termination:** 120 Ω across CANH/CANL at each bus end node; omit on intermediate nodes
- **Transceiver:** SN65HVD230 (SOIC-8, 3.3 V logic) — see `08-hardware-firmware-contracts.md`
- **Physical bus:** CANH/CANL distributed via Molex Mini-Fit Jr main bus connectors (pins 5/6)

> **SJW=4TQ is required on Blue Pill clones** — crystal-to-crystal variation causes
> intermittent CRC errors at SJW=1TQ. SJW=4TQ costs nothing and should be kept permanently
> even on production PCBs with specified crystals.
