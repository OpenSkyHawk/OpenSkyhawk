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

**Byte order: both fields little-endian on the wire** — `controlId` low byte first, then its high
byte, then `value` the same way (#295). It is the packed struct's native layout on the STM32 and
the RP2040, and the only byte order a third-party node or a CAN capture should assume.

`ControlPacketPair` packs two packets into one 8-byte CAN frame:

```cpp
struct ControlPacketPair {
    ControlPacket a;
    ControlPacket b;
};
```

| Byte(s) | Field | Encoding |
|---------|-------|----------|
| 0-1 | `a.controlId` | uint16_t little-endian |
| 2-3 | `a.value` | uint16_t little-endian |
| 4-5 | `b.controlId` | uint16_t little-endian; `0x0000` = empty slot |
| 6-7 | `b.value` | uint16_t little-endian |

`ControlPacketPair` is used for `CTRL_BCAST` and the four event frames (`EVT_n`, `EVT_REL_n`,
`EVT_DIR_n`, `EVT_ACTION_n`) only. If only one packet is ready, slot B uses `controlId = 0x0000`
as the null sentinel and receivers ignore it.

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
constexpr uint32_t canIdTelem(uint8_t n)  { return 0x180 + n; }  // 0x181–0x1BF (paged node telemetry, #216)
constexpr uint32_t canIdEvt(uint8_t n)   { return 0x200 + n; }  // 0x201–0x23F
constexpr uint32_t canIdEcho(uint8_t n)  { return 0x300 + n; }  // 0x301–0x33F
constexpr uint32_t canIdReady(uint8_t n) { return 0x400 + n; }  // 0x401–0x43F
```

---

## Frame Type Table

| Frame | CAN ID | Direction | DLC | Purpose |
|-------|--------|-----------|-----|---------|
| `CTRL_BCAST` | `0x010` | PanelBridge → All | 8 | Broadcast `ControlPacketPair` to all panels — the whole DCS-BIOS export stream (`controlId` = export address, `0x0002`–`0xFFFE`; #295, D24) |
| `TEST_SEQ` | `0x011` | PanelBridge → All | 8 | CAN throughput / RTT test — 8-byte payload echoed back unchanged by sub-nodes |
| `SYNC_REQ` | `0x012` | PanelBridge → node n | 1 | Ask node n (byte 0) to re-poll its inputs and emit EVTs, ending with READY(RESYNC). Sent one node at a time from the bridge's sync queue (#387): a node seen alive (HB, boot READY, recovery) and every alive node on `model_time` backward. An empty `SYNC_REQ` (DLC 0) is for every node — nodes still honour it; the bridge no longer sends it. |
| `HB_0` | `0x100` | PanelBridge → All | 8 | Master liveness beacon every 500 ms; every other node runs a master-loss watchdog on it (#93) |
| `HB_n` | `0x100+n` | Node_n → PanelBridge | 8 | Node heartbeat every 500 ms for n=1-63 — see HB payload below |
| `HEALTH_n` | `0x140+n` | Node_n → PanelBridge | 8 | Internal die-temp + node-health telemetry every 1000 ms (n=0-63, incl. PanelBridge itself) — see HEALTH payload below (#213/#221) |
| `TELEM_n` | `0x180+n` | Node_n → PanelBridge | 8 | Paged node telemetry (`TelemPayload`) — one frame per second plus flag-change edges; first user the PDU (#216) — see TELEM payload below |
| `EVT_n` | `0x200+n` | Node_n → PanelBridge | 8 | Absolute input events (switches, selectors, pots) — `ControlPacketPair`, value `%u` → `set_state` |
| `ECHO_n` | `0x300+n` | Node_n → PanelBridge | 8 | TEST_SEQ echo — 8-byte payload mirrored unchanged |
| `READY_n` | `0x400+n` | Node_n → PanelBridge | 8 | Boot signal after initial input poll and EVT burst are complete; carries the node's firmware version (`ReadyPayload`, #295). Re-sent, flagged `RESYNC`, in answer to every `SYNC_REQ` (#383) — see READY payload below |
| `EVT_REL_n` | `0x500+n` | Node_n → PanelBridge | 8 | Relative encoder events (`RotaryEncoder` REL) — `ControlPacketPair`, signed `%+d` → `variable_step` (#147) |
| `EVT_DIR_n` | `0x600+n` | Node_n → PanelBridge | 8 | Directional encoder events (`RotaryEncoder` DIR) — `ControlPacketPair`, `±1` → `INC`/`DEC` `fixed_step` (#147) |
| `EVT_ACTION_n` | `0x700+n` | Node_n → PanelBridge | 8 | Action events (`ActionButton`) — `ControlPacketPair`, selector `0` → `TOGGLE` `action` (#116) |
| `BOOT_CMD_n` | `0x740+n` | PanelBridge → Node_n's bootloader | ≤8 | Bootloader commands (#363, #364) — CONNECT / BEGIN / BLOCK + DATA / END / GO / SET_NODE_ID; a CONNECT in the 200 ms after a reset also catches a running board, and a running app with a bootloader installed takes a CONNECT as "enter the bootloader" (#365). PanelBridge relays it from the host (`04`); it never acts on `BOOT_CMD_0`. Lowest priority on the bus, so update traffic never delays a control. Values in `BootProtocol.h`; protocol in `12-bootloader.md` |
| `BOOT_RSP_n` | `0x780+n` | Node_n's bootloader → PanelBridge | ≤8 | Bootloader replies and the 1 s announcement (8 bytes, with the chip's UID hash) while a board waits in its bootloader (#363, #364) |
| `BOOT_UNCONF_CMD` | `0x7C0` | PanelBridge → unconfigured bootloader | ≤8 | Commands to a bootloader with no NODE_ID yet; CONNECT names its UID hash and binds that board alone (#364) |
| `BOOT_UNCONF_RSP` | `0x7C1` | unconfigured bootloader → PanelBridge | 8 | Announcement and replies of a bootloader with no NODE_ID (option byte Data0 = `0xFF`) and no valid app (#363, #364) |

---

## Heartbeat Payload

`HB_n` frames carry 8 bytes of node health data. PanelBridge reads node heartbeats
(`HB_1`–`HB_63`) to track node liveness and diagnostic state; its own `HB_0` is the master
liveness beacon the other nodes watch (#93).
CANProtocol owns both the helper that fills this payload, so higher layers do not read HAL CAN
registers directly, and — since #340 — the sending itself: every node calls
`CANProtocol::serviceNodeStatus()` once per loop, which sends `HB_n` every 500 ms and `HEALTH_n`
every 1000 ms and, on every node except PanelBridge, runs the master-loss watchdog (WARNING after
1500 ms without `HB_0`, once a master has been seen). On those nodes `start()` adds `HB_0` to the
hardware filter automatically. Each node passes its board kind (`NodeKind`, #344) to
`serviceNodeStatus()` from its library role — see *Node Health Payload* below.

**Duplicate NODE_ID check (#344).** `start()` also adds the node's **own** `HB_n` to its filter, on
every node including PanelBridge. bxCAN never receives its own transmissions, so a frame on that ID
means a second board was flashed with the same `NODE_ID` — which otherwise only shows up as one
blended node (uptime/rxCount jumping between two boards, error frames from identical-ID
collisions). The node then reports `NodeFaultCode::DUPLICATE_NODE_ID` in its `HEALTH_n` (DEGRADED +
`faultId`, plus the DiagSerial fault edge log):
- **Top priority** — it is chosen ahead of every registered `FaultSource`, because nothing else
  reported under a shared ID can be trusted; a lesser fault can never mask it.
- **Auto-clears** `DUP_NODE_TIMEOUT_MS` (3000 ms) after the last foreign frame, so removing or
  re-flashing the other board clears it without rebooting this one. A live duplicate re-raises
  within one 500 ms heartbeat.
- In `startLoopback()` test mode the node's own heartbeats do come back; the check discounts one
  looped-back frame per heartbeat it sent, so only extra frames read as a duplicate.
- The own-ID frame is still passed to `onReceive()`.

```cpp
struct __attribute__((packed)) HeartbeatPayload {
    uint8_t  nodeId;    // 0:   node ID (redundant with CAN ID, aids logging)
    uint8_t  flags;     // 1:   bit0=BOFF, bit1=EPVF (Error Passive)
    uint16_t uptime;    // 2–3: seconds since boot, little-endian (wraps at ~18 h)
    uint16_t rxCount;   // 4–5: node-owned accepted RX count, little-endian
    uint16_t esr;       // 6–7: (CAN1->ESR >> 16), little-endian — low byte=TEC, high byte=REC
};
```

**flags bitmask** (`HeartbeatFlag`, `NodeStatus.h`):
- bit 0 (`0x01`): BOFF — CAN controller is in bus-off state
- bit 1 (`0x02`): EPVF — Error Passive (TEC ≥ 128 or REC ≥ 128)

**esr field:** `(uint16_t)(CAN1->ESR >> 16)`. CAN_ESR register layout: TEC in bits [23:16],
REC in bits [31:24]. After the shift: low byte = TEC, high byte = REC — `HB_ESR_TEC_SHIFT` (0) and
`HB_ESR_REC_SHIFT` (8) in `NodeStatus.h`.

**rxCount field:** node-owned receive counter for diagnostics. PanelGroup uses accepted
`CTRL_BCAST` frames received since boot.

**Host node-status reporting (#86):** this same `HeartbeatPayload` is the source for the roster +
health that PanelBridge reports to the host. No new CAN frame is added — PanelBridge caches the
last `HB_n` per node and re-serializes it into a DCS-BIOS `_NODE_STATUS` message
(see `04-dcs-bios-integration.md` and `06-panelbridge-api.md`). As of `_NODE_STATUS` proto **v2**
the message also carries the node's cached `dieTempC` + health/fault fields from `HEALTH_n` (below);
proto **v3** (#344) adds the node's board kind and the bridge's own node-0 row; proto **v4** (#295)
adds the node's firmware version from its `READY_n` (below).

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
    int8_t   dieTempC;   // 1:   internal die temp, whole °C (DIE_TEMP_UNAVAILABLE)    — #213
    uint8_t  flags;      // 2:   NodeHealthFlag bits (NodeStatus.h): OVERHEAT, DEGRADED           — #213/#163
    uint8_t  faultMask;  // 3:   fault source/domain bitmap — reserved for future domain bits (0)
    uint8_t  faultId;    // 4:   NodeFaultCode (NodeStatus.h): 0 = no fault, else primary fault  — #163
    uint8_t  nodeKind;   // 5:   NodeKind (NodeStatus.h): board kind from the firmware role        — #344
    uint8_t  rsvd[2];    // 6–7: reserved, transmit 0 — future generic health (resetCause, freeRAM, …)
};
```

This is the **shared node-health wire contract**. Distinct features populate their own fields
within the fixed 8 bytes and never collide: temperature (#213) owns `dieTempC` / `flags` bit0;
the degraded-state feature (#163) owns `flags` bit1 / `faultId`, populated per node by
`OpenSkyhawk::aggregateFaults()` (the primary fault, first non-`NONE` in `FaultSource` registry
order) — or `DUPLICATE_NODE_ID`, which outranks them all (see *Duplicate NODE_ID check* above);
the board-kind feature (#344) owns `nodeKind`. A healthy node ships all-zero; a faulted node ships `flags` DEGRADED + a nonzero `faultId`,
but `faultMask` **stays 0** — it is reserved for future fault-domain bits. `faultId` is a
`NodeFaultCode` (NodeStatus.h) the client maps to a label — no fault strings on the wire. **Rail voltage/current is deliberately absent** — that is the PDU's dedicated power
telemetry (`TELEM_n` below, #216; INA180A2 current sense per console, #202), not generic per-node health; the reserved
bytes are for future *generic* health fields (reset cause, free RAM, I2C error count).

**nodeKind (#344)** — what kind of board this `NODE_ID` is, as a `NodeKind` (NodeStatus.h,
append-only): `0x00` unknown, `0x01` panel group, `0x02` PDU, `0x03` bridge. It is announced by
the board's own firmware **role**, never derived from the `NODE_ID` (a PDU can take any ID):
`serviceNodeStatus(kind, …)` takes it as a required argument, and each node library passes its
own — `PanelGroup` → panel group, `PduNode` (PowerMonitor) → PDU, `PanelBridge` → bridge — so a
sketch never sets it and can't forget it. "Bridge" is the SimGateway + PanelBridge board: one board,
two MCUs; the PanelBridge MCU reports it in `HEALTH_0`, and the SimGateway (not on the bus) has no
kind of its own. Firmware older than #344 sent `0` in this byte (it was reserved), so `0x00` reads
as unknown — the client never adopts a node on it.

**Default-on**, compile out with `-DNODE_HEALTH_TELEM=0`. PanelBridge caches each node's fields
(under `PANELBRIDGE_NODE_STATUS`) and forwards all of them in `_NODE_STATUS` (proto v4, #295), plus
its own `HEALTH_0` as the node-0 row. A `HEALTH_n` frame never changes node liveness (`HB_n` owns
that); the only delta it triggers is a `_NODE_STATUS` push when the node's cached `nodeKind`
changes (see `04-dcs-bios-integration.md`).

**flags bitmask:**
- bit 0 (`0x01`): overheat — `dieTempC >= NODE_OVERHEAT_C`. **Opt-in:** the flag is computed
  only when a build defines `NODE_OVERHEAT_C`; the default build ships no threshold (pure
  telemetry) because the uncalibrated sensor needs field data before a sane trip point is set.
  When enabled, a node also raises its status-LED WARNING on its own overheat.
- bit 1 (`0x02`): degraded — node is alive but a registered `FaultSource` reports non-`NONE` (#163)
  — a DrumDisplay I2C fault, a PDU rail over/under-voltage, a bridge host-link loss, etc. Derived:
  set whenever `faultId != NONE`. **No status-LED change** — degraded surfaces via the frame + a
  DiagSerial edge log only (multi-source WARNING-latch arbitration is deferred).

**Calibration caveat:** the STM32F103 sensor is **UNCALIBRATED** (no factory trim / no
`VREFINT_CAL`). Conversion uses datasheet typicals (V25 = 1.43 V, Avg_Slope = 4.3 mV/°C),
referencing Vsense to the Vdd derived from Vrefint (typ 1.20 V). Absolute accuracy ~±few °C;
it reads **die** temperature (not ambient) with a self-heat offset. Use for relative trend and
overheat flagging — accurate hot-spot sensing (e.g. PDU shunt/fuse zones) still uses external NTCs.

---

## Node Telemetry Payload — TELEM_n (#216)

`TELEM_n` frames carry **paged node telemetry**: byte 0 is a `TelemPage` (NodeStatus.h) naming the
layout of bytes 1–7. Page ids are **global and append-only** — a page means the same layout whichever
node sends it, so the host decodes by page, never by node type — and `0x00` is invalid. Multi-byte
fields are little-endian.

```cpp
struct __attribute__((packed)) TelemPayload {
    uint8_t page;     // 0:   TelemPage (NodeStatus.h); 0x00 invalid
    uint8_t body[7];  // 1-7: page-defined
};
```

**Why one frame family with a page byte**, not one ID block per value group: each per-node ID block
costs 64 IDs, and with a page byte PanelBridge can forward frames **unread** (`_NODE_TELEM`, see
`04-dcs-bios-integration.md`) — a new page type needs no bridge change. `0x180+n` sits with `HB_n`
(`0x100`) and `HEALTH_n` (`0x140`) in the node-status block; it outranks `EVT_n` in arbitration
exactly as `HEALTH_n` does, which at ≤ 1 frame/s per node costs an input event at most one frame time
(~0.27 ms). Generic node health (die temp, fault id) stays in `HEALTH_n` and is never repeated here.

**PDU pages** (the PDU monitoring node, `PowerMonitor` library):

| Page | Byte 1 | Bytes 2–3 | Bytes 4–5 | Bytes 6–7 |
|---|---|---|---|---|
| `0x01 PDU_RAIL_12V` / `0x02 PDU_RAIL_5V` | `PduRailFlag` bits | `vAvgMv` u16 | `vMinMv` u16 (window min) | `iAvgMa` **int16** |
| `0x03 PDU_STATUS` | `PduStatusFlag` bits | `ntcTempDC` int16, 0.1 °C (`NTC_TEMP_UNAVAILABLE` = `INT16_MIN`) | `i12PeakMa` u16 | `i5PeakMa` u16 |

Each page's byte offsets are named in `NodeStatus.h` — `PduRailField` for both rail pages,
`PduStatusField` for `PDU_STATUS` — and the PDU packs its frames through them.

- `PduRailFlag`: bit0 UNDER_V · bit1 OVER_V · bit2 OVER_CURRENT · bit3 FUSE_OPEN · bit4 SENSOR_FAULT.
- `PduStatusFlag`: bit0 UNCAL_VDDA · bit1 UNCAL_ZERO · bit2 TEMP_WARN · bit3 TEMP_CRIT · bit4 NTC_FAULT.
  All flags are fault-sense — all-zero is healthy and calibrated.
- Units are engineering units computed on the node, which holds its own calibration; the host needs no
  per-board knowledge. `iAvgMa` is signed so residual zero-offset stays visible.
- Peak current is after the board's ~100 µs RC filter (1 kΩ / 100 nF on each INA180 output).

**Send policy (PDU):** one frame per second — the `HEALTH_n` rate — rotating `PDU_RAIL_12V` →
`PDU_RAIL_5V` → `PDU_STATUS`, so each page refreshes every 3 s and its averages / minimum / peaks cover
those 3 s. A page whose flag byte changes is sent at once, at most once per 100 ms per page, without
restarting its window; every page starts from an all-clear baseline, so a condition present at boot
goes out promptly. `SYNC_REQ` sends all three pages immediately. The node's rail and thermal monitors
are `FaultSource`s, so their worst condition also reaches `HEALTH_n.faultId` (`FUSE_OPEN`,
`SHORT_CIRCUIT`, `OVER_VOLTAGE`, `UNDER_VOLTAGE`, `OVER_TEMPERATURE`, `SENSOR_FAULT`).

**Bus load at 500 kbit/s** (8-byte frame ≈ 130 bits worst-case stuffing ≈ 0.27 ms): 3 PDUs steady =
3 frames/s ≈ **0.08 %**; worst-case fault storm (every page edging at the 100 ms limit) = 90 frames/s
≈ 2.4 %. For scale, `HB_n` + `HEALTH_n` from 20 nodes is 60 frames/s ≈ 1.6 %.

---

## Ready Payload — READY_n (#295)

`READY_n` (`0x400+n`) is sent once per boot, at the end of the node's `setup()` after its boot EVT
burst (`CANProtocol::sendReady()`), so every reflash, reset or power cycle announces the firmware
the node now runs:

```cpp
struct ReadyPayload {          // 8 bytes, DLC 8
    uint8_t fwMajor;           // 0: firmware release version (FirmwareVersion.h, OPENSKYHAWK_FW_*)
    uint8_t fwMinor;           // 1
    uint8_t fwPatch;           // 2
    uint8_t bootloader;        // 3: installed bootloader's protocol version, 0 = none (#365)
    uint8_t flags;             // 4: ReadyFlag bits (#383) — 0 = the boot READY
    uint8_t rsvd[3];           // 5-7: reserved, transmit 0
};
```

**Resync (#383, #387).** The boot READY is the only time a node announced its version, so a bridge
that booted after the node — a bridge-only reset, a Gateway update, a cold power-up where the bridge
is slower — reported it as 0.0.0 until the node restarted. Since #383 every node also answers each
`SYNC_REQ` for it with a READY flagged `ReadyFlag::RESYNC` (`0x01`), sent by `CANProtocol::drain()`
**after** its EVT burst has left (#387: its transmit ring empty and all three mailboxes free, 50 ms
cap — sent at once, a higher-ID frame of the burst could still go after it). It re-announces the
version and marks the end of the node's answer: the bridge caches the version and moves its sync
queue to the next node; it never queues a node for a RESYNC READY (they would answer each other
forever). Cost: one 8-byte frame per sync. Firmware without the flag reads as a boot READY.

PanelBridge caches the version per node (under `PANELBRIDGE_NODE_STATUS`), keeps it when the node
times out, and reports it in `_NODE_STATUS` v4 (`04-dcs-bios-integration.md`). A READY from a node
that is already alive — it rebooted inside the 3 s heartbeat timeout, e.g. a quick reflash — pushes
a `_NODE_STATUS` delta when the version changed. Firmware older than #295 sends READY with DLC 0:
the bridge reads that as **not reported** (0.0.0). The bridge's other use of a boot READY — mark the
node alive, queue it for a sync — ignores the payload.

---

## ControlPacketPair Frame Payload

`CTRL_BCAST` and the four event frames carry exactly **8 bytes** — a `ControlPacketPair`
(byte layout in § ControlPacket Wire Format). No other frame type is implicitly decoded as a pair:

```cpp
// Payload = two ControlPackets. Slot B may be null.
ControlPacketPair{ ControlPacket{controlId, value}, ControlPacket{controlId, value} }
```

Slot B is empty when `b.controlId == 0x0000`. Receivers must process slot A first, then slot B
only if `b.controlId != 0x0000`.

CANProtocol owns the `ControlPacketPair` batching path for `CTRL_BCAST`, `EVT_n`, and the relative
event frames `EVT_REL_n` / `EVT_DIR_n` (#147) / `EVT_ACTION_n` (#116) — each with its own pending
slot. Calling `sendBatched()` with any other CAN ID is a programming error: the frame has no
registered slot and is silently discarded — no log, no fault. Adding a batched frame therefore
means registering its ID in `begin()`, not just defining its `canId` helper; the `tx_batching`
test is what proves the slot exists.
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
void CANProtocol::sendBatched(uint32_t canId, const ControlPacket& pkt); // CTRL_BCAST / EVT_n / EVT_REL_n / EVT_DIR_n / EVT_ACTION_n
void CANProtocol::flushBatched(uint32_t canId);                          // force slot-B null
```

### TEST_SEQ Payload Wire Layout

`TEST_SEQ` carries exactly 8 payload bytes and `ECHO_n` mirrors those bytes unchanged. It is a bench
diagnostic: PanelBridge sends it when `T` is typed on its DiagSerial and prints the round-trip time.

**What the freeze covers (#295):** the two IDs, and every node mirroring all 8 bytes unchanged on
`ECHO_n` — a 1.x node must keep answering a 1.0 bridge's test. The layout below is
**bridge-internal**: only PanelBridge writes and reads it (nodes never look inside), so it may
change in a minor release.

| Byte(s) | Field | Encoding |
|---------|-------|----------|
| 0-1 | `seq` | uint16_t little-endian |
| 2-5 | `sentMs` | uint32_t little-endian `millis()` timestamp |
| 6-7 | `reserved` | uint16_t little-endian, currently 0 |

Implementations must serialize this into a `uint8_t[8]`, or use a packed struct with
`static_assert(sizeof(TestSeqPayload) == 8)`. Do not transmit `sizeof(struct)` from an
unpacked C++ struct, because natural alignment can pad this layout beyond 8 bytes.

---

## Compatibility — how the wire grows (#295, D22)

From v1.0 the frame IDs and payloads are frozen: a change that breaks an existing reader is a new
**major** version. Within a major, the wire only grows, and every reader tolerates growth:

- **Frames never grow and fields never move.** A classic CAN frame carries 8 bytes, and the
  STM32F103's bxCAN has no CAN FD. An existing field is never moved, resized or reinterpreted —
  that is a major.
- **Reserved bits and bytes:** transmit 0, ignore on receive. A later minor may give one a
  meaning; an older reader keeps ignoring it.
- **Unknown values are kept, not rejected.** A `NodeKind`, `NodeFaultCode`, `TelemPage` or flag bit
  a reader doesn't know is carried as "unknown (0xNN)". New values are minor.
- **Where new data goes** (each a minor):
  - generic node health → `HEALTH_n`'s reserved bytes (6–7);
  - node-specific telemetry → a new `TelemPage` on `TELEM_n` (append-only, forwarded unread);
  - identity or anything else → `READY_n`'s reserved bytes (3–7), or a new frame family in a
    free ID range (`0x013`–`0x0FF`, `0x1C0`–`0x1FF`, `0x240`–`0x2FF`, `0x340`–`0x3FF`,
    `0x440`–`0x4FF`, `0x540`–`0x5FF`, `0x640`–`0x6FF`, `0x7C2`–`0x7FF`; `0x740`–`0x7C1` is the
    bootloader's, see the frame table). Its position in the map sets its arbitration
    priority (lower ID wins), which is frozen with it.
- `TELEM_n` never carries generic health, and `HEALTH_n` never carries node-specific telemetry.

`TEST_SEQ` / `ECHO_n` are covered only as far as other boards rely on them: the two IDs, and every
node mirroring the 8 bytes unchanged. The payload contents are bridge-internal (above).

---

## PanelBridge Receive Filter

The heartbeat address family is `canIdHb(n) = 0x100 + n`, so the full HB range is
`0x100`-`0x13F` with `HB_0` PanelBridge's own master beacon. PanelBridge accepts node frames across
nine inbound ranges: HB_1-HB_63 (`0x101`-`0x13F`), HEALTH_1-HEALTH_63 (`0x141`-`0x17F`),
TELEM_1-TELEM_63 (`0x181`-`0x1BF`), EVT_1-EVT_63 (`0x201`-`0x23F`), ECHO_1-ECHO_63
(`0x301`-`0x33F`), READY_1-READY_63 (`0x401`-`0x43F`), EVT_REL (`0x501`-`0x53F`), EVT_DIR
(`0x601`-`0x63F`) and EVT_ACTION (`0x701`-`0x73F`). These cannot be expressed as a single CAN mask
filter.

**Strategy:** use a **pass-all mask filter** (accepts every ID) with software-side validation
inside the CAN RX interrupt handler. Frames whose ID does not fall in one of the accepted ranges
are silently discarded. `HB_0` (`0x100`) is PanelBridge's own transmission, not a child-node liveness
frame — so an `HB_0` that does arrive came from another board on node 0 and trips the bridge's
duplicate NODE_ID check (#344), like any node's. This is safe
because PanelGroup nodes never transmit outside their assigned ranges.

---

## CAN TX Queue

When `CANProtocol::send()` is called and all 3 STM32 CAN TX mailboxes are occupied, the
frame is placed in a software ring buffer (~16 entries). The queue is drained automatically
via the TX-complete interrupt — no explicit drain call is required from `loop()`.

**Overflow policy is frame-type dependent:**

| Frame type | Policy |
|------------|--------|
| `CTRL_BCAST` DCS output broadcasts | Queued in order like any frame — **never coalesced** (#389): a frame carries two of ~250 controls, not the whole state, so replacing a queued one lost two others' updates (~19 % of a real stream). The bridge's TX ring is 64 frames (`CAN_TX_RING_SIZE`); a lone packet waits ≤ 2 ms for its partner (1.98 packets per frame on a real stream). Highest priority ID on the bus. |
| `EVT_n` input events | Bounded retry: keep the event for up to 3 send attempts, then drop it and increment the `DIAG_ERR` TX drop counter. No unbounded backlog. |
| `HB_n`, `HEALTH_n`, `TELEM_n` periodic status | Same bounded retry as the event frames — up to 3 send attempts, then drop and increment the `DIAG_ERR` TX drop counter. Not coalesced: the next period's frame carries fresh state anyway. |
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
