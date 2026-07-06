# CANProtocol — Technical Specification

**Status:** Done
**FirmwarePlan ref:** `FirmwarePlan/02-can-protocol.md`, `FirmwarePlan/06-panelbridge-api.md`,
`FirmwarePlan/05-panelgroup-api.md`, `FirmwarePlan/09-startup-resync-diagnostics.md`
**Depends on:** `HIDControls.md` — `CANProtocol.h` includes `HIDControls.h`; HIDControls must exist first

---

## Responsibility

Owns all CAN bus interaction for OpenSkyhawk STM32 nodes. Provides shared types and
constants (ControlPacket, CanStatus, frame IDs, CAN ID functions) used by every node, and a
runtime API used by PanelGroup and PanelBridge to send frames, receive frames, and configure
filters.

CAN arbitration IDs (`CAN_ID_*`, `canId*()`) and payload `ControlPacket::controlId`
values are separate namespaces. Equal numeric values in the two namespaces do not
conflict because they occupy different CAN frame fields.

PanelGroup and PanelBridge never access the HAL CAN peripheral directly — all CAN operations
go through CANProtocol.

Does **not** own the CAN peripheral hardware configuration (prescaler, baud rate, GPIO) —
that is `STM32Board::begin()`. Does not own DCS-BIOS routing, input dispatch, or output
dispatch — those belong to PanelBridge and PanelGroup respectively.

---

## File Layout

```
Firmware/Libraries/CANProtocol/
├── CANProtocol.h     ← types, constants, inline ID functions, full namespace API
├── CANProtocol.cpp   ← runtime implementation
└── library.json
```

Included by every STM32 node:
```cpp
#include <CANProtocol.h>
```

`CANProtocol.h` includes `<HIDControls.h>` — so CTRL_* controlId constants are available
to any STM32 sketch that includes `<CANProtocol.h>`, without a separate include. HIDControls
is a dedicated platform-agnostic library (`Firmware/Libraries/HIDControls/`) shared by
CANProtocol (STM32) and SimGateway sketches (RP2040).

Implemented as a `namespace` — there is always exactly one CAN bus per firmware build.

### Test project

```
Firmware/Tests/CANProtocol/
├── platformio.ini
└── tests/
    ├── protocol_layout.cpp     — static_assert sizeof(ControlPacket)==4,
    │                             sizeof(ControlPacketPair)==8, sizeof(HeartbeatPayload)==8;
    │                             canIdHb(0)==0x100 reserved, canIdHb(1)==0x101; slot B null
    │                             sentinel is controlId == 0x0000
    ├── loopback_filter.cpp     — filterAcceptId / filterAcceptAll; mandatory IDs always present;
    │                             frame with non-accepted ID not delivered to onReceive
    ├── rx_dispatch.cpp         — SYNC_REQ fires onSyncReq (not onReceive); TEST_SEQ auto-sends
    │                             ECHO with the same 8-byte payload and is not forwarded to
    │                             onReceive; other frames reach onReceive; 8-byte
    │                             ControlPacketPair payloads are delivered intact
    ├── tx_batching.cpp         — sendBatched() forms ControlPacketPair payloads for CTRL_BCAST
    │                             and EVT_n only; half-full batches flush by deadline; explicit
    │                             flushBatched() emits slot-B null
    ├── tx_queue.cpp            — rapid send() fills mailboxes; txDropCount() increments on overflow;
    │                             verifies frame-type policy: CTRL_BCAST keeps newest state,
    │                             EVT/control frames retry up to 3 attempts then drop; queued
    │                             frames preserve full payload bytes
    ├── status_callbacks.cpp    — onStatusChange fires STARTING before start(); NORMAL immediately
    │                             after start(); callback not called again without a status change
    ├── heartbeat_payload.cpp   — makeHeartbeatPayload() fills nodeId, uptime, flags, rxCount,
    │                             and ESR-derived TEC/REC without callers touching HAL registers
    └── health_payload.cpp      — makeNodeHealthPayload() packs nodeId/dieTempC, zeroes flags +
                                  fault + reserved; on-target readDieTempC()/readVddMv() range check (#213)
```

