# PanelBridge — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/06-panelbridge-api.md`, `FirmwarePlan/04-dcs-bios-integration.md`, `FirmwarePlan/03-uart-usb-hid-protocol.md`, `FirmwarePlan/09-startup-resync-diagnostics.md`
**Depends on:** `CANProtocol.md`, `STM32Board.md`, `A4ECGenerator.md` (for `A4EC_InputMap.h` and `A4EC_CmdIds.h`)

---

## Responsibility

PanelBridge is the STM32 CAN master and DCS-BIOS processing node. It owns the DCS-BIOS
listener bridge, `CTRL_BCAST` output submission to CANProtocol, CAN EVT input dispatch,
`SYNC_REQ` recovery requests, node liveness tracking, TEST_SEQ trigger handling, and HID
frame forwarding to SimGateway.

PanelBridge does **not** own CAN frame IDs or queue internals (CANProtocol), HID joystick
dispatch (SimGateway), PanelGroup input/output object behavior, or the generated A-4E-C input
map format. PanelBridge never talks to the HAL CAN peripheral directly; all CAN traffic goes
through CANProtocol.

---

## File Layout

```
Firmware/Libraries/PanelBridge/
├── PanelBridge.h
├── PanelBridge.cpp
└── library.json
```

Included by the PanelBridge sketch:

```cpp
#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>
```

The sketch only includes `<STM32Board.h>` if it directly calls board-level helpers such as
`STM32Board::setDebug(true)`. `PanelBridge::setup()` uses STM32Board internally.

`PanelBridge.cpp` includes the generated dispatch map:

```cpp
#include <A4EC_InputMap.h>
```

PanelGroup sketches include `A4EC_CmdIds.h`; they do not include `A4EC_InputMap.h`.

### Test Project

```
Firmware/Tests/PanelBridge/
├── platformio.ini
└── tests/
    ├── startup_sequence.cpp     — setup order, pass-all filter, CAN start, cold-boot SYNC_REQ
    ├── node_tracking.cpp        — READY sync, alive/dead callbacks, heartbeat timeout/recovery
    ├── input_dispatch_dcs.cpp   — 0x8000-0x86FF routing; 0x8700 and 0xFFFF rejected;
    │                             SWITCH/ACTION/ENCODER/ACCEL_ENCODER/MULTIPOS formatting
    ├── input_dispatch_hid.cpp   — controlId < 0x8000 becomes exact 6-byte HID frame
    ├── output_submission.cpp    — DCS output listener submits ControlPackets to
    │                             CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, ...)
    ├── model_time_resync.cpp    — first value seeds state; later decrease broadcasts SYNC_REQ
    └── test_seq_diag.cpp        — DiagSerial 'T' sends exactly 8 TEST_SEQ bytes; ECHO consumed
```

Tests use fakes for `CANProtocol::send()`, `CANProtocol::sendBatched()`, `Serial`, DiagSerial, and
`sendDcsBiosMessage()` so routing can be validated without DCS or a physical CAN bus.

#### Dual-Board Integration Test (hardware, physical CAN bus)

Tests the startup/reconnect lifecycle that cannot be exercised with fakes. Requires two boards:
- `NODE_ID=0` — PanelBridge role (master, accepts all frames, sends CTRL_BCAST / SYNC_REQ)
- `NODE_ID=1` — PanelGroup role (sub-node, sends HB/EVT/READY, receives CTRL_BCAST / SYNC_REQ)

Both boards run the same sketch file; role is selected by `NODE_ID` at compile time.

**Startup order is not guaranteed.** The test must pass regardless of which board powers on first.
Three timelines must all produce the same end state — see `FirmwarePlan/09-startup-resync-diagnostics.md`:

```
Timeline A — PanelBridge first:
  PanelBridge boots → broadcasts SYNC_REQ
  PanelGroup boots → initial EVT burst → READY
  PanelBridge: receives EVTs, receives READY → broadcasts SYNC_REQ again
  PanelGroup: re-polls, sends current EVTs
  → End state: PanelBridge has current input state

Timeline B — PanelGroup first:
  PanelGroup boots → EVT burst → READY (PanelBridge not listening yet, frames lost)
  PanelBridge boots → broadcasts SYNC_REQ
  PanelGroup receives SYNC_REQ → re-polls → sends EVTs
  → End state: PanelBridge has current input state

Timeline C — Reconnect after dropout:
  Both running → PanelGroup power-cycled or disconnected
  PanelBridge: marks node dead after 3 s no HB → fires onNodeDead
  PanelGroup reconnects → boots → EVT burst → READY
  PanelBridge: receives HB or READY → marks alive → fires onNodeAlive → broadcasts SYNC_REQ
  PanelGroup receives SYNC_REQ → re-polls → sends current EVTs
  → End state: same as Timeline A
```

