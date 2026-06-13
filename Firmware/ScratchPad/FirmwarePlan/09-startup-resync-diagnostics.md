# 09 — Startup, Resync, and Diagnostics

**Owns:** boot sequence steps for all three node types, SYNC_REQ/READY handshake protocol,
DIAG frame definitions.
**Does not own:** what triggers SYNC_REQ (→ 06), CAN frame IDs (→ 02), hardware pin
assignments (→ 08), HID frame format (→ 03).

---

## PanelGroup Boot Sequence

1. `STM32Board::begin()` — init status LED (PB14/PB15), DiagSerial (USART1 115200), CAN HAL.
2. Read `NODE_ID` compile-time define.
3. For each registered expander: init MCP23017, read full port state (baseline), configure
   interrupt pins, call `attachInterrupt()`.
4. Configure CAN receive filter for `CTRL_BCAST` (0x010), `TEST_SEQ` (0x011), and
   `SYNC_REQ` (0x012).
5. `canStart()` — enable CAN peripheral.
6. **Poll all registered input objects and send CAN EVTs with current values.** EVTs use
   CANProtocol-owned `ControlPacketPair` batching; if only one event is ready, slot B uses
   `controlId = 0x0000`. During this boot snapshot pass, PanelGroup calls
   `CANProtocol::flushBatched(canIdEvt(NODE_ID))` so the odd trailing packet flushes
   immediately before READY. During normal runtime, CANProtocol flushes a half-full EVT
   batch no later than two `PanelGroup::loop()` iterations after slot A is queued.
   PanelBridge routes each non-null packet normally:
   `0x8000 <= controlId <= 0x86FF` → `sendDcsBiosMessage()` → ASCII on UART → SimGateway → USB CDC;
   `controlId < 0x8000` → HID frame → SimGateway → Joystick. If no process is reading the
   USB ports, the host silently discards the data — no special handling needed.
7. Send `READY` frame (`canIdReady(NODE_ID)` = `0x400 + NODE_ID`) — signals PanelBridge
   that the initial input poll and EVT burst are complete, and that this node is ready to
   answer a full-state `SYNC_REQ`.
8. Begin heartbeat timer (first HB sent 500 ms after boot).

---

## PanelGroup — SYNC_REQ Response

On receipt of `SYNC_REQ` (0x012) at any time after boot: re-poll all registered input objects
and emit batched CAN EVTs — identical to step 6 above. This is the PanelGroup's response to
cold boot, READY-triggered requests, heartbeat recovery requests, and DCS session change
requests.

No state-sync is requested for outputs. PanelGroup waits passively for the next DCS-BIOS update
cycle (~50 ms) to receive current output values. Outputs may be at default (zero/off) for up to
~50 ms after a resync — this is acceptable.

---

## PanelBridge Boot Sequence

1. `STM32Board::begin()`.
2. Init UART2 @ 250000 (`Serial`, PA2/PA3) — link to SimGateway.
3. Configure CAN receive filter (pass-all mask — accepts all IDs; software range check in RX
   handler validates HB/EVT/ECHO/READY ranges, discards everything else).
   See `02-can-protocol.md#panelbridge-receive-filter` for range validation rules.
4. `canStart()`.
5. Broadcast `SYNC_REQ` (0x012) — causes any already-running PanelGroup nodes to re-poll
   and emit their current input states. Handles the case where PanelGroup nodes booted before
   PanelBridge.
6. `DcsBios::setup()` is called by the sketch.

---

## PanelBridge — READY Frame Handling

When PanelBridge receives a READY frame (`0x400+n`) from node n, the node is confirmed online
and ready to answer a full-state request. PanelBridge logs the READY event, updates
`lastSeenMs` like a heartbeat, marks the node alive if needed, and broadcasts `SYNC_REQ`.

READY does not carry input state itself. The state snapshot is the EVT burst emitted when the
PanelGroup receives the following `SYNC_REQ`.

---

## SYNC_REQ / READY Handshake — Timing Race

The handshake solves the case where PanelGroup nodes boot before PanelBridge:

```
Timeline A — PanelBridge boots first (normal case):
  PanelBridge boots → broadcasts SYNC_REQ (step 5)
  PanelGroup boots → initial EVT burst (step 6) → READY (step 7)
  PanelBridge receives EVTs during its own boot → routes normally
  PanelBridge receives READY → broadcasts SYNC_REQ
  PanelGroup re-polls → emits current EVTs again → routes normally

Timeline B — PanelGroup boots first (power sequencing race):
  PanelGroup boots → initial EVT burst (step 6) → READY (step 7)
    [PanelBridge not yet listening — EVTs and READY are lost]
  PanelBridge boots → broadcasts SYNC_REQ (step 5)
  PanelGroup receives SYNC_REQ → re-polls → emits EVTs again → routes normally

Timeline C — Node recovers after heartbeat timeout:
  PanelBridge marks node dead after 3 seconds without HB
  Node resumes and sends HB or READY
  PanelBridge marks node alive → broadcasts SYNC_REQ
  PanelGroup re-polls → emits current EVTs again → routes normally
```

In all timelines, PanelBridge eventually receives all current input states.

---

## SimGateway Boot Sequence

1. Set USB identity (VID/PID, product strings) — see `03-uart-usb-hid-protocol.md`.
2. Init Joystick (`use16bit(true)`, `useManualSend(true)`).
3. Init UART @ 250000 to PanelBridge.
4. `SimGateway::setup(Serial1)` — enters relay mode: raw DCS-BIOS bytes forwarded
   USB CDC ↔ UART; HID frame demultiplexer armed on UART RX.

SimGateway has no boot handshake — it is stateless with respect to the CAN cluster.

**USB enumeration race:** TinyUSB on RP2040 silently drops HID reports if USB is not yet
enumerated — no crash risk. Any HID frames that arrive on UART before enumeration completes
are parsed and dispatched normally; the resulting `Joystick.send()` calls are no-ops until
the host is ready. Boot-time HID frame loss is acceptable: axes update on the next physical
movement; buttons are transient and not held at power-on.

---

## DIAG Frames

DIAG frames carry diagnostic telemetry. Output on **USART1 (DiagSerial) on each STM32 board
only** — not forwarded to SimGateway, not available on USB CDC.

Connect a USB-to-TTL adapter to the 3-pin DiagSerial header (GND/RX/TX at PA9/PA10) on any
STM32 board to observe its output at 115200 baud, independently of the DCS data path.

| Frame type | Content |
|------------|---------|
| `DIAG_RTT` | RTT ping-pong result (seq, sent timestamp) |
| `DIAG_HB` | Sub-node heartbeat (nodeId, rxCount) |
| `DIAG_ERR` | CAN error counters (TEC, REC, busOff flag, CAN TX queue drop count) |

Each board's DIAG output is independent. There is no aggregated bus-wide diagnostic stream
routed to SimGateway. SimGateway has no DIAG callbacks.

PanelBridge does not publish diagnostic frames onto CAN. It consumes PanelGroup heartbeat and
ECHO diagnostic information and may report local human-readable/test diagnostics only on its
own DiagSerial header when debug/test output is enabled.

**TEST_SEQ trigger:** a single command byte (e.g. `'T'`) received on PanelBridge's DiagSerial
causes PanelBridge to broadcast TEST_SEQ (0x011) and collect ECHO responses from each node
for RTT measurement. If debug/test DiagSerial output is enabled, results may be printed
locally on PanelBridge's DiagSerial; they are not published onto CAN.