All runtime scenarios use `startLoopback()` — single board, no second node or physical bus
needed. `protocol_layout.cpp` is compile-time/static layout validation only.

`TX_ERROR` and `BUS_OFF` status transitions require real bus faults and cannot be tested in
loopback. Those are covered by the existing breadboard test: `Firmware/Prototype/` (Experiment B,
2026-05-20 — 21-min soak plus fault injection with wire pulls and transceiver power cuts).

`tx_queue.cpp` tests payload preservation in loopback; CTRL_BCAST coalescing and TX ring overflow
are **SKIP** in loopback — see **Dual-Board Integration Tests** below.

**`platformio.ini`:**

```ini
[env_base]
platform = ststm32
board = genericSTM32F103C8
framework = arduino
build_flags = -DNODE_ID=1 -DHAL_CAN_MODULE_ENABLED -DUSB_NONE -DHSE_VALUE=8000000
lib_extra_dirs = ${PROJECT_DIR}/../../Libraries
lib_deps =
    STM32Board
    CANProtocol
upload_protocol = stlink
upload_flags =
    -c
    set CPUTAPID 0
monitor_speed = 115200

[env:test_loopback_filter]
extends = env_base
build_src_filter = -<*> +<loopback_filter/loopback_filter.cpp>

[env:test_protocol_layout]
extends = env_base
build_src_filter = -<*> +<protocol_layout/protocol_layout.cpp>

[env:test_rx_dispatch]
extends = env_base
build_src_filter = -<*> +<rx_dispatch/rx_dispatch.cpp>

[env:test_tx_queue]
extends = env_base
build_src_filter = -<*> +<tx_queue/tx_queue.cpp>

[env:test_status_callbacks]
extends = env_base
build_src_filter = -<*> +<status_callbacks/status_callbacks.cpp>

[env:test_tx_batching]
extends = env_base
build_src_filter = -<*> +<tx_batching/tx_batching.cpp>

[env:test_heartbeat_payload]
extends = env_base
build_src_filter = -<*> +<heartbeat_payload/heartbeat_payload.cpp>

; ── Dual-board integration tests — require physical CAN bus ───────────────────
; Sender: NODE_ID=0  Receiver/ACK: NODE_ID=1
; Flash sender env to one board, receiver env to the other, then open both serial monitors.

[env:test_dual_coalesce_sender]
extends = env_base
build_flags = -DNODE_ID=0 -DHAL_CAN_MODULE_ENABLED -DUSB_NONE -DHSE_VALUE=8000000
build_src_filter = -<*> +<dual_coalesce/dual_coalesce.cpp>

[env:test_dual_coalesce_receiver]
extends = env_base
build_flags = -DNODE_ID=1 -DHAL_CAN_MODULE_ENABLED -DUSB_NONE -DHSE_VALUE=8000000
build_src_filter = -<*> +<dual_coalesce/dual_coalesce.cpp>

[env:test_dual_overflow_sender]
extends = env_base
build_flags = -DNODE_ID=0 -DHAL_CAN_MODULE_ENABLED -DUSB_NONE -DHSE_VALUE=8000000
build_src_filter = -<*> +<dual_overflow/dual_overflow.cpp>

[env:test_dual_overflow_ack]
extends = env_base
build_flags = -DNODE_ID=1 -DHAL_CAN_MODULE_ENABLED -DUSB_NONE -DHSE_VALUE=8000000
build_src_filter = -<*> +<dual_overflow/dual_overflow.cpp>
```

#### Dual-Board Integration Tests

Require two STM32F103CBT6 boards, two SN65HVD230 CAN transceivers, and a physical CAN bus with 120 Ω
termination at each end. Role is determined at compile time by `NODE_ID`.

**Startup order:** neither test requires boards to be powered simultaneously. The sender
waits 2 s after `start()` before transmitting — giving the receiver board time to boot.
If the receiver boots late it catches the trailing frames; results are still valid.

