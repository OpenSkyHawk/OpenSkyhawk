# 09 — Startup, Resync, and Diagnostics

**Owns:** boot sequence steps for all three node types, SYNC_REQ/READY handshake protocol,
DiagSerial diagnostics.
**Does not own:** what triggers SYNC_REQ (→ 06), CAN frame IDs (→ 02), hardware pin
assignments (→ 08), HID frame format (→ 03).

---

## Before Any App — the Bootloader

Every STM32 board (PanelGroup, PDU, PanelBridge) starts in its bootloader, which checks the app and
jumps to it after a 200 ms catch window — the boot sequences below begin there. A running PanelGroup
or PDU that receives a CONNECT on its `BOOT_CMD_n` resets back into the bootloader when one is
installed (`STM32Board::enterBootloader()`, #365); READY_n byte 3 reports the bootloader's version. It stays instead when the
app asked for it or the app fails its check, and announces on `BOOT_RSP_n`. See `12-bootloader.md`.

## PanelGroup Boot Sequence

1. `STM32Board::begin()` — release JTAG (SWD kept), init status LED (PB14/PB15), DiagSerial (USART1 115200), CAN HAL.
2. Read `NODE_ID` compile-time define.
3. For each registered expander: init MCP23017, read full port state (baseline), configure
   interrupt pins, call `attachInterrupt()`.
4. Configure CAN receive filter for `CTRL_BCAST` (0x7E0), `TEST_SEQ` (0x011), and
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
   `controlId < 0x8000` → HID frame → SimGateway → USB HID report. If no process is reading the
   USB ports, the host silently discards the data — no special handling needed.
7. Send `READY` frame (`CANProtocol::sendReady()` → `canIdReady(NODE_ID)` = `0x400 + NODE_ID`) —
   signals PanelBridge that the initial input poll and EVT burst are complete, and that this node
   is ready to answer a full-state `SYNC_REQ`. Its payload is the node's firmware version
   (`ReadyPayload`, #295). From here on the node answers every `SYNC_REQ` with a READY flagged
   `RESYNC` too (#383), so a bridge that boots later still learns its version. Then
   `STM32Board::markBootHealthy()` sets the bootloader's crash-loop counter back to 0 (#364) — the
   last line of `setup()`.
8. Heartbeat begins — `CANProtocol::serviceNodeStatus()` arms its timers on its first call, in
   the first `loop()` after boot, so the first HB goes out ~500 ms after boot completes (#340).

---

## PanelGroup — SYNC_REQ Response

On receipt of a `SYNC_REQ` (0x012) for this node — byte 0 = its `NODE_ID`, or an empty one (every
node) — at any time after boot: re-report every registered input and emit batched CAN EVTs, as in
step 6 above (an input with no absolute state sends nothing, `05` § Re-reporting). A `SYNC_REQ`
naming another node is ignored. Once the answer has left — the TX ring empty and all three
mailboxes free, or after 50 ms at most — `CANProtocol::drain()` sends READY flagged `RESYNC`, the end
of the answer (#383, #387); not before the node's own boot READY. This is the PanelGroup's response
to every sync the bridge queues: its boot READY, its first heartbeat after the bridge booted or after
a timeout, and a DCS session change.

No state-sync is requested for outputs. PanelGroup waits passively for the export stream to carry
each output's value: a changed value on the next DCS-BIOS frame, an unchanged one with DCS-BIOS's
autosync, every few seconds. After a reboot an output may sit at its default (zero/off) until then —
late outputs that catch up are acceptable.

---

## PanelBridge Boot Sequence

1. `STM32Board::begin()`.
2. Init UART2 @ 250000 (`Serial`, PA2/PA3) — link to SimGateway.
3. Configure CAN receive filter (pass-all mask — accepts all IDs; software range check in RX
   handler validates HB/EVT/ECHO/READY ranges, discards everything else).
   See `02-can-protocol.md#panelbridge-receive-filter` for range validation rules.
4. `CANProtocol::start()`.
5. No `SYNC_REQ` broadcast (#387): each already-running node is queued for a sync as its first
   heartbeat arrives, and synced one at a time (`06` § Sequential sync). Handles the case where
   PanelGroup nodes booted before PanelBridge.
6. Arm the boot roster reply (`_NODE_STATUS`, node-0 row only at cold boot); it goes out a row at a
   time from `loop()` (#388).
7. `STM32Board::markBootHealthy()` — the bootloader's crash-loop counter back to 0 (#364), the last
   line of `PanelBridge::setup()`.
8. `DcsBios::setup()` is called by the sketch.

---

## PanelBridge — READY Frame Handling

When PanelBridge receives a READY frame (`0x400+n`) from node n, the node is confirmed online
and ready to answer a full-state request. PanelBridge logs the READY event, updates
`lastSeenMs` like a heartbeat, marks the node alive if needed, and queues it for a sync (#387).

READY does not carry input state itself. The state snapshot is the EVT burst emitted when the
PanelGroup receives its `SYNC_REQ`. It carries the node's firmware version instead
(#295): PanelBridge caches it per node for `_NODE_STATUS` and pushes a delta when an alive node's
version changes. An empty READY (older firmware) reads as not reported.

---

## SYNC_REQ / READY Handshake — Timing Race

The handshake solves the case where PanelGroup nodes boot before PanelBridge:

```
Timeline A — PanelBridge boots first (normal case):
  PanelBridge boots (no SYNC_REQ, step 5)
  PanelGroup boots → initial EVT burst (step 6) → READY (step 7)
  PanelBridge receives EVTs during its own boot → routes normally
  PanelBridge receives READY → queues the node → SYNC_REQ(n)
  PanelGroup re-polls → emits current EVTs again → READY(RESYNC) → routes normally

Timeline B — PanelGroup boots first (power sequencing race):
  PanelGroup boots → initial EVT burst (step 6) → READY (step 7)
    [PanelBridge not yet listening — EVTs and READY are lost]
  PanelBridge boots → the node's next HB → queued → SYNC_REQ(n), one node at a time
  PanelGroup receives SYNC_REQ(n) → re-polls → emits EVTs again → READY(RESYNC) → routes normally

Timeline C — Node recovers after heartbeat timeout:
  PanelBridge marks node dead after 3 seconds without HB
  Node resumes and sends HB or READY
  PanelBridge marks node alive → queues it → SYNC_REQ(n) — no other node answers
  PanelGroup re-polls → emits current EVTs again → READY(RESYNC) → routes normally
```

In all timelines, PanelBridge eventually receives all current input states.

---

## SimGateway Boot Sequence

1. Record why this boot happened (power-on / watchdog / recovery reboot) from the watchdog
   scratch registers, for `DIAG` (#355).
2. Set USB identity (VID/PID, product strings) — see `03-uart-usb-hid-protocol.md`.
3. Init UART @ 250000 to PanelBridge, with a 1 KB software RX queue.
4. Load axis calibration from flash; initialise the TinyUSB HID descriptor and enumerate.
5. `SimGateway::setup(Serial1)` returns — relay mode: DCS-BIOS export bytes forwarded
   USB CDC → UART, whole lines UART → USB CDC; HID frame demultiplexer armed on UART RX.
6. The first `SimGateway::loop()` call arms the 3 s hardware watchdog and starts the USB recovery
   ladder (`07-simgateway-api.md#link-health-and-self-recovery`).

SimGateway has no boot handshake — it is stateless with respect to the CAN cluster.

**USB enumeration race:** TinyUSB on RP2040 silently drops HID reports if USB is not yet
enumerated — no crash risk. Any HID frames that arrive on UART before enumeration completes
are parsed and dispatched normally; the resulting report sends are no-ops until
the host is ready. Boot-time HID frame loss is acceptable: axes update on the next physical
movement; buttons are transient and not held at power-on.

---

## DiagSerial Diagnostics

Each STM32 board writes human-readable diagnostic lines (`[BRIDGE] …`, `[NODE] …`, `[PDU] …`) on
**USART1 (DiagSerial) only** — not forwarded to SimGateway, not available on USB CDC. Most of them
appear only with `STM32Board::setDebug(true)`; the templates and panel sketches ship with it off
(#389).

Connect a USB-to-TTL adapter to the 3-pin DiagSerial header (GND/RX/TX at PA9/PA10) on any
STM32 board to observe its output at 115200 baud, independently of the DCS data path.

**No framed DIAG messages.** `CANProtocol.h` still defines `DIAG_MAGIC` and the `DIAG_RTT` /
`DIAG_HB` / `DIAG_ERR` / `DIAG_EVT` frame types from an early plan, but nothing sends them — they are
retired. Of the CAN counters `DIAG_ERR` was meant to carry, TEC, REC and bus-off already travel in
every `HB_n` (and so in `_NODE_STATUS`); the drop counters — `txDropCount()`, `rxDropCount()`,
`rxOverrunCount()` — are counted on the board and not reported over the bus (D32).

Each board's diagnostic output is independent. There is no aggregated bus-wide diagnostic stream
routed to SimGateway. (SimGateway's own `DIAG` report to the host, #355, is about the Gateway's USB
and UART link, not the CAN bus — `07-simgateway-api.md`.)

PanelBridge does not publish diagnostic frames onto CAN. It consumes PanelGroup heartbeat and
ECHO diagnostic information and may report local human-readable/test diagnostics only on its
own DiagSerial header when debug/test output is enabled.

**TEST_SEQ trigger:** a single command byte (e.g. `'T'`) received on PanelBridge's DiagSerial
causes PanelBridge to broadcast TEST_SEQ (0x011) and collect ECHO responses from each node
for RTT measurement. If debug/test DiagSerial output is enabled, results may be printed
locally on PanelBridge's DiagSerial; they are not published onto CAN.
