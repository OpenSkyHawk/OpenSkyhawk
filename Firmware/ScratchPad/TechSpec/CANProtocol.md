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
    │                             sizeof(ControlPacketPair)==8, sizeof(HeartbeatPayload)==8,
    │                             sizeof(ReadyPayload)==8 with the version in bytes 0-2 (#295);
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
    ├── tx_queue.cpp            — queued frames preserve full payload bytes (loopback); burst order
    │                             and ring overflow need a real bus (dual_order, dual_overflow)
    ├── status_callbacks.cpp    — onStatusChange fires STARTING before start(); NORMAL immediately
    │                             after start(); callback not called again without a status change
    ├── heartbeat_payload.cpp   — makeHeartbeatPayload() fills nodeId, uptime, flags, rxCount,
    │                             and ESR-derived TEC/REC without callers touching HAL registers
    ├── health_payload.cpp      — makeNodeHealthPayload() packs nodeId/dieTempC + faultId/DEGRADED +
    │                             nodeKind in byte 5 (#344), faultMask/reserved 6-7 stay 0; on-target
    │                             readDieTempC()/readVddMv() range (#213)
    ├── health_flags.cpp        — overheat+degraded coexistence: -DNODE_OVERHEAT_C=0, asserts a faulted
    │                             hot payload sets BOTH flag bits (|= never clobbers overheat) (#163)
    ├── node_status_service.cpp — serviceNodeStatus() (#340), loopback: start() auto-accepts HB_0 and
    │                             drain() still forwards it; HB_n every 500 ms (first ~500 ms after
    │                             start); HEALTH_n every 1000 ms; master-loss WARNING absent before a
    │                             master is seen, raised after HB_0 stops > 1500 ms, cleared on return
    ├── duplicate_node_id.cpp   — duplicate NODE_ID check (#344), loopback: own looped-back HBs never
    │                             trip it; extra HB_n frames the test sends raise DUPLICATE_NODE_ID +
    │                             DEGRADED in lastNodeHealth(), held while they flow, clear ~3 s after
    │                             they stop
    └── node_status_bus.cpp     — the same service on a physical bus against a master (PanelBridge or
                                  a USB-CAN adapter sending HB_0 every 500 ms): DiagSerial logs every
                                  LED state change + a 10 s summary (warnRaises / hb0Seen / TEC / REC /
                                  fault / kind) for watchdog timing, flapping-master and link-drop runs,
                                  and soaks; flashed on two boards with the same NODE_ID, it is also
                                  the duplicate-NODE_ID bench (fault=9, then fault=0 once one board is
                                  unplugged)
```

All runtime scenarios use `startLoopback()` — single board, no second node or physical bus
needed. `protocol_layout.cpp` is compile-time/static layout validation only.

`TX_ERROR` and `BUS_OFF` status transitions require real bus faults and cannot be tested in
loopback. Those are covered by the existing breadboard test: `Firmware/Prototype/` (Experiment B,
2026-05-20 — 21-min soak plus fault injection with wire pulls and transceiver power cuts).

`tx_queue.cpp` tests payload preservation in loopback; CTRL_BCAST burst order and TX ring overflow
are **SKIP** in loopback — see **Dual-Board Integration Tests** below.

**`platformio.ini`:**

```ini
[env_base]
platform = ststm32@^20.0.0
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

[env:test_dual_order_sender]
extends = env_base
build_flags = -DNODE_ID=0 -DHAL_CAN_MODULE_ENABLED -DUSB_NONE -DHSE_VALUE=8000000
build_src_filter = -<*> +<dual_order/dual_order.cpp>

[env:test_dual_order_receiver]
extends = env_base
build_flags = -DNODE_ID=1 -DHAL_CAN_MODULE_ENABLED -DUSB_NONE -DHSE_VALUE=8000000
build_src_filter = -<*> +<dual_order/dual_order.cpp>

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

Require two STM32F103C8 (or CB) boards, two SN65HVD230 CAN transceivers, and a physical CAN bus with 120 Ω
termination at each end. Role is determined at compile time by `NODE_ID`.

**Startup order:** neither test requires boards to be powered simultaneously. The sender
waits 2 s after `start()` before transmitting — giving the receiver board time to boot.
If the receiver boots late it catches the trailing frames; results are still valid.

```
Firmware/Tests/CANProtocol/tests/
├── dual_order/
│   └── dual_order.cpp      — CTRL_BCAST burst arrives whole and in order on a real bus (#389)
│                             NODE_ID=0 (sender): queues 20 CTRL_BCAST frames with values 1–20
│                               as fast as possible; waits 2 s after start(), then transmits burst.
│                             NODE_ID=1 (receiver): counts received CTRL_BCAST frames and checks
│                               each value is the previous + 1. Reports at t=5 s.
│                             PASS: rxCount == 20, in order, lastValue == 20; sender drops == 0.
│
└── dual_overflow/
    └── dual_overflow.cpp   — TX ring overflow drop policy
                              NODE_ID=0 (tester): queues 100 canIdHb(0) frames with IRQs off;
                                reports txDropCount() at t=3 s.
                              NODE_ID=1 (ACK node): receives and ACKs all frames; reports count at t=4 s.
                              PASS: txDropCount() > 0 on sender (one mailbox for the ID + 63 ring
                                slots = 64 → 36 dropped); ACK node count + sender drops == 100.
```

**Expected dual_order output (sender DiagSerial):**
```
[SENDER] 20-frame burst queued
[SENDER] drops=0  PASS
```

**Expected dual_order output (receiver DiagSerial):**
```
[RECV] rx_count=20  lastVal=20
[RECV] All 20:   PASS
[RECV] In order: PASS
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
 * Sent every 500 ms by every node through serviceNodeStatus() (#340). PanelBridge reads
 * HB_1–HB_63 to track node health and populate diagnostics; its own HB_0 is the master
 * liveness beacon every other node watches (#93).
 */
struct __attribute__((packed)) HeartbeatPayload {
    uint8_t  nodeId;   ///< Node ID — redundant with CAN ID, aids logging
    uint8_t  flags;    ///< HeartbeatFlag bits (NodeStatus.h): BOFF, EPVF
    uint16_t uptime;   ///< Seconds since boot, little-endian (wraps at ~18 h)
    uint16_t rxCount;  ///< Node-owned accepted RX count, little-endian
    uint16_t esr;      ///< TEC at HB_ESR_TEC_SHIFT, REC at HB_ESR_REC_SHIFT (NodeStatus.h)
};

/**
 * @brief 8-byte payload carried by TELEM_n frames — paged node telemetry (#216).
 *
 * Byte 0 is a TelemPage (NodeStatus.h) naming the layout of bytes 1–7; page ids are global and
 * append-only. Multi-byte page fields are little-endian. Layouts: FirmwarePlan/02-can-protocol.md
 * (Node Telemetry Payload).
 */
struct __attribute__((packed)) TelemPayload {
    uint8_t page;     ///< 0: TelemPage; 0x00 invalid
    uint8_t body[7];  ///< 1-7: page-defined
};
static_assert(sizeof(TelemPayload) == 8, "TelemPayload must fill one classic CAN frame");

/**
 * @brief 8-byte payload carried by READY_n — the node's boot announcement (#295).
 *
 * The firmware release version (FirmwareVersion.h). Sent once per boot by sendReady(); older
 * firmware sends READY_n empty (DLC 0), which PanelBridge reports as not reported (0.0.0).
 */
struct __attribute__((packed)) ReadyPayload {
    uint8_t fwMajor;  ///< 0: OPENSKYHAWK_FW_MAJOR
    uint8_t fwMinor;  ///< 1
    uint8_t fwPatch;     ///< 2
    uint8_t bootloader;  ///< 3: STM32Board::bootloaderVersion() — 0 = none (#365)
    uint8_t flags;       ///< 4: ReadyFlag bits (#383) — 0 = the boot READY
    uint8_t rsvd[3];     ///< 5-7: reserved, transmit 0
};
static_assert(sizeof(ReadyPayload) == 8, "ReadyPayload must fill one classic CAN frame");

enum class ReadyFlag : uint8_t {
    RESYNC = 0x01,  // an answer to SYNC_REQ: the bridge caches the version, no SYNC_REQ back (#383)
};

/** @brief CAN bus health states. Reported to STM32Board via onStatusChange(). */
enum class CanStatus {
    STARTING,   ///< Peripheral configured, not yet started
    NORMAL,     ///< Bus active, no errors
    TX_ERROR,   ///< TEC > 0 — transmit errors accumulating
    BUS_OFF,    ///< CAN controller halted — bus-off condition
};

// ── Fixed CAN arbitration IDs (PanelBridge → All) ────────────────────────────

static constexpr uint32_t CAN_ID_CTRL_BCAST = 0x7E0;  ///< Broadcast ControlPacketPair to all panels — the lowest priority on the bus (#389, D31)
static constexpr uint32_t CAN_ID_TEST_SEQ   = 0x011;  ///< RTT throughput test
static constexpr uint32_t CAN_ID_SYNC_REQ   = 0x012;  ///< Ask a node to re-send its state (byte 0 = its NODE_ID, #387)
static constexpr uint8_t  SYNC_REQ_LEN_TARGETED = 1;  ///< DLC of a targeted SYNC_REQ; an empty one (DLC 0) is for every node

// ── CAN ID functions (per-node IDs computed from NODE_ID) ─────────────────────

/** @brief Heartbeat frame ID for node n. Range 0x100–0x13F; n=0 is PanelBridge. */
constexpr uint32_t canIdHb(uint8_t n)    { return 0x100 + n; }

/** @brief Node-health/thermal telemetry frame ID for node n. Range 0x140–0x17F; n=0 is PanelBridge. */
constexpr uint32_t canIdHealth(uint8_t n) { return 0x140 + n; }

/** @brief Paged node-telemetry frame ID for node n (TelemPayload, #216). Range 0x181–0x1BF.
 *         PanelBridge forwards it to the host unread as `_NODE_TELEM`. */
constexpr uint32_t canIdTelem(uint8_t n)  { return 0x180 + n; }

/** @brief Input event frame ID for node n. Range 0x201–0x23F. */
constexpr uint32_t canIdEvt(uint8_t n)   { return 0x200 + n; }

/** @brief TEST_SEQ echo frame ID for node n. Range 0x301–0x33F. */
constexpr uint32_t canIdEcho(uint8_t n)  { return 0x300 + n; }

/** @brief Boot-complete READY frame ID for node n. Range 0x401–0x43F. */
constexpr uint32_t canIdReady(uint8_t n) { return 0x400 + n; }

/** @brief Relative-input event frame ID for node n. Range 0x501–0x53F. */
constexpr uint32_t canIdEvtRel(uint8_t n) { return 0x500 + n; }

/** @brief Directional-input event frame ID for node n. Range 0x601–0x63F. */
constexpr uint32_t canIdEvtDir(uint8_t n) { return 0x600 + n; }

/** @brief Action-input event frame ID for node n. Range 0x701–0x73F. */
constexpr uint32_t canIdEvtAction(uint8_t n) { return 0x700 + n; }

// ── controlId namespace ───────────────────────────────────────────────────────
// Payload controlIds live inside ControlPacket data bytes. They are intentionally
// separate from the 11-bit CAN arbitration IDs above.

static constexpr uint16_t CTRL_ID_HID_MIN  = 0x0010;  ///< HID range start (axes + buttons)
static constexpr uint16_t CTRL_ID_HID_MAX  = 0x00FF;  ///< HID range end
static constexpr uint16_t CTRL_ID_DCS_MIN  = 0x8000;  ///< DCS-BIOS range start
static constexpr uint16_t CTRL_ID_DCS_MAX  = 0x86FF;  ///< DCS-BIOS range end

// Node-status host contract + fault vocabulary (NODE_STATUS_*, NodeHealthFlag, NodeFaultCode,
// NodeKind, FaultSource) live in the neutral `NodeStatus` library (see NodeStatus.md), not here.
// CANProtocol owns only NodeHealthPayload (the CAN HEALTH_n frame struct).

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
     * CTRL_BCAST (0x7E0), TEST_SEQ (0x011), and SYNC_REQ (0x012) are always
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
     * registrations. Always adds CTRL_BCAST (0x7E0), TEST_SEQ (0x011), and
     * SYNC_REQ (0x012) to the active filter — these are mandatory for all nodes
     * and cannot be excluded. On every node except PanelBridge (NODE_ID 0) it also adds
     * HB_0 (0x100) for the serviceNodeStatus() master-loss watchdog (#340), and on every node
     * its own HB_NODE_ID for the duplicate NODE_ID check (#344).
     *
     * Resets the serviceNodeStatus() timers, which arm on its first call. Sets CanStatus to
     * NORMAL on success and fires the onStatusChange callback.
     */
    void start();

    /**
     * @brief Start in silent loopback mode — for bench testing only.
     *
     * Identical to start() but uses CAN_MODE_SILENT_LOOPBACK: frames transmitted
     * by this node are received back internally without going on the physical bus.
     * Enables single-board testing of filter dispatch, RX callbacks, and TX queue
     * behaviour without a second node. The duplicate NODE_ID check discounts one looped-back
     * own HB_n per heartbeat sent, so only extra own-ID frames read as a duplicate (#344).
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
     * all other frames. On a non-bridge node an HB_0 is first stamped for the
     * serviceNodeStatus() watchdog, then still passed to onReceive(). A frame on the node's own
     * HB_NODE_ID is first stamped for the duplicate NODE_ID check (#344), then passed on too.
     *
     * Also services sendBatched() deadlines: any half-full ControlPacketPair
     * that has waited two owning firmware loop iterations — and, on PanelBridge, at least
     * BATCH_HOLD_MS (2 ms, #389) — is flushed with slot B set to the null sentinel.
     */
    void drain();

    // ── Transmit ──────────────────────────────────────────────────────────────

    /**
     * @brief Send a CAN frame.
     *
     * Every frame goes through the TX ring (16 entries; 64 on the bridge) and leaves in the
     * order it was sent; the TX-complete interrupt keeps the mailboxes loaded. A frame that
     * loses arbitration is resent until it goes out; one that hits a bus error, up to 3
     * attempts in all (#389, D32). At most one frame per CAN ID is in the mailboxes at a time.
     * A full ring drops the new frame. CTRL_BCAST and EVT_n payloads are
     * ControlPacketPair batches; a single input/output packet uses slot B controlId == 0x0000
     * as the null sentinel. Special/control/diagnostic frames use their explicit DLC and
     * payload. Every drop is counted on the board in txDropCount() — not reported over the bus.
     *
     * @param canId  11-bit standard CAN ID.
     * @param data   Pointer to payload bytes.
     * @param len    Payload length in bytes (0–8).
     */
    void send(uint32_t canId, const uint8_t* data, uint8_t len);

    /**
     * @brief Submit one ControlPacket to a CANProtocol-owned ControlPacketPair batch.
     *
     * Valid only for CAN_ID_CTRL_BCAST, canIdEvt(n), canIdEvtRel(n), canIdEvtDir(n) and
     * canIdEvtAction(n) — each with its own pending slot. The first packet is held as
     * slot A briefly while CANProtocol waits for a second packet. When slot B arrives,
     * CANProtocol sends one 8-byte ControlPacketPair. If no slot B arrives by the
     * batching deadline serviced from drain(), CANProtocol sends slot A with slot B
     * controlId == 0x0000.
     *
     * A canId with no registered slot is silently discarded — see the batch-registration
     * note in the Batching section.
     *
     * PanelBridge uses this for DCS output broadcasts. PanelGroup uses this for EVT_n
     * input events and the REL / DIR / ACTION dispatch-form frames. Neither higher-level
     * library builds ControlPacketPair payloads or manages CAN TX queue state directly.
     *
     * @param canId  CAN_ID_CTRL_BCAST, canIdEvt(NODE_ID), canIdEvtRel(NODE_ID),
     *               canIdEvtDir(NODE_ID), or canIdEvtAction(NODE_ID).
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
     * Fired by drain() when a SYNC_REQ frame (0x012) for this node is received — byte 0 = its
     * NODE_ID, or an empty frame (every node); one naming another node is ignored (#387).
     * PanelGroup registers this to re-report every registered input. After the boot READY,
     * drain() then ends the answer with a READY flagged RESYNC (see sendReady()).
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
     * it; otherwise the overheat bit is 0 (pure telemetry). This is the packing boundary that owns
     * the NodeFaultCode->byte cast: it writes faultId and OR-derives the DEGRADED flag (bit1) when
     * fault != NONE (never clobbering the overheat bit), and writes nodeKind (byte 5, #344).
     * faultMask + reserved bytes 6-7 stay 0 (#163).
     *
     * @param nodeId   Node ID to place in the payload; 1-63 PanelGroup, GATEWAY_NODE_ID PanelBridge.
     * @param kind     Board kind (NodeKind) from the node's firmware role — never from nodeId.
     * @param dieTempC Internal die temp in whole °C (DIE_TEMP_UNAVAILABLE = unavailable).
     * @param fault    Primary node fault; NONE (default) = healthy node.
     * @return         Fully populated NodeHealthPayload ready to send as HEALTH_n.
     */
    NodeHealthPayload makeNodeHealthPayload(uint8_t nodeId, NodeKind kind, int8_t dieTempC,
                                            NodeFaultCode fault = NodeFaultCode::NONE);

    /**
     * @brief Return the cumulative TX queue drop count since last reset.
     * @returns Number of frames dropped due to TX buffer overflow.
     */
    uint32_t txDropCount();

    /** @brief Build the READY_n payload: this build's firmware version (FirmwareVersion.h), #295.
     *  @param flags ReadyFlag bits — 0 for the boot READY (#383). */
    ReadyPayload makeReadyPayload(uint8_t flags = 0);

    /** @brief SYNC_REQ for one node (#387): byte 0 = @p nodeId. PanelBridge's sync queue only. */
    void sendSyncReq(uint8_t nodeId);

    /** @brief Frames lost to a full software RX ring since startup (#387). Should stay 0. */
    uint32_t rxDropCount();

    /** @brief RX FIFO 0 hardware overruns since startup (#389): frames lost because all three
     *  slots were full. Needs interrupts held off for three frame times; should stay 0. */
    uint32_t rxOverrunCount();

    /**
     * @brief Send this node's boot READY_n with makeReadyPayload(). Called once, at the end of the
     *        node library's setup() (PanelGroup step 8, PduNode) — never by sketches. From then on
     *        drain() ends its answer to every SYNC_REQ for this node (byte 0 = NODE_ID, or DLC 0)
     *        with a READY flagged RESYNC: onSyncReq fires first, and the READY goes once the TX ring
     *        is empty and all mailboxes are free (50 ms cap), so it marks the end of the burst (#383, #387).
     */
    void sendReady();

    // ── Node status service (every node) ──────────────────────────────────────

    /**
     * @brief Run the node-generic status duties. Call once per loop(), after drain() (#340).
     *
     * - HB_NODE_ID every 500 ms (makeHeartbeatPayload), carrying rxCount.
     * - HEALTH_NODE_ID every 1000 ms (makeNodeHealthPayload, carrying `kind`) + the DiagSerial
     *   fault edge log (tag BRIDGE on NODE_ID 0, NODE otherwise). Compiled out with
     *   -DNODE_HEALTH_TELEM=0. The fault is DUPLICATE_NODE_ID while the duplicate check is live,
     *   else aggregateFaults().
     * - Duplicate NODE_ID check (#344), every node: a frame on the node's own HB_NODE_ID raises
     *   DUPLICATE_NODE_ID, ahead of every FaultSource; clears DUP_NODE_TIMEOUT_MS after the last.
     * - Master-loss watchdog on every node except PanelBridge (NODE_ID 0): once HB_0 has been
     *   seen, master loss is set after MASTER_TIMEOUT_MS of silence and cleared by the next HB_0.
     * - WARNING on a non-bridge node = master loss OR overheat (NODE_OVERHEAT_C builds only),
     *   written only when that OR changes. On PanelBridge an overheat is raise-only.
     *
     * Timers arm on the first call (first loop() after setup()), so the first HB/HEALTH go out
     * one period after boot completes.
     *
     * @param kind    Board kind for HEALTH_n (#344), required — each node library passes its role's
     *                (PanelGroup PANEL_GROUP, PduNode PDU, PanelBridge BRIDGE).
     * @param rxCount Caller-owned accepted RX count for the heartbeat; 0 if the node keeps none.
     * @note Samples millis() after drain() and compares signed (#225).
     */
    void serviceNodeStatus(NodeKind kind, uint16_t rxCount = 0);

    /** @brief The HEALTH_n payload serviceNodeStatus() last built (PanelBridge's node-0 row, #344).
     *         Not-yet-seen (dieTempC DIE_TEMP_UNAVAILABLE, the rest 0) before the first one. */
    const NodeHealthPayload& lastNodeHealth();

    /** @brief Master-loss timeout — 3× the 500 ms HB_0 cadence. */
    static constexpr uint32_t MASTER_TIMEOUT_MS = 1500;

    /** @brief Duplicate NODE_ID clear time — 6× the 500 ms HB cadence (#344). */
    static constexpr uint32_t DUP_NODE_TIMEOUT_MS = 3000;

} // namespace CANProtocol
```

### Node status service

Every node owes the same status traffic whatever else it does, so CANProtocol owns it rather than
each node library (#340 — PanelGroup and PanelBridge each carried a copy before). It is the lowest
layer that can: STM32Board sits below CANProtocol, so it cannot send frames without a circular
dependency.

| Duty | Cadence | Who |
|------|---------|-----|
| `HB_n` (`makeHeartbeatPayload`) | 500 ms | every node; PanelBridge's is `HB_0` |
| `HEALTH_n` (`makeNodeHealthPayload`, with the caller's `NodeKind`) + fault edge log | 1000 ms | every node, unless `-DNODE_HEALTH_TELEM=0` |
| Duplicate NODE_ID check → `DUPLICATE_NODE_ID` in `HEALTH_n` | checked every call | every node, PanelBridge included |
| Master-loss watchdog + overheat → `STM32Board::setWarning()` | checked every call | every node except NODE_ID 0 (bridge: overheat raise-only) |

- **`HB_0` intake.** On a non-bridge node `start()` adds `canIdHb(0)` to the hardware filter list
  (no-op under `filterAcceptAll()`), and `drain()` stamps each `HB_0` for the watchdog *before*
  passing it to `onReceive()` as usual — node code may still observe it.
- **Board kind (#344).** `kind` is a required argument: the node library is the role (sketches never
  call the service), so a new node library won't build until it says what it is. It never comes
  from `NODE_ID`.
- **Duplicate NODE_ID (#344).** `start()` adds the node's own `canIdHb(NODE_ID)` to the filter on
  every node (PanelBridge is pass-all anyway). bxCAN never receives its own frames, so `drain()`
  stamps any frame on that ID as foreign, then passes it to `onReceive()`. While a stamp is younger
  than `DUP_NODE_TIMEOUT_MS` the `HEALTH_n` fault is `DUPLICATE_NODE_ID` — chosen before
  `aggregateFaults()`, so no `FaultSource` can mask it (registry order is construction order, which
  can't guarantee a priority). It drives no LED, like every DEGRADED fault. In `startLoopback()` the
  node's own heartbeats come back, so the service counts the heartbeats it sent and `drain()`
  discounts that many own-ID frames.
- **WARNING rules.** The LED has one shared WARNING latch, so on a non-bridge node the service keeps
  its two causes apart and drives the latch from their OR, writing it only when the OR changes:
  - *master loss* — set when `HB_0` has been silent for more than `MASTER_TIMEOUT_MS` after a master
    was first seen (a cold start before the bridge never warns); cleared by the next `HB_0`;
  - *overheat* — the `HEALTH_n` overheat flag, only in builds that define `NODE_OVERHEAT_C`; set and
    cleared with each 1 s HEALTH sample.

  A returning master therefore never clears an overheat that is still true (PR #342 review — before
  #340 every `HB_0` cleared the latch, overheat included). On PanelBridge the latch also carries its
  dead-node WARNING, which the service cannot see, so there an overheat is raise-only.
- **Timing.** The first `serviceNodeStatus()` call arms both timers. That call is the first
  `loop()` after `setup()` returned — boot complete, `READY_n` already sent — so the first `HB_n`
  goes out ~500 ms after boot completes (FirmwarePlan 09 step 8) however long a node's setup takes
  after `start()`. `start()` only resets them (PR #342 review). The service samples `millis()` itself after `drain()`, and the
  watchdog compares elapsed time signed, so an `HB_0` stamped a millisecond after the caller's own
  `now` cannot read as overdue — the #225 pattern.

---

## Key Data Structures

### ControlPacket Batch State

CANProtocol owns the private batch builders for input/output routing frames. Higher layers
submit individual packets through `sendBatched()`; they never allocate or queue
`ControlPacketPair` payloads themselves.

Each active batched CAN ID stores:

```cpp
struct BatchState {
    uint32_t canId;
    bool hasA;
    ControlPacket a;
    uint8_t loopsWaited;
    uint32_t sinceMs;      // when `a` started waiting (#389)
};
```

Supported batched IDs are `CAN_ID_CTRL_BCAST`, `canIdEvt(n)`, `canIdEvtRel(n)`, `canIdEvtDir(n)`
and `canIdEvtAction(n)` — five slots, one per frame. When a second packet arrives, CANProtocol
sends `{a, b}` as one 8-byte frame. If no second packet arrives by the deadline serviced from
`drain()`, or if the caller explicitly calls `flushBatched(canId)`, CANProtocol sends `{a, null}`
where `null.controlId == 0x0000`. The deadline is two `drain()` calls **and** `BATCH_HOLD_MS`: 0 on
a node, so input latency stays two loops; 2 ms on PanelBridge (#389), whose loop takes microseconds —
two loops alone sent nearly every `CTRL_BCAST` frame half empty.

**A batched frame ID must have a slot registered in `_batches[]` during `begin()`.** `sendBatched()`
looks the ID up there and returns silently when it is absent — no log, no fault. Adding a batched
frame therefore means growing the array *and* registering the ID; doing only the latter writes past
the end of a fixed-size array, and doing only the former drops every packet on that frame while the
sending class looks correct.

#### Dispatch forms

The frame ID an input event arrives on selects how PanelBridge renders it for DCS-BIOS. The payload
`controlId` decides *routing* (DCS vs HID); the frame decides *form*. This is why an input class
that speaks a new form needs a new frame rather than a new value: the `canIdEvt` value space is
already fully spoken for as literal `set_state` values.

| Frame | Payload value | DCS-BIOS interface | Rendered as | Emitted by |
|---|---|---|---|---|
| `canIdEvt(n)` 0x200+n | absolute `uint16` | `set_state` | `%u` | `Switch2Pos`, `Switch3Pos`, `SwitchMultiPos`, `AnalogInput`, `AnalogMultiPos` |
| `canIdEvtRel(n)` 0x500+n | signed ±step | `variable_step` | `%+d` | `RotaryEncoder` (REL) |
| `canIdEvtDir(n)` 0x600+n | signed ±1 | `fixed_step` | `INC` / `DEC` | `RotaryEncoder` (DIR) |
| `canIdEvtAction(n)` 0x700+n | selector; `0` only | `action` | `TOGGLE` | `ActionButton` |

On the REL, DIR and ACTION frames the value is not a magnitude, so anything outside the defined set
is malformed: the bridge drops it and logs, rather than letting a corrupt value become a real
cockpit action.

### TX Ring Buffer

Every `send()` goes through a software ring (16 frames; 64 on PanelBridge, `CAN_TX_RING_SIZE`) and
leaves in the order it was sent. The TX-complete interrupt (`USB_HP_CAN1_TX_IRQHandler`) keeps the
three mailboxes loaded; `send()` and `drain()` run the same code with interrupts off.

Each queued entry stores:

```cpp
struct TxQueueEntry {
    uint32_t canId;
    uint8_t  len;
    uint8_t  data[8];
    uint8_t  attempts;  // bus-error attempts so far; at 3 the frame is dropped
};
```

```
Capacity:        16 frames (64 on PanelBridge)
Drop counter:    txDropCount() — a full ring, or a frame out of error attempts
```

**Settling a mailbox (#389, D32).** `AutoRetransmission` is off (below), so the controller tries a
frame once. A copy of each mailbox's frame is kept with it; before a mailbox is reused, its outcome
(one `TSR` read, cleared by writing `RQCP`) decides:

| Outcome | Action |
|---------|--------|
| `TXOK` | Done. |
| `ALST` — lost arbitration | Back to the head of the ring, always. |
| `TERR` — bus error | Back to the head of the ring until 3 attempts in all, then dropped and counted. |

**One frame per CAN ID in the mailboxes.** The ring head waits while a mailbox still holds a frame
with its CAN ID, so a resend can't land after a newer frame of the same ID (a pot would be left at an
older value). The TX-complete interrupt refills in microseconds, so this costs no throughput.

`CTRL_BCAST` carries DCS output state such as gauge positions, LEDs, and dimmers, as a
`ControlPacketPair`. It is queued in order and never coalesced (#389): a frame carries two of ~250
controls, so replacing a queued one loses two others' updates. It is the lowest-priority frame on
the bus (D31), so its frames are the ones that wait and get resent.

`EVT_n` carries input events as a `ControlPacketPair`. Every frame type — events, the periodic
`HB_n` / `HEALTH_n` / `TELEM_n`, `READY_n`, `SYNC_REQ`, `TEST_SEQ`, `ECHO_n` — follows the same
rules; none is coalesced.

### RX Queue

RX FIFO 0's interrupt (`USB_LP_CAN1_RX0_IRQHandler`) moves frames into a software ring (8 frames;
128 on PanelBridge, `CAN_RX_RING_SIZE`). `drain()` hands them to the callbacks in loop() context,
outside the ISR. A full ring drops the new frame (`rxDropCount()`).

The HAL is configured with `ReceiveFifoLocked = DISABLE`: if the 3-frame hardware FIFO fills
before the ISR drains it, a frame is lost in hardware. Each overrun is counted
(`rxOverrunCount()`). Until #389 the NVIC line was never enabled and the FIFO was only polled
from `drain()`, so a slow loop pass overran it without a trace.

Both CAN interrupts run at NVIC priority 2 — below the UARTs (1), above the timers (14). They
share their vectors with USB, which no OpenSkyhawk STM32 build enables (`-DUSB_NONE`).

---

## Implementation Notes

### HAL configuration — critical settings

Two HAL settings must be applied during peripheral init or the node will fail under any bus
fault. Both validated in Experiment B (2026-05-20 soak test and fault injection).

**AutoRetransmission: DISABLE**
With auto-retransmit enabled, a single unACKed frame (e.g. during a momentary bus fault)
fills all three TX mailboxes and the node enters bus-off immediately — the TX queue never
gets a chance to drain. With `DISABLE`, TEC rises +8 per failed attempt; the node enters
Error Passive at TEC=128 and self-heals when the fault clears. The resend decision is the
software's instead (TX Ring Buffer above): lost arbitration always, a bus error up to 3 attempts.

**AutoBusOff: ENABLE**
Hardware auto-recovery from bus-off takes ~3 ms at 500 kbps (1408 recessive bits). With
`ENABLE`, the CAN controller recovers automatically — no firmware intervention required.
With `DISABLE`, firmware would have to manually restart the controller after bus-off.

These are set once in `STM32Board::begin()` during peripheral configuration and must not be
overridden.

---

### Mandatory filter IDs

`start()` unconditionally adds CTRL_BCAST (0x7E0), TEST_SEQ (0x011), and SYNC_REQ (0x012)
to the active filter regardless of what the caller registered. These are global — every
node must receive them. It also adds the node's own `HB_n` (duplicate NODE_ID check, #344) and, on
every node except PanelBridge, `HB_0` (master-loss watchdog, #340). If `filterAcceptAll()` was called, the hardware is pass-all and
these additions are redundant but harmless.

### ControlPacketPair payload

`CTRL_BCAST` and `EVT_n` frames carry exactly 8 bytes: a raw `ControlPacketPair`.
Slot B is ignored when `b.controlId == 0x0000`. Pair decoding is not global; special frames
use their own explicit DLC/payload and may use a single 4-byte `ControlPacket` only when that
frame type says so.

The protocol allows CANProtocol to wait briefly for slot B, but not indefinitely. CANProtocol
owns the batch state for `CTRL_BCAST` and `EVT_n`, and `drain()` services the two-loop
deadline (plus 2 ms on PanelBridge, above). PanelBridge and PanelGroup call `sendBatched()` with individual `ControlPacket`s.
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

SYNC_REQ is intercepted by `drain()`. One naming another node (DLC ≥ 1, byte 0 ≠ `NODE_ID`) is
dropped; one naming this node, or an empty one, fires `onSyncReq()`. Not forwarded to `onReceive`.
PanelGroup registers `onSyncReq` to re-report its inputs. Once the boot READY is out, `drain()` then
sends READY flagged `RESYNC` when the answer has left — the TX ring empty and all three mailboxes
free, or after 50 ms (#383, #387). PanelBridge sends `SYNC_REQ` one node at a time
(`sendSyncReq()`); it never sends the empty form.

### CAN status monitoring

A periodic check (inside `drain()`) reads
`CAN1->ESR` and updates CanStatus:
- TEC > 0 and not bus-off → `TX_ERROR`
- BOFF bit set → `BUS_OFF`
- Otherwise → `NORMAL`

On any status change, the registered `onStatusChange` callback fires. STM32Board wires
this to the LED state machine.

### TX queue draining

Automatic — no explicit call needed from `loop()`. When a TX mailbox completes, the TX-complete
interrupt settles it and loads the next queued frame (#389). This keeps the bus saturated without
stalling the main loop.

---

## Dependencies

| Dependency | Source | Notes |
|---|---|---|
| STM32duino Arduino core | PlatformIO `framework = arduino` | HAL CAN, interrupt callbacks |
| STM32Board | `Firmware/Libraries/STM32Board` | `begin()` must be called first to configure the peripheral |
| HIDControls | `Firmware/Libraries/HIDControls` | Platform-agnostic library; `CANProtocol.h` includes it so CTRL_* constants are available transitively |
| `NODE_ID` define | `platformio.ini` `build_flags` | Used in `canIdHb(NODE_ID)` etc. |


### Entering the bootloader (#365)

A non-bridge node adds its own `BOOT_CMD_n` (`Boot::canIdBootCmd(NODE_ID)`, BootProtocol.h) to the
mandatory filter list (`ids[MAX_FILTER_IDS + 6]`). `drain()` handles it before the generic callback: a
`CONNECT` calls `STM32Board::enterBootloader()` (resets into the bootloader when one is installed,
else returns) and the frame is never forwarded. `makeReadyPayload()` fills byte 3 from
`STM32Board::bootloaderVersion()`. Depends on `BootProtocol` (header-only).