```
Firmware/Tests/CANProtocol/tests/
├── dual_coalesce/
│   └── dual_coalesce.cpp   — CTRL_BCAST coalescing on real bus
│                             NODE_ID=0 (sender): queues 20 CTRL_BCAST frames with values 1–20
│                               as fast as possible; waits 2 s after start(), then transmits burst.
│                             NODE_ID=1 (receiver): counts received CTRL_BCAST frames and records
│                               last payload byte. Reports at t=4 s.
│                             PASS: rxCount < 20 (coalescing occurred) AND lastValue == 20.
│                             txDropCount() on sender should be 0 (coalescing, not overflow).
│
└── dual_overflow/
    └── dual_overflow.cpp   — TX ring overflow drop policy
                              NODE_ID=0 (tester): queues 30 canIdEvt(1) frames in a tight loop
                                immediately after start() (no delay — fills mailboxes + ring);
                                reports txDropCount() at t=2 s.
                              NODE_ID=1 (ACK node): receives and ACKs all frames; reports count at t=4 s.
                              PASS: txDropCount() > 0 on sender (ring + mailboxes = 19 capacity →
                                at least 11 frames dropped); ACK node count + sender drops == 30.
```

**Expected dual_coalesce output (sender DiagSerial):**
```
[SENDER] burst sent
[SENDER] drops=0   PASS (coalescing, not overflow)
```

**Expected dual_coalesce output (receiver DiagSerial):**
```
[RECV] rx_count=N  lastVal=20
[RECV] Coalesced: PASS   (N < 20)
[RECV] Correct val: PASS
```

**Expected dual_overflow output (sender DiagSerial):**
```
[TESTER] drops=D   PASS  (D > 0)
```

---

## Public API