```
Firmware/Tests/PanelBridge/tests/
└── dual_integration/
    └── dual_integration.cpp    — hardware integration test for all three timelines
                                  NODE_ID=0: PanelBridge role — setup(), loop(); logs
                                    SYNC_REQ broadcasts, onNodeAlive/Dead events, EVT receipts
                                  NODE_ID=1: PanelGroup role — setup(), loop(); logs
                                    READY send, SYNC_REQ receipts, EVT re-poll bursts
                                  Test is open-ended: user reads DiagSerial on both boards
                                  and verifies correct event sequence for each timeline.
                                  Timeline C: power-cycle NODE_ID=1 board while both running.
```

**Key assertions (verified manually from DiagSerial output):**
- `[BRIDGE] SYNC_REQ broadcast` appears: at start AND after each READY/node-alive event
- `[BRIDGE] node=1 alive` appears after PanelGroup boots (any order)
- `[BRIDGE] node=1 dead` appears if PanelGroup is power-cycled; `alive` reappears after reconnect
- `[NODE] SYNC_REQ received, re-polling` appears in response to each SYNC_REQ
- `[NODE] READY sent` appears once per boot, after initial EVT burst

```ini
[env:test_dual_integration_bridge]
extends = env_base
build_flags = -DNODE_ID=0 -DHAL_CAN_MODULE_ENABLED -DUSB_NONE -DHSE_VALUE=8000000
build_src_filter = -<*> +<dual_integration/dual_integration.cpp>

[env:test_dual_integration_node]
extends = env_base
build_flags = -DNODE_ID=1 -DHAL_CAN_MODULE_ENABLED -DUSB_NONE -DHSE_VALUE=8000000
build_src_filter = -<*> +<dual_integration/dual_integration.cpp>
```

**`platformio.ini`:**

```ini
[env_base]
platform = ststm32
board = genericSTM32F103C8
framework = arduino
build_flags = -DNODE_ID=0 -DDCSBIOS_DEFAULT_SERIAL
lib_deps =
    file://../../Libraries/PanelBridge
    file://../../Libraries/CANProtocol
    file://../../Libraries/STM32Board
    file://../../Libraries/A4EC
    dcs-skunkworks/DCS-BIOS

[env:test_startup_sequence]
extends = env_base
build_src_filter = -<*> +<tests/startup_sequence.cpp>

[env:test_node_tracking]
extends = env_base
build_src_filter = -<*> +<tests/node_tracking.cpp>

[env:test_input_dispatch_dcs]
extends = env_base
build_src_filter = -<*> +<tests/input_dispatch_dcs.cpp>

[env:test_input_dispatch_hid]
extends = env_base
build_src_filter = -<*> +<tests/input_dispatch_hid.cpp>

[env:test_output_submission]
extends = env_base
build_src_filter = -<*> +<tests/output_submission.cpp>

[env:test_model_time_resync]
extends = env_base
build_src_filter = -<*> +<tests/model_time_resync.cpp>

[env:test_seq_diag]
extends = env_base
build_src_filter = -<*> +<tests/test_seq_diag.cpp>
```

---

## Public API

