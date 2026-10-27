# CAN Bus Protocol

The CAN bus is the backbone of OpenSkyhawk — every panel hangs off it, and every control
crosses it in the same `ControlPacket` format. This page is the full protocol reference:
configuration, frame IDs, wire format, the startup handshake, and the settings you must get
right. Constants here are taken from `Firmware/Libraries/CANProtocol/CANProtocol.h` — when in
doubt, that header is authoritative.

## Bus configuration

| Parameter | Value |
|-----------|-------|
| Baud rate | **500 kbps** |
| Bit timing | Prescaler = 4, BS1 = 13TQ, BS2 = 4TQ, **SJW = 4TQ** |
| Transceiver | SN65HVD230 (SOIC-8, 3.3 V logic), one per STM32 board, on PA11 (RX) / PA12 (TX) |
| Topology | Two-wire differential (CANH/CANL), daisy-chained across all boards |
| Termination | 120 Ω across CANH/CANL at the **two physical end nodes only** — omit on intermediate nodes |

The bus requires an external 8 MHz crystal on every STM32 board — the internal RC oscillator
is not accurate enough for 500 kbps. Both the crystal requirement and the SJW = 4TQ value are
empirical findings; see [Design Decisions](design-decisions.md#d5-sjw-4tq-empirically-determined).

## Frame ID scheme

All frames use standard 11-bit CAN IDs. Per-node IDs are computed from `NODE_ID` (1–63) with
helper functions — **never hard-code these values**, always use the helper.

| Frame | CAN ID | Helper | Direction | DLC |
|-------|--------|--------|-----------|-----|
| `TEST_SEQ` | `0x011` | `CAN_ID_TEST_SEQ` | PanelBridge → All | 8 |
| `SYNC_REQ` | `0x012` | `CAN_ID_SYNC_REQ` | PanelBridge → node *n* | 1 |
| `HB_n` | `0x100 + n` | `canIdHb(n)` | Node *n* → PanelBridge | 8 |
| `HEALTH_n` | `0x140 + n` | `canIdHealth(n)` | Node *n* → PanelBridge | 8 |
| `TELEM_n` | `0x180 + n` | `canIdTelem(n)` | Node *n* → PanelBridge | 8 |
| `EVT_n` | `0x200 + n` | `canIdEvt(n)` | Node *n* → PanelBridge | 8 |
| `ECHO_n` | `0x300 + n` | `canIdEcho(n)` | Node *n* → PanelBridge | 8 |
| `READY_n` | `0x400 + n` | `canIdReady(n)` | Node *n* → PanelBridge | 8 |
| `EVT_REL_n` | `0x500 + n` | `canIdEvtRel(n)` | Node *n* → PanelBridge | 8 |
| `EVT_DIR_n` | `0x600 + n` | `canIdEvtDir(n)` | Node *n* → PanelBridge | 8 |
| `EVT_ACTION_n` | `0x700 + n` | `canIdEvtAction(n)` | Node *n* → PanelBridge | 8 |
| `BOOT_CMD_n` | `0x740 + n` | `Boot::canIdBootCmd(n)` | PanelBridge → node *n*'s bootloader | ≤8 |
| `BOOT_RSP_n` | `0x780 + n` | `Boot::canIdBootRsp(n)` | Node *n*'s bootloader → PanelBridge | ≤8 |
| `BOOT_UNCONF_*` | `0x7C0` / `0x7C1` | `Boot::UNCONFIGURED_CMD_ID` / `_RSP_ID` | PanelBridge ↔ a bootloader with no NODE_ID | ≤8 |
| `CTRL_BCAST` | `0x7E0` | `CAN_ID_CTRL_BCAST` | PanelBridge → All | 8 |

- **A lower ID wins arbitration**, so the table is also the priority order, highest first. Every
  node's input, heartbeat and update frame outranks the sim's output: an input happens once, while
  a sim value comes back by itself on the next change and on DCS-BIOS's autosync.
- `CTRL_BCAST` carries DCS-BIOS output state from PanelBridge to every node — the whole export
  stream, whatever aircraft is flying; each output picks out its own address. It is the
  lowest-priority frame on the bus: under load the sim's output yields to every node's frame.
- Arbitration orders frames *between* boards, not inside one. Each board sends from a single queue,
  in order, so PanelBridge's own `HB_0` and `SYNC_REQ` wait behind any `CTRL_BCAST` frames queued
  before them, and those lose every contest with a node's frame. How long that wait gets on a busy
  bus has not been measured.
- `BOOT_CMD_n` / `BOOT_RSP_n` are the [bootloader](../firmware/bootloader.md)'s frames: a board
  waiting in its bootloader announces itself once a second and takes a new firmware image in 64-byte
  blocks. They sit below every input and status frame (only the sim's `CTRL_BCAST` is lower), so
  update traffic never delays an input. The
  `0x7C0`/`0x7C1` pair is for a board that has no NODE_ID yet.
- `EVT_n` carries input events from a node back up to PanelBridge.
- `HB_n` is a 500 ms heartbeat with CAN health (TEC/REC, flags, uptime).
  Every node — PanelBridge included, as `HB_0` — sends `HB_n` and `HEALTH_n` from one shared
  CANProtocol routine, which also runs each node's watch on the master heartbeat.
- `HEALTH_n` carries `NodeHealthPayload` — internal die temperature, the node's
  degraded/fault state, and its **board kind** (panel group, PDU or bridge). The kind comes from
  the board's own firmware, never from its `NODE_ID`, so the host app can tell what a newly seen
  node is before anyone adopts it. PanelBridge reports every node's kind to the host, its own
  included.
- Each node also listens on its own heartbeat ID. A board never hears its own frames, so a
  heartbeat there means a second board was flashed with the same `NODE_ID`; the node then flags a
  duplicate-ID fault in `HEALTH_n`, which the host app shows.
- `TELEM_n` carries paged telemetry: byte 0 says which page the other seven bytes are. The
  power-distribution node uses it for per-rail voltage and current and its board temperature, one
  frame a second plus an immediate frame whenever a rail condition changes. PanelBridge passes every
  `TELEM_n` frame to the host app unread, so a new kind of page needs no bridge change.
- `TEST_SEQ` / `ECHO_n` are a round-trip latency test: PanelBridge sends `TEST_SEQ`, a node
  echoes on `ECHO_n`.
- `SYNC_REQ` / `READY_n` are the boot/resync handshake (below).

!!! note "canIdHb(1) = 0x101, not 0x100"
    `canIdHb(0)` (`0x100`) belongs to PanelBridge: it is the master heartbeat, sent every
    500 ms, and every other node raises its WARNING light if it stops. Real nodes start at
    `canIdHb(1) = 0x101`. The old `CAN_ID_HB_1 = 0x100` constant is deprecated; use the
    `canIdHb(n)` helper.

### Why there are four event frames, not one

`EVT_n` carries an **absolute** value: the control's new position, as a number. Three inputs
cannot say what they mean that way, so each gets its own frame rather than overloading the
value space:

| Frame | Payload | PanelBridge emits |
|-------|---------|-------------------|
| `EVT_REL_n` | signed ±step (`int16`) | `%+d` for a DCS-BIOS `variable_step` control |
| `EVT_DIR_n` | signed ±1 (`int16`) | `INC` / `DEC` for a `fixed_step` control |
| `EVT_ACTION_n` | selector, `0` = `TOGGLE` | `TOGGLE` — the only argument the action interface defines |

`EVT_ACTION_n` exists because an action is a *keyword*, not a number: the absolute value space
is already spoken for by literal `set_state` values, so `TOGGLE` has no integer to ride on —
the same reason `EVT_DIR_n` exists for `INC`/`DEC`. A payload value other than `0` on
`EVT_ACTION_n` is malformed and the bridge drops it.

## ControlPacket wire format

Everything that routes a control is a `ControlPacket` — a 4-byte `{ controlId, value }` pair:

```cpp
struct ControlPacket     { uint16_t controlId; uint16_t value; };  // 4 bytes
struct ControlPacketPair { ControlPacket a; ControlPacket b; };    // 8 bytes
```

- **`controlId`** is the routing key — *what* the control is. It is **not** a CAN arbitration
  ID; the two namespaces are separate and equal numeric values don't collide because they
  occupy different CAN frame fields. See [DCS-BIOS vs HID](dcsbios-vs-hid.md) for the
  `controlId` ranges.
- **`value`** is the payload — interpretation depends on the `controlId` range.
- **Byte order:** both fields are little-endian on the wire, low byte first — in a
  `ControlPacketPair`, bytes 0–1 `a.controlId`, 2–3 `a.value`, 4–5 `b.controlId`, 6–7 `b.value`.
  Every multi-byte CAN payload field in this project is little-endian.

`CTRL_BCAST` and all four event frames — `EVT_n`, `EVT_REL_n`, `EVT_DIR_n` and
`EVT_ACTION_n` — carry a **`ControlPacketPair`** so two packets share one 8-byte frame. Each
has its own pending slot. When only one packet is ready, slot B's `controlId` is set to the
null sentinel `0x0000`. Batching is owned by `CANProtocol::sendBatched()` / `flushBatched()` —
callers submit individual `ControlPacket`s and the library packs them.

A frame ID with no batch slot is **silently discarded**, so adding a batched frame means
registering it in `_batches[]`, not just defining its `canId` helper.

## NODE_ID scheme (brief)

`NODE_ID` is a compile-time constant (`build_flags = -DNODE_ID=N` in `platformio.ini`), range
0–63. **0** is PanelBridge (reserved); **1–63** are PanelGroup nodes, assigned incrementally
and permanent once assigned. SimGateway has no NODE_ID. Full policy and the live registry are
on the [NODE_ID & CAN Addressing](../firmware/node-id.md) page.

## Startup and sync sequence

PanelGroup nodes may power up before or after PanelBridge, so boot state can't be assumed.
The `SYNC_REQ` / `READY` handshake makes input state deterministic:

1. On cold boot a PanelGroup completes its initial input poll, then sends `READY_n`, carrying its
   firmware version so the client can show which build every board runs.
2. PanelBridge asks for a node's state with a `SYNC_REQ` naming that node — when it first sees
   the node (its heartbeat after the bridge boots, its `READY_n`, or its return after a timeout),
   and for every node on a DCS session change. It asks **one node at a time**: all nodes answering
   at once overflowed the bridge with only three nodes, and a full cockpit would send over a
   thousand frames in one burst.
3. The node re-polls all its inputs and re-sends their current state as `EVT_n` frames, then a
   `READY_n` flagged as a resync. That READY tells the bridge the node is done, so it moves on to
   the next one (or after 150 ms if a node doesn't answer), and it re-announces the node's
   firmware version to a bridge that started after it. A resync READY never starts another sync.

This lets PanelBridge collect a fresh input snapshot at any time. `SYNC_REQ` asks for **inputs
only**: it does not make DCS-BIOS send the outputs again. Missed *outputs* (LEDs, gauges) are
tolerable: a value that changes arrives on the next DCS-BIOS frame, and one that doesn't change is
re-sent only by DCS-BIOS's autosync, every few seconds. So after a reboot an output can sit at its
default until then, and a late output that catches up is accepted. Missed *inputs* (switch
positions) are not tolerable — they'd leave DCS in the wrong state until the next physical change,
which is why the handshake exists. See [Design Decisions](design-decisions.md) for the reasoning.

## Gotchas — get these right

!!! warning "Two CAN HAL settings are correctness requirements"
    | Setting | Required value | Consequence if wrong |
    |---------|---------------|----------------------|
    | `AutoRetransmission` | **DISABLE** | One unACKed frame jams all 3 TX mailboxes → immediate bus-off; the TX queue never drains. |
    | `AutoBusOff` | **ENABLE** | Hardware recovers from bus-off in ~3 ms; with it disabled, firmware must restart the controller by hand. |

    With retransmission off in hardware, the software decides: a frame that loses arbitration is
    sent again until it goes out, one that hits a bus error up to 3 times, and frames always leave
    in order. These are not preferences. The full failure analysis is in
    [Design Decisions D4](design-decisions.md#d4-autoretransmission-disable-this-one-will-catch-you-out).

- **SJW = 4TQ.** Lower values cause intermittent CRC errors from crystal tolerance between
  boards. Keep it at 4TQ permanently — it costs nothing. See
  [D5](design-decisions.md#d5-sjw-4tq-empirically-determined).
- **Batch flush deadline.** On a node, a half-full `ControlPacketPair` goes out within **two
  firmware `loop()` iterations**, so user input never sits waiting for a slot-B packet that may
  never arrive. PanelBridge's `CTRL_BCAST` also waits at least 2 ms for a partner, so the export
  stream fills both slots. Input-snapshot bursts (boot, `SYNC_REQ` re-polls) flush any trailing
  single packet immediately at the end of the poll pass.
- **Nothing is coalesced; a full queue drops the new frame.** Every frame, `CTRL_BCAST` included,
  queues in order. Replacing a queued `CTRL_BCAST` with a newer one would lose two other controls'
  updates each time (about a fifth of the sim's output under a real export stream), so it never
  happens. A dropped frame is counted on the board (`CANProtocol::txDropCount()`), not reported
  over the bus. A healthy bus drops nothing.