```cpp
// CANProtocol.h

// ── Types ─────────────────────────────────────────────────────────────────────

/** @brief Primary input/output routing packet. 4 bytes; two are batched for CTRL_BCAST/EVT_n. */
struct __attribute__((packed)) ControlPacket {
    uint16_t controlId;  ///< Payload routing key; not a CAN arbitration ID
    uint16_t value;      ///< Payload — interpretation depends on controlId range
};

/**
 * @brief Two ControlPackets packed into one 8-byte input/output CAN frame.
 *
 * Used by CTRL_BCAST and EVT_n only. Slot B controlId == 0x0000 signals an empty/padding slot.
 */
struct __attribute__((packed)) ControlPacketPair {
    ControlPacket a;
    ControlPacket b;
};

/**
 * @brief 8-byte payload carried by HB_n heartbeat frames.
 *
 * Sent every 500 ms by PanelGroup nodes. PanelBridge reads HB_1–HB_63 to track
 * PanelGroup health and populate diagnostics. HB_0 is reserved but not transmitted.
 */
struct __attribute__((packed)) HeartbeatPayload {
    uint8_t  nodeId;   ///< Node ID — redundant with CAN ID, aids logging
    uint8_t  flags;    ///< bit0=BOFF, bit1=EPVF (Error Passive; TEC ≥ 128)
    uint16_t uptime;   ///< Seconds since boot, little-endian (wraps at ~18 h)
    uint16_t rxCount;  ///< Node-owned accepted RX count, little-endian
    uint16_t esr;      ///< (CAN1->ESR >> 16): low byte=TEC, high byte=REC
};

/** @brief CAN bus health states. Reported to STM32Board via onStatusChange(). */
enum class CanStatus {
    STARTING,   ///< Peripheral configured, not yet started
    NORMAL,     ///< Bus active, no errors
    TX_ERROR,   ///< TEC > 0 — transmit errors accumulating
    BUS_OFF,    ///< CAN controller halted — bus-off condition
};

// ── Fixed CAN arbitration IDs (PanelBridge → All) ────────────────────────────

static constexpr uint32_t CAN_ID_CTRL_BCAST = 0x010;  ///< Broadcast ControlPacketPair to all panels
static constexpr uint32_t CAN_ID_TEST_SEQ   = 0x011;  ///< RTT throughput test
static constexpr uint32_t CAN_ID_SYNC_REQ   = 0x012;  ///< Request all nodes to re-poll inputs

// ── CAN ID functions (per-node IDs computed from NODE_ID) ─────────────────────

/** @brief Heartbeat frame ID for node n. Range 0x100–0x13F; n=0 is PanelBridge. */
constexpr uint32_t canIdHb(uint8_t n)    { return 0x100 + n; }

/** @brief Input event frame ID for node n. Range 0x201–0x23F. */
constexpr uint32_t canIdEvt(uint8_t n)   { return 0x200 + n; }

/** @brief TEST_SEQ echo frame ID for node n. Range 0x301–0x33F. */
constexpr uint32_t canIdEcho(uint8_t n)  { return 0x300 + n; }

/** @brief Boot-complete READY frame ID for node n. Range 0x401–0x43F. */
constexpr uint32_t canIdReady(uint8_t n) { return 0x400 + n; }

// ── controlId namespace ───────────────────────────────────────────────────────
// Payload controlIds live inside ControlPacket data bytes. They are intentionally
// separate from the 11-bit CAN arbitration IDs above.

static constexpr uint16_t CTRL_ID_HID_MIN  = 0x0010;  ///< HID range start (axes + buttons)
static constexpr uint16_t CTRL_ID_HID_MAX  = 0x00FF;  ///< HID range end
static constexpr uint16_t CTRL_ID_DCS_MIN  = 0x8000;  ///< DCS-BIOS range start
static constexpr uint16_t CTRL_ID_DCS_MAX  = 0x86FF;  ///< DCS-BIOS range end

// ── Node-status host contract + fault dictionary (#86 / #163) ─────────────────
// How PanelBridge reports connected nodes + health to the host over DCS-BIOS. Lives here
// (CAN-membership layer, next to NodeHealthPayload / canIdHealth), NOT in HIDControls.h.
// **Canonical contract source the client's sync-a4ec.ts parses** — bump the proto version
// on any _NODE_STATUS wire change. Full field decode: FirmwarePlan/04-dcs-bios-integration.md.
#define NODE_STATUS_PROTO_VERSION 2
#define NODE_STATUS_REQ_ADDR      0x86FE
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"

// HEALTH_n faultId dictionary (#163) — coarse, one active at a time; exact device logged on
// DiagSerial, not the wire. Client maps id → label (SkyHawkClient#40). Append to grow.
enum class NodeFaultId : uint8_t { NONE = 0x00, I2C_PERIPHERAL = 0x01 /* 0x02+ reserved */ };

// ── HID axis/button controlIds — from HIDControls.h (included above) ────────
// CANProtocol.h includes <HIDControls.h>; CTRL_* constants are available to any
// STM32 sketch that includes <CANProtocol.h> without a separate include.
//
// This list is NOT exhaustive — new CTRL_* constants are added to HIDControls.h
// as sub-nodes are catalogued. Full reserved range: 0x0010–0x00FF.
// HIDControls.h is the authoritative list; entries below reflect the current
// known set and may not be current — check the header file.

static constexpr uint16_t CTRL_ROLL     = 0x0010;  ///< Roll axis   — stick sub-node (AS5600 / pot)
static constexpr uint16_t CTRL_PITCH    = 0x0011;  ///< Pitch axis  — stick sub-node (AS5600 / pot)
static constexpr uint16_t CTRL_THROTTLE = 0x0012;  ///< Throttle    — throttle sub-node (ADC)
static constexpr uint16_t CTRL_RUDDER   = 0x0013;  ///< Rudder axis — pedal sub-node (ADC)
static constexpr uint16_t CTRL_BRAKE_L  = 0x0014;  ///< Left brake  — pedal sub-node (ADC)
static constexpr uint16_t CTRL_BRAKE_R  = 0x0015;  ///< Right brake — pedal sub-node (ADC)
static constexpr uint16_t CTRL_ZOOM     = 0x0016;  ///< Zoom axis   — throttle sub-node (ADC)
// ... additional CTRL_* axes and buttons added here as panels are catalogued

// ── Callback types ────────────────────────────────────────────────────────────

/** @brief Fired when CAN bus status changes. Register via onStatusChange(). */
using CanStatusCallback  = void(*)(CanStatus status);

/** @brief Fired when a CAN frame is received. Register via onReceive(). */
using CanRxCallback      = void(*)(uint32_t canId, const uint8_t* data, uint8_t len);

/** @brief Fired when SYNC_REQ is received. Register via onSyncReq(). */
using CanSyncReqCallback = void(*)();

// ── Namespace ─────────────────────────────────────────────────────────────────

namespace CANProtocol {

    // ── Filter configuration (call before start()) ────────────────────────────

    /**
     * @brief Accept all incoming CAN frames. Use for PanelBridge.
     *
     * Configures a pass-all hardware mask filter. Software-side range validation
     * is the caller's responsibility (see PanelBridge TechSpec).
     * Mandatory IDs (CTRL_BCAST, TEST_SEQ, SYNC_REQ) are redundantly included
     * but have no effect with pass-all active.
     */
    void filterAcceptAll();

    /**
     * @brief Accept a specific CAN ID. Use for PanelGroup nodes.
     *
     * Adds one ID to the hardware filter list. Call multiple times for multiple IDs.
     * CTRL_BCAST (0x010), TEST_SEQ (0x011), and SYNC_REQ (0x012) are always
     * included automatically by start() — do not add them manually.
     *
     * @param canId 11-bit standard CAN ID to accept.
     */
    void filterAcceptId(uint32_t canId);

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Start the CAN peripheral and apply registered filters.
     *
     * Must be called after STM32Board::begin() and after all filter and callback
     * registrations. Always adds CTRL_BCAST (0x010), TEST_SEQ (0x011), and
     * SYNC_REQ (0x012) to the active filter — these are mandatory for all nodes
     * and cannot be excluded.
     *
     * Sets CanStatus to NORMAL on success and fires the onStatusChange callback.
     */
    void start();

    /**
     * @brief Start in silent loopback mode — for bench testing only.
     *
     * Identical to start() but uses CAN_MODE_SILENT_LOOPBACK: frames transmitted
     * by this node are received back internally without going on the physical bus.
     * Enables single-board testing of filter dispatch, RX callbacks, and TX queue
     * behaviour without a second node.
     *
     * @note Never call this in production firmware.
     */
    void startLoopback();

    /**
     * @brief Process RX callbacks and batched-ControlPacket deadlines.
     *
     * Call once per loop() iteration. Drains all frames received since the last
     * call. For each frame: fires onSyncReq() for SYNC_REQ, auto-replies with
     * ECHO carrying the same 8-byte payload for TEST_SEQ, fires onReceive() for
     * all other frames.
     *
     * Also services sendBatched() deadlines: any half-full ControlPacketPair
     * that has waited two owning firmware loop iterations is flushed with
     * slot B set to the null sentinel.
     */
    void drain();

    // ── Transmit ──────────────────────────────────────────────────────────────

    /**
     * @brief Send a CAN frame.
     *
     * If a TX mailbox is available, sends immediately. If all three mailboxes are
     * occupied, the frame is placed in the TX software ring buffer (~16 entries).
     * The buffer is drained automatically when mailboxes free up via TX-complete
     * interrupt. Overflow is frame-type dependent: CTRL_BCAST may drop/coalesce
     * stale state; EVT/control frames retry up to 3 send attempts before drop;
     * heartbeat frames may drop stale heartbeat state. CTRL_BCAST and EVT_n payloads are
     * ControlPacketPair batches; a single input/output packet uses slot B controlId == 0x0000
     * as the null sentinel. Special/control/diagnostic frames use their explicit DLC and
     * payload. All drops increment the DIAG_ERR TX drop counter.
     *
     * @param canId  11-bit standard CAN ID.
     * @param data   Pointer to payload bytes.
     * @param len    Payload length in bytes (0–8).
     */
    void send(uint32_t canId, const uint8_t* data, uint8_t len);

    /**
     * @brief Submit one ControlPacket to a CANProtocol-owned ControlPacketPair batch.
     *
     * Valid only for CAN_ID_CTRL_BCAST and canIdEvt(n). The first packet is held as
     * slot A briefly while CANProtocol waits for a second packet. When slot B arrives,
     * CANProtocol sends one 8-byte ControlPacketPair. If no slot B arrives by the
     * batching deadline serviced from drain(), CANProtocol sends slot A with slot B
     * controlId == 0x0000.
     *
     * PanelBridge uses this for DCS output broadcasts. PanelGroup uses this for EVT_n
     * input events. Neither higher-level library builds ControlPacketPair payloads or
     * manages CAN TX queue state directly.
     *
     * @param canId  CAN_ID_CTRL_BCAST or canIdEvt(NODE_ID).
     * @param pkt    ControlPacket to batch.
     */
    void sendBatched(uint32_t canId, const ControlPacket& pkt);

    /**
     * @brief Force a half-full ControlPacketPair batch to send immediately.
     *
     * If the named batched CAN ID has a pending slot A, sends slot A with slot B set to
     * the null sentinel. If no packet is pending, this is a no-op. Used at the end of
     * boot and SYNC_REQ input snapshot passes so odd trailing input states are not held.
     *
     * @param canId CAN_ID_CTRL_BCAST or canIdEvt(NODE_ID).
     */
    void flushBatched(uint32_t canId);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    /**
     * @brief Register a CAN bus status change callback.
     *
     * Fired when status transitions between STARTING, NORMAL, TX_ERROR, BUS_OFF.
     * Intended for STM32Board LED wiring — see STM32Board TechSpec.
     *
     * @param cb Callback to invoke with the new CanStatus.
     */
    void onStatusChange(CanStatusCallback cb);

    /**
     * @brief Register a SYNC_REQ handler.
     *
     * Fired by drain() when a SYNC_REQ frame (0x012) is received. PanelGroup
     * registers this to trigger a re-poll of all registered input objects.
     *
     * @param cb Callback to invoke on SYNC_REQ receipt.
     */
    void onSyncReq(CanSyncReqCallback cb);

    /**
     * @brief Register a general-purpose RX frame handler.
     *
     * Fired by drain() for all received frames except SYNC_REQ (handled by
     * onSyncReq) and TEST_SEQ (handled automatically by CANProtocol — ECHO sent
     * with the same 8-byte payload).
     * The caller receives the raw CAN ID and payload bytes and interprets them.
     *
     * @param cb Callback invoked with canId, data pointer, and payload length.
     * @note Only one onReceive handler per node. PanelGroup and PanelBridge each
     *       register their own handler in their setup().
     */
    void onReceive(CanRxCallback cb);

    // ── Diagnostics ───────────────────────────────────────────────────────────

    /**
     * @brief Return the CAN Transmit Error Counter.
     * @returns TEC value (0–255) from the ESR register.
     */
    uint8_t tec();

    /**
     * @brief Return the CAN Receive Error Counter.
     * @returns REC value (0–255) from the ESR register.
     */
    uint8_t rec();

    /**
     * @brief Return whether the CAN controller is in bus-off state.
     * @returns true if ESR BOFF bit is set.
     */
    bool busOff();

    /**
     * @brief Build the standard 8-byte heartbeat payload for the current node.
     *
     * Fills uptime, CAN health flags, and ESR-derived TEC/REC fields from CANProtocol-owned
     * state. Callers provide the logical node ID and their own receive counter.
     *
     * @param nodeId  Node ID to place in the payload; 1-63 are PanelGroups, 0 is reserved.
     * @param rxCount Caller-owned receive counter to place in the payload.
     * @return        Fully populated HeartbeatPayload ready to send as HB_n.
     */
    HeartbeatPayload makeHeartbeatPayload(uint8_t nodeId, uint16_t rxCount);

    /**
     * @brief Build the 8-byte node-health payload (internal die temp) for HEALTH_n (#213/#221).
     *
     * Packs the caller-supplied die temperature (via STM32Board::readDieTempC()). Sets the
     * overheat flag (bit0) only when NODE_OVERHEAT_C is defined at build time and dieTempC meets
     * it; otherwise flags is 0 (pure telemetry). Fault + reserved bytes zeroed (owned by #163).
     *
     * @param nodeId   Node ID to place in the payload; 1-63 PanelGroup, 0 PanelBridge.
     * @param dieTempC Internal die temp in whole °C (INT8_MIN = unavailable).
     * @return         Fully populated NodeHealthPayload ready to send as HEALTH_n.
     */
    NodeHealthPayload makeNodeHealthPayload(uint8_t nodeId, int8_t dieTempC);

    /**
     * @brief Return the cumulative TX queue drop count since last reset.
     * @returns Number of frames dropped due to TX buffer overflow.
     */
    uint32_t txDropCount();

} // namespace CANProtocol
```