```cpp
// PanelBridge.h

#pragma once

#include <stdint.h>

namespace PanelBridge {

    /** @brief Callback fired when a PanelGroup node becomes alive. */
    using NodeCallback = void(*)(uint8_t nodeId);

    /**
     * @brief Register a callback fired when a node transitions from dead/unseen to alive.
     *
     * Callback registration is optional. Passing nullptr disables the callback.
     *
     * @param cb Function called with the PanelGroup node ID, range 1-63.
     */
    void onNodeAlive(NodeCallback cb);

    /**
     * @brief Register a callback fired when a live node misses the heartbeat timeout.
     *
     * Callback registration is optional. Passing nullptr disables the callback.
     *
     * @param cb Function called with the PanelGroup node ID, range 1-63.
     */
    void onNodeDead(NodeCallback cb);

    /**
     * @brief Initialise STM32 board services, UART2, CANProtocol, and PanelBridge internals.
     *
     * Calls STM32Board::begin(), starts Serial at 250000 baud for the SimGateway link,
     * configures CANProtocol with a pass-all receive filter, registers the CAN RX callback,
     * starts CAN, and broadcasts cold-boot SYNC_REQ.
     *
     * @note DcsBios::setup() is called by the sketch after PanelBridge::setup().
     */
    void setup();

    /**
     * @brief Run PanelBridge work that is not owned by DcsBios::loop().
     *
     * Drains CANProtocol RX, dispatches EVT packets, services CANProtocol batching deadlines,
     * checks node heartbeat timeouts, and handles DiagSerial TEST_SEQ trigger bytes.
     */
    void loop();

} // namespace PanelBridge
```

`PanelBridge::setup()` takes no UART argument. The DCS-BIOS library owns `Serial` directly
through `DCSBIOS_DEFAULT_SERIAL`.

---

## Sketch Contract

```cpp
#define DCSBIOS_DEFAULT_SERIAL
#include <DcsBios.h>
#include <PanelBridge.h>

void onNodeAlive(uint8_t nodeId) { /* optional diagnostics */ }
void onNodeDead(uint8_t nodeId)  { /* optional diagnostics */ }

void setup() {
    PanelBridge::onNodeAlive(onNodeAlive);
    PanelBridge::onNodeDead(onNodeDead);
    PanelBridge::setup();
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
    PanelBridge::loop();
}
```

Loop order is intentional. `DcsBios::loop()` consumes the SimGateway -> PanelBridge binary
export stream and fires DCS-BIOS listeners. Those listeners submit DCS output packets to
CANProtocol's batching path. `PanelBridge::loop()` then services CANProtocol deadlines and
processes CAN EVTs from PanelGroup nodes.

---

## Key Data Structures

### Node State

PanelBridge tracks PanelGroup node IDs 1-63. Node 0 is PanelBridge itself and is never tracked
as a child node.

```cpp
struct NodeState {
    bool alive;
    bool everSeen;
    uint32_t lastSeenMs;
    HeartbeatPayload lastHeartbeat;
};
```

Rules:

- `READY_n` updates `lastSeenMs`, marks node n as seen/alive, fires `onNodeAlive(n)` if it
  was previously dead or unseen, and broadcasts `SYNC_REQ`.
- `HB_n` updates `lastSeenMs` and `lastHeartbeat`.
- `HB_n` from a dead or unseen node marks it alive, fires `onNodeAlive(n)`, and broadcasts
  `SYNC_REQ`.
- A live node with no heartbeat for 3000 ms is marked dead and fires `onNodeDead(n)` once.
- No `SYNC_REQ` is sent when a node merely becomes dead; the recovery sync happens when it
  announces it is alive again.

### DCS Output Submission

DCS-BIOS output callbacks arrive as individual address/value updates. PanelBridge converts
each update to a `ControlPacket` and submits it to CANProtocol:

```cpp
CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, ControlPacket{address, value});
```

CANProtocol owns `ControlPacketPair` construction, the half-full batch deadline, mailbox
handling, and the software TX queue. PanelBridge does not keep a pending output batch and
does not form `ControlPacketPair` payloads itself.

### TEST_SEQ Payload

PanelBridge emits an 8-byte TEST_SEQ payload so echoed frames can produce RTT diagnostics.

```cpp
struct __attribute__((packed)) TestSeqPayload {
    uint16_t seq;
    uint32_t sentMs;
    uint16_t reserved;
};
static_assert(sizeof(TestSeqPayload) == 8);
```

Preferred implementation is explicit serialization into `uint8_t payload[8]`:
bytes 0-1 = `seq`, bytes 2-5 = `sentMs`, bytes 6-7 = `reserved`, all little-endian. Do not
send `sizeof(TestSeqPayload)` from an unpacked struct; natural alignment can make it larger
than the CAN payload.

PanelGroup nodes echo the 8 bytes unchanged in `ECHO_n`. On receipt, PanelBridge subtracts
`sentMs` from the current `millis()` value. PanelBridge consumes this diagnostic information
locally; it does not publish diagnostic frames onto CAN.

---

