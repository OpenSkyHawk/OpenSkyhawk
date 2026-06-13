# 06 — PanelBridge API

**Owns:** Node tracking, `ExportStreamListener` contract, input dispatch orchestration,
SYNC_REQ broadcast triggers, DCS session change detection, and DCS output submission to CAN.
**Does not own:** CAN frame IDs or queue implementation details (→ 02), DCS-BIOS entry struct
and generator rules (→ 04), boot sequence steps (→ 09), UART/HID protocol (→ 03).

---

## Role

PanelBridge is the single STM32F103 CAN master that:

1. Runs the DCS-BIOS library on UART2 (`Serial`, PA2/PA3 — the link to SimGateway).
2. Receives all DCS-BIOS export values and submits them to CANProtocol's `CTRL_BCAST`
   batching path.
3. Receives CAN EVTs from PanelGroup nodes and routes them:
   - `0x8000 <= controlId <= 0x86FF` → binary search in generated input map → `sendDcsBiosMessage()`
   - `controlId < 0x8000` → wrap in HID frame → UART → SimGateway
4. Tracks PanelGroup liveness from `HB_1`-`HB_63`. PanelBridge does not publish `HB_0`;
   `canIdHb(0)` is reserved by the ID formula but unused.

---

## Node Tracking

PanelBridge tracks sub-node liveness via heartbeat frames:

```cpp
void onNodeAlive(uint8_t nodeId) { /* fires when a node transitions dead/unseen → alive */ }
void onNodeDead(uint8_t nodeId)  { /* fires if no HB seen for 3 seconds */ }

PanelBridge::onNodeAlive(onNodeAlive);
PanelBridge::onNodeDead(onNodeDead);
```

Node alive/dead callbacks are optional; PanelBridge continues to function without them.

**Heartbeat recovery handling:** When PanelBridge receives `HB_n` from a node that was
previously unseen or marked dead, it marks the node alive, fires `onNodeAlive(nodeId)`, and
broadcasts `SYNC_REQ`. That asks the newly available node to re-poll its inputs and send its
current state. `SYNC_REQ` is global, so other nodes may re-send their current state too; this
is acceptable because node joins and recoveries are rare.

**READY frame handling:** When PanelBridge receives a READY frame (`0x400+n`) from node n,
the node is confirmed online and ready to answer a full-state request. PanelBridge logs the
READY event, updates `lastSeenMs` like a heartbeat, marks the node alive if needed, and
broadcasts `SYNC_REQ`. The READY frame carries no input state itself; the following EVTs
emitted in response to `SYNC_REQ` are the state snapshot.

---

## ExportStreamListener

PanelBridge subclasses the DCS-BIOS `ExportStreamListener` to intercept all output values.
Auto-registers during `PanelBridge::setup()`. No sketch declarations required.

The listener overrides the DCS-BIOS Arduino library callback:

```cpp
void onDcsBiosWrite(unsigned int address, unsigned int data);
```

For each DCS-BIOS export address in the `0x8000`-`0x86FF` range, the listener builds a
`ControlPacket` and calls:

```cpp
CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, packet);
```

CANProtocol owns `ControlPacketPair` construction, the half-full batch deadline, mailbox
handling, and TX queue overflow policy. PanelBridge does not maintain a pending output queue.
All PanelGroup nodes receive every broadcast and each output object checks whether each
non-null packet's registered address matches.

---

## Input Dispatch

`PanelBridge::loop()` drains the CAN EVT queue each iteration:

```
Receive CAN EVT ControlPacketPair {slot A, slot B, nodeId}
│
├── for each non-null slot:
│
│   ├── 0x8000 <= controlId <= 0x86FF  →  binary search in A4EC_InputMap
│   │     sendDcsBiosMessage(entry.name, formatted_arg)
│   │     → raw ASCII on UART → SimGateway → USB CDC → DCS
│
│   └── controlId < 0x8000   →  wrap in HID frame (see 03-uart-usb-hid-protocol.md)
│         send over UART to SimGateway
│         SimGateway sees HID_MAGIC → dispatch to HIDAxis / HIDButton
```