---

## Key Data Structures

### ControlPacket Batch State

CANProtocol owns the private batch builders for input/output routing frames. Higher layers
submit individual packets through `sendBatched()`; they never allocate or queue
`ControlPacketPair` payloads themselves.

Each active batched CAN ID stores:

```cpp
struct PacketBatchState {
    bool hasA;
    ControlPacket a;
    uint8_t loopsWaited;
};
```

Supported batched IDs are `CAN_ID_CTRL_BCAST` and `canIdEvt(n)`. When a second packet arrives,
CANProtocol sends `{a, b}` as one 8-byte frame. If no second packet arrives by the deadline
serviced from `drain()`, or if the caller explicitly calls `flushBatched(canId)`,
CANProtocol sends `{a, null}` where `null.controlId == 0x0000`.

### TX Ring Buffer

Software queue used when all three STM32 CAN TX mailboxes are occupied. Drained
automatically via TX-complete interrupt — no explicit drain call needed from the caller.

Each queued entry stores:

```cpp
struct TxQueueEntry {
    uint32_t canId;
    uint8_t  len;
    uint8_t  data[8];
    uint8_t  attempts;  // bounded-retry classes only; max 3
};
```

```
Capacity:        ~16 frames
Overflow policy: frame-type dependent
Drop counter:    txDropCount() — incremented on each drop, visible via DIAG_ERR
```

