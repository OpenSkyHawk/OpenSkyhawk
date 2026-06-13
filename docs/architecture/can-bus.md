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
| `CTRL_BCAST` | `0x010` | `CAN_ID_CTRL_BCAST` | PanelBridge → All | 8 |
| `TEST_SEQ` | `0x011` | `CAN_ID_TEST_SEQ` | PanelBridge → All | 8 |
| `SYNC_REQ` | `0x012` | `CAN_ID_SYNC_REQ` | PanelBridge → All | 0 |
| `HB_n` | `0x100 + n` | `canIdHb(n)` | Node *n* → PanelBridge | 8 |
| `EVT_n` | `0x200 + n` | `canIdEvt(n)` | Node *n* → PanelBridge | 8 |
| `ECHO_n` | `0x300 + n` | `canIdEcho(n)` | Node *n* → PanelBridge | 8 |
| `READY_n` | `0x400 + n` | `canIdReady(n)` | Node *n* → PanelBridge | 0 |

- `CTRL_BCAST` carries DCS-BIOS output state from PanelBridge to every node.
- `EVT_n` carries input events from a node back up to PanelBridge.
- `HB_n` is a 500 ms heartbeat with CAN health (TEC/REC, flags, uptime).
- `TEST_SEQ` / `ECHO_n` are a round-trip latency test: PanelBridge sends `TEST_SEQ`, a node
  echoes on `ECHO_n`.
- `SYNC_REQ` / `READY_n` are the boot/resync handshake (below).

!!! note "canIdHb(1) = 0x101, not 0x100"
    `canIdHb(0)` (`0x100`) is reserved for PanelBridge and **is never transmitted** — the
    master has no consumer for its own heartbeat. Real nodes start at `canIdHb(1) = 0x101`.
    The old `CAN_ID_HB_1 = 0x100` constant is deprecated; use the `canIdHb(n)` helper.

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

`CTRL_BCAST` and `EVT_n` frames carry a **`ControlPacketPair`** so two packets share one
8-byte frame. When only one packet is ready, slot B's `controlId` is set to the null sentinel
`0x0000`. Batching is owned by `CANProtocol::sendBatched()` / `flushBatched()` — callers
submit individual `ControlPacket`s and the library packs them.

## NODE_ID scheme (brief)

`NODE_ID` is a compile-time constant (`build_flags = -DNODE_ID=N` in `platformio.ini`), range
0–63. **0** is PanelBridge (reserved); **1–63** are PanelGroup nodes, assigned incrementally
and permanent once assigned. SimGateway has no NODE_ID. Full policy and the live registry are
on the [NODE_ID & CAN Addressing](../firmware/node-id.md) page.

## Startup and sync sequence

PanelGroup nodes may power up before or after PanelBridge, so boot state can't be assumed.
The `SYNC_REQ` / `READY` handshake makes input state deterministic:

1. On cold boot a PanelGroup completes its initial input poll, then sends `READY_n`.
2. PanelBridge broadcasts `SYNC_REQ` — on its own cold boot, on a DCS session change, when it
   sees a `READY_n`, or when a node transitions from unseen to alive.
3. On receiving `SYNC_REQ`, every PanelGroup re-polls all its inputs and re-sends their
   current state as `EVT_n` frames.

This guarantees PanelBridge can request a fresh input snapshot at any time. Missed *outputs*
(LEDs, gauges) are tolerable — a fresh DCS-BIOS update arrives within ~50 ms. Missed *inputs*
(switch positions) are not — they'd leave DCS in the wrong state until the next physical
change, which is why the handshake exists. See
[Design Decisions](design-decisions.md) for the reasoning.

## Gotchas — get these right

!!! warning "Two CAN HAL settings are correctness requirements"
    | Setting | Required value | Consequence if wrong |
    |---------|---------------|----------------------|
    | `AutoRetransmission` | **DISABLE** | One unACKed frame jams all 3 TX mailboxes → immediate bus-off; the TX queue never drains. |
    | `AutoBusOff` | **ENABLE** | Hardware recovers from bus-off in ~3 ms; with it disabled, firmware must restart the controller by hand. |

    These are not preferences. The full failure analysis is in
    [Design Decisions D4](design-decisions.md#d4-autoretransmission-disable-this-one-will-catch-you-out).

- **SJW = 4TQ.** Lower values cause intermittent CRC errors from crystal tolerance between
  boards. Keep it at 4TQ permanently — it costs nothing. See
  [D5](design-decisions.md#d5-sjw-4tq-empirically-determined).
- **Batch flush deadline.** A half-full `ControlPacketPair` must flush within **two firmware
  `loop()` iterations** — user input must not sit waiting for a slot-B packet that may never
  arrive. Input-snapshot bursts (boot, `SYNC_REQ` re-polls) flush any trailing single packet
  immediately at the end of the poll pass.
- **TX overflow is frame-type dependent.** `CTRL_BCAST` drops stale state and keeps the
  newest (coalescing by `controlId`); `EVT_n` and the special frames use bounded retry (3
  attempts) then drop and bump a diagnostic counter. A healthy bus drops nothing — any
  `EVT_n` drop is a signal, not normal.