## Implementation Notes

### Setup Sequence

`PanelBridge::setup()` performs the firmware-owned portion of the boot sequence:

1. `STM32Board::begin()`.
2. `Serial.begin(250000)` for the UART2 link to SimGateway.
3. Register the DCS-BIOS export listener / adapter.
4. `CANProtocol::filterAcceptAll()`.
5. Register `CANProtocol::onReceive()` callback.
6. `CANProtocol::start()`.
7. Send cold-boot `SYNC_REQ` with DLC 0.

The sketch then calls `DcsBios::setup()`.

### CAN RX Validation

PanelBridge uses a pass-all CAN filter because the valid receive ranges cannot be expressed
as one STM32 CAN mask. The heartbeat address family is still `canIdHb(n) = 0x100 + n`;
`HB_0` at `0x100` is reserved but not transmitted, and `HB_1`-`HB_63` are child-node
liveness frames. The RX handler validates IDs in software:

| Range | Accepted by PanelBridge | Action |
|-------|--------------------------|--------|
| `0x101`-`0x13F` | `HB_1`-`HB_63` | Update node heartbeat/liveness |
| `0x201`-`0x23F` | `EVT_1`-`EVT_63` | Dispatch each non-null ControlPacket slot |
| `0x301`-`0x33F` | `ECHO_1`-`ECHO_63` | Consume TEST_SEQ RTT diagnostic response |
| `0x401`-`0x43F` | `READY_1`-`READY_63` | Mark node ready and broadcast `SYNC_REQ` |

All other IDs are discarded silently. `HB_0` (`0x100`) is not accepted as a child-node
heartbeat and is not transmitted by PanelBridge.

### READY and Recovery SYNC_REQ

READY is a signal that a PanelGroup completed its boot-time input poll and can answer a
full-state request. READY carries no input data. On READY, PanelBridge updates `lastSeenMs`,
marks the node alive if needed, and broadcasts `SYNC_REQ`.

Heartbeat recovery uses the same mechanism. If a node was unseen or marked dead and later
sends HB, PanelBridge marks it alive and broadcasts `SYNC_REQ`.

`SYNC_REQ` is currently a global frame. Every PanelGroup that receives it re-polls only the
input objects registered on that specific PanelGroup and emits EVTs for those inputs. Duplicate
snapshots are acceptable because READY and recovery events are rare, and each EVT is routed
idempotently by the normal input dispatch path.

### DCS-BIOS Output Listener

PanelBridge owns an internal DCS-BIOS listener/adapter. The adapter is private to
`PanelBridge.cpp`; no DCS-BIOS listener type appears in `PanelBridge.h`.

```cpp
class BridgeExportListener : public DcsBios::ExportStreamListener {
public:
    BridgeExportListener()
        : DcsBios::ExportStreamListener(0x8000, 0x86FF) {}

    void onDcsBiosWrite(unsigned int address, unsigned int data) override;
};
```

For each DCS-BIOS export update in the cockpit output address range (`0x8000`-`0x86FF`):

1. Build `ControlPacket{ address, value }`.
2. Call `CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, packet)`.
3. Let CANProtocol form the `ControlPacketPair`, enforce the half-full batch deadline, and
   apply the frame-type TX queue policy.

The callback forwards to an internal helper so the listener path is testable without the
DCS-BIOS library:

```cpp
void handleDcsBiosExport(uint16_t address, uint16_t value);
```

### DCS Session Change Detection

The DCS-BIOS export stream includes a five-character model-time seconds string exported by
CommonData as `CommonData_MOD_TIME_A` (`0x0440` in the current generated `Addresses.h`).
PanelBridge uses the generated symbol, not a hard-coded number:

```cpp
DcsBios::StringBuffer<5> modelTimeBuffer(CommonData_MOD_TIME_A, onModelTimeChange);
```

`onModelTimeChange(char* value)` converts the string to seconds. The first valid value only
seeds the previous-value state. If a later value decreases, DCS has loaded a new mission or
aircraft session and PanelBridge broadcasts `SYNC_REQ`.

`SYNC_REQ` asks PanelGroup nodes to queue physical input states for reporting. DCS output
values do not need an active refresh task in PanelBridge; they arrive on the next normal
DCS-BIOS publishing cycle and flow through the regular output listener.