The input map (`A4EC_InputMap.h`) is included by PanelBridge only — not by PanelGroup sketches.
See `04-dcs-bios-integration.md` for struct layout and value encoding.
Slot B with `controlId = 0x0000` is the null sentinel and is ignored. Values above
`0x86FF`, including `0xFFFF`, are not DCS-BIOS inputs and are dropped/logged locally.

---

## SYNC_REQ Broadcast Triggers

PanelBridge broadcasts `SYNC_REQ` (CAN frame `0x012`) in these cases:

1. **Cold boot** — immediately after CAN start, before `DcsBios::setup()`. Causes any
   already-running PanelGroup nodes to re-poll and emit their current input states.
2. **DCS session change** — when `model_time` in the DCS-BIOS binary stream decreases (goes
   backward), indicating DCS has loaded a new aircraft or mission.
3. **READY received** — when a PanelGroup announces that its boot-time input poll and initial
   EVT burst are complete.
4. **Node recovery** — when heartbeat tracking observes a transition from dead/unseen to alive.

Each PanelGroup node that receives `SYNC_REQ` re-polls every registered input object and sends
a CAN EVT with its current value. PanelBridge routes each EVT normally.

> **Note:** `DcsBios::resetAllStates()` re-sends panel input states to DCS and is useful when
> DCS-BIOS input handlers are registered locally on the running MCU. In the OpenSkyhawk
> architecture, input handling is distributed across PanelGroup nodes, so the `SYNC_REQ` CAN
> broadcast is the correct equivalent.

---

## DCS Session Change Detection

The DCS-BIOS binary stream includes a five-character model-time seconds string exported by
CommonData as `CommonData_MOD_TIME_A` (`0x0440` in the current generated `Addresses.h`).
PanelBridge uses the generated symbol, not a hard-coded number:

```cpp
DcsBios::StringBuffer<5> modelTimeBuffer(CommonData_MOD_TIME_A, onModelTimeChange);
```

`onModelTimeChange(char* value)` converts the string to seconds and compares it with the
previous value. The first valid value only seeds the previous-value state. When a later value
decreases, PanelBridge broadcasts `SYNC_REQ`.

`SYNC_REQ` only asks PanelGroup nodes to re-send their physical input states. PanelBridge does
not run an active output refresh task; DCS outputs wait for the next normal DCS-BIOS publishing
cycle, which is expected within roughly 50 ms. LEDs and gauges then update naturally from the
usual `ExportStreamListener` path.

---

## Sketch Structure

DCS-BIOS library is configured to use `Serial` (UART2, PA2/PA3) via the `DCSBIOS_DEFAULT_SERIAL`
define. The sketch calls `PanelBridge::setup()` then `DcsBios::setup()` and loops both.
No per-control declarations are needed — the generated input map covers everything.

```cpp
#define DCSBIOS_DEFAULT_SERIAL   // DCS-BIOS → Serial (UART2 PA2/PA3 → SimGateway)
#include <DcsBios.h>
#include <PanelBridge.h>

void setup() {
    PanelBridge::onNodeAlive(onNodeAlive);
    PanelBridge::onNodeDead(onNodeDead);
    PanelBridge::setup();                 // CAN init; DCS-BIOS stream on Serial
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();       // receive DCS-BIOS stream; fire ExportStreamListener
    PanelBridge::loop();   // drain CAN EVTs; service batching deadlines; route inputs
}
```

`PanelBridge.h` must expose `PanelBridge::setup()` with no UART argument. `Serial` maps to
UART2 (PA2/PA3) in this STM32duino configuration and is owned directly by DCS-BIOS.

The sketch only needs `#include <STM32Board.h>` if it explicitly calls board-level helpers
such as `STM32Board::setDebug(true)`. `PanelBridge::setup()` uses STM32Board internally.

---

## TEST_SEQ

A single command byte (e.g. `'T'`) received on DiagSerial causes PanelBridge to broadcast
`TEST_SEQ` (0x011) and collect ECHO responses for RTT measurement. SimGateway is not involved.
PanelBridge does not publish diagnostics onto CAN; it only consumes ECHO responses and may
print local debug/test output on its own DiagSerial header.