For bounded-retry frames, an attempt means a handoff attempt to the HAL TX mailbox layer.
Simple queue insertion because all mailboxes are currently occupied does not count as an
attempt. When `attempts == 3` and the frame still cannot be handed to HAL, drop it and
increment `txDropCount()`.

`CTRL_BCAST` carries DCS output state such as gauge positions, LEDs, and dimmers. The payload
is a `ControlPacketPair`. For these state snapshots, newest value is preferred over stale
intermediate values; queued frames may be dropped or coalesced by `controlId` where practical.

`EVT_n` carries input events as a `ControlPacketPair`. `EVT_n`, `READY_n`, `SYNC_REQ`,
`TEST_SEQ`, and `ECHO_n` are event/control frames. They use bounded retry: keep the frame
for up to 3 send attempts, then drop it and increment the `DIAG_ERR` TX drop counter. This
avoids losing user actions during transient mailbox contention without allowing an unhealthy
bus to create an unbounded backlog.

`HB_n` heartbeat frames may drop stale queued heartbeat state; the next heartbeat replaces it.

### RX Queue

Frames received in the CAN RX interrupt are placed in a small ring buffer. `drain()`
processes this buffer in loop() context, outside the ISR. Keeps ISR execution time minimal.

The HAL is configured with `ReceiveFifoLocked = DISABLE`: if the hardware FIFO fills before
the ISR drains it, the oldest frame is overwritten by the newest. This should be rare because
the RX ISR immediately moves frames into the software RX queue. Unlike DCS output broadcasts,
EVT frames are discrete user actions, so RX overflow is treated as a diagnostic problem rather
than a preferred freshness policy.