The model-time buffer remains internal to `PanelBridge.cpp`; no model-time API is exposed in
`PanelBridge.h`.

### EVT Input Dispatch

PanelBridge drains CANProtocol in `loop()`. For each valid `EVT_n` frame, payload length must
be exactly 8 bytes and is interpreted as a `ControlPacketPair`. Slot A is processed first.
Slot B is processed only when `b.controlId != 0x0000`.

Dispatch is based only on `controlId`:

| `controlId` range | Destination |
|-------------------|-------------|
| `0x0000` | Null sentinel — ignore |
| `< 0x8000` | HID frame over UART to SimGateway |
| `0x8000`-`0x86FF` | Binary search `A4EC_INPUT_MAP`, then `sendDcsBiosMessage()` |
| anything else | Drop and report locally for diagnostics/tests |

PanelBridge does not care which physical node sent a DCS-BIOS input. Multiple PanelGroup
inputs may use the same `DCSIN_*` constant if they intentionally trigger the same DCS command.

### DCS-BIOS Input Formatting

For `controlId` in `0x8000`-`0x86FF`, PanelBridge finds the matching `DcsBiosInputEntry` by
binary search over `A4EC_INPUT_MAP`.

| Input type | Valid values | Argument passed to `sendDcsBiosMessage()` |
|------------|--------------|-------------------------------------------|
| `InputType::SWITCH` | 0, 1 | `arg0` for 0, `arg1` for 1 |
| `InputType::ACTION` | non-zero press | `arg0`; value 0 is ignored |
| `InputType::ENCODER` | 0, 1 | `arg0` for 0, `arg1` for 1 |
| `InputType::ACCEL_ENCODER` | 0, 1, 2, 3 | `arg0`, `arg1`, `arg0fast`, `arg1fast` |
| `InputType::MULTIPOS` | 0-65535 | decimal string representation of `value` |

Invalid values, unknown `controlId`s, or entries with null required arguments are dropped and
counted/logged locally for diagnostics/tests. PanelBridge does not publish diagnostic frames
onto CAN. `MULTIPOS` formatting uses a local buffer of at least 6 bytes (`"65535"` plus null
terminator).

### HID Frame Forwarding

For non-null `controlId < 0x8000`, PanelBridge writes the 6-byte HID frame defined in
`03-uart-usb-hid-protocol.md` to `Serial`:

| Byte offset | Field |
|-------------|-------|
| 0 | `0xAA` |
| 1 | `0x55` |
| 2-3 | `controlId`, little-endian |
| 4-5 | `value`, little-endian |

The HID frame shares the PanelBridge -> SimGateway UART direction with DCS-BIOS ASCII commands.
The non-ASCII magic bytes cannot collide with `sendDcsBiosMessage()` output.

### TEST_SEQ and Diagnostics

PanelBridge watches DiagSerial (`Serial1`) for a single command byte, `T`. On receipt:

1. Increment the TEST_SEQ sequence number.
2. Send `CAN_ID_TEST_SEQ` with an 8-byte `TestSeqPayload`.
3. Store outstanding sequence metadata until ECHO replies arrive.
4. For each valid `ECHO_n`, consume the response for local diagnostics/test assertions. If
   debug/test DiagSerial output is enabled, print the node ID, sequence, sent timestamp, and
   measured RTT locally.

SimGateway is not involved in diagnostics. DIAG output is only on the STM32 USART1 diagnostic
header. PanelBridge does not publish diagnostic messages onto CAN; it only consumes
PanelGroup HB/ECHO diagnostic data and optionally logs locally.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| DCS-BIOS Arduino library | Firmware dependency | PanelBridge runs DCS-BIOS on `Serial` via `DCSBIOS_DEFAULT_SERIAL` |
| CANProtocol | `Firmware/Libraries/CANProtocol` | Frame IDs, ControlPacketPair, HeartbeatPayload, send/sendBatched/drain API |
| STM32Board | `Firmware/Libraries/STM32Board` | Board init, DiagSerial, CAN HAL init, status LED |
| A4EC generated headers | `tools/gen_a4ec` output | `A4EC_InputMap.h` included by PanelBridge only |
| SimGateway UART contract | FirmwarePlan/03 | HID frame format and raw ASCII forwarding assumptions |
| STM32duino Arduino core | PlatformIO `framework = arduino` | `Serial`, `Serial1`, `millis()` |