---

## Implementation Notes

### HAL configuration — critical settings

Two HAL settings must be applied during peripheral init or the node will fail under any bus
fault. Both validated in Experiment B (2026-05-20 soak test and fault injection).

**AutoRetransmission: DISABLE**
With auto-retransmit enabled, a single unACKed frame (e.g. during a momentary bus fault)
fills all three TX mailboxes and the node enters bus-off immediately — the TX queue never
gets a chance to drain. With `DISABLE`, TEC rises +8 per failed attempt; the node enters
Error Passive at TEC=128 and self-heals when the fault clears.

**AutoBusOff: ENABLE**
Hardware auto-recovery from bus-off takes ~3 ms at 500 kbps (1408 recessive bits). With
`ENABLE`, the CAN controller recovers automatically — no firmware intervention required.
With `DISABLE`, firmware would have to manually restart the controller after bus-off.

These are set once in `STM32Board::begin()` during peripheral configuration and must not be
overridden.

---

### Mandatory filter IDs

`start()` unconditionally adds CTRL_BCAST (0x010), TEST_SEQ (0x011), and SYNC_REQ (0x012)
to the active filter regardless of what the caller registered. These are global — every
node must receive them. If `filterAcceptAll()` was called, the hardware is pass-all and
these additions are redundant but harmless.

### ControlPacketPair payload

`CTRL_BCAST` and `EVT_n` frames carry exactly 8 bytes: a raw `ControlPacketPair`.
Slot B is ignored when `b.controlId == 0x0000`. Pair decoding is not global; special frames
use their own explicit DLC/payload and may use a single 4-byte `ControlPacket` only when that
frame type says so.

The protocol allows CANProtocol to wait briefly for slot B, but not indefinitely. CANProtocol
owns the batch state for `CTRL_BCAST` and `EVT_n`, and `drain()` services the two-loop
deadline. PanelBridge and PanelGroup call `sendBatched()` with individual `ControlPacket`s.
PanelGroup snapshot bursts call `flushBatched(canIdEvt(NODE_ID))` at the end of the poll pass
so an odd trailing packet is sent immediately.

The existing prototype `PanelGroup.cpp` sends 8 bytes (ControlPacket + 4-byte `millis()`
timestamp). The timestamp was added for prototype RTT measurement and is **not** part of the
production protocol. Production firmware uses bytes 4–7 as slot B of the `ControlPacketPair`.

---

### TEST_SEQ auto-reply

When `drain()` encounters a TEST_SEQ frame, CANProtocol sends the ECHO reply automatically:
```cpp
CANProtocol::send(canIdEcho(NODE_ID), data, len);
```
The TEST_SEQ payload must be exactly 8 bytes and is echoed unchanged. No user code required.
The TEST_SEQ frame is not forwarded to the `onReceive` callback.

### SYNC_REQ handling

SYNC_REQ is intercepted by `drain()` and fires `onSyncReq()` callback. Not forwarded
to `onReceive`. PanelGroup registers `onSyncReq` to trigger input re-poll.

### CAN status monitoring

A periodic check (inside `drain()` or via TX-complete / error interrupts) reads
`CAN1->ESR` and updates CanStatus:
- TEC > 0 and not bus-off → `TX_ERROR`
- BOFF bit set → `BUS_OFF`
- Otherwise → `NORMAL`

On any status change, the registered `onStatusChange` callback fires. STM32Board wires
this to the LED state machine.

### TX queue draining

Automatic — no explicit call needed from `loop()`. When a TX mailbox completes
(HAL TX-complete callback), CANProtocol checks the ring buffer and sends the next queued
frame. This keeps the bus saturated without stalling the main loop.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| STM32duino Arduino core | PlatformIO `framework = arduino` | HAL CAN, interrupt callbacks |
| STM32Board | `Firmware/Libraries/STM32Board` | `begin()` must be called first to configure the peripheral |
| HIDControls | `Firmware/Libraries/HIDControls` | Platform-agnostic library; `CANProtocol.h` includes it so CTRL_* constants are available transitively |
| `NODE_ID` define | `platformio.ini` `build_flags` | Used in `canIdHb(NODE_ID)` etc. |
