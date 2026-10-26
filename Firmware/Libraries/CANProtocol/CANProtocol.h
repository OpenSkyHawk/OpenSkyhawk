/**
 * @file CANProtocol.h
 * @brief Shared CAN bus types, frame IDs, and runtime API for OpenSkyhawk STM32 nodes.
 *
 * @details Owns all CAN bus interaction for PanelGroup and PanelBridge nodes.
 * Types and constants (ControlPacket, CanStatus, frame IDs, CAN ID functions) are
 * platform-agnostic. The runtime namespace (filters, lifecycle, send, callbacks,
 * diagnostics) is STM32-only and guarded by ARDUINO_ARCH_STM32.
 * CAN arbitration IDs (CAN_ID_*, canId*()) and payload ControlPacket::controlId
 * values are separate namespaces; equal numeric values do not conflict because
 * they occupy different CAN frame fields.
 *
 * Dependency: STM32Board::begin() must be called before CANProtocol::start().
 * CANProtocol owns CAN bus operation; STM32Board owns peripheral hardware init.
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>
#include <HIDControls.h>
#include <NodeStatus.h>   // NodeFaultCode / NodeHealthFlag — packed into HEALTH_n by makeNodeHealthPayload

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
    ControlPacket b;  ///< controlId 0x0000 = unused padding slot
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
    uint16_t rxCount;  ///< Caller-owned accepted RX count, little-endian
    uint16_t esr;      ///< TEC at HB_ESR_TEC_SHIFT, REC at HB_ESR_REC_SHIFT (NodeStatus.h)
};

/**
 * @brief 8-byte payload carried by HEALTH_n frames — per-node health telemetry.
 *
 * Sent every 1000 ms by every STM32 node. This is the shared node-health wire contract;
 * separate features populate their own fields, all within the fixed 8 bytes:
 *   - Temperature (#213): dieTempC, flags bit0 — read from the MCU's built-in internal
 *     temperature sensor (ADC ch16); no external parts, no PCB change.
 *   - Degraded state (#163): flags bit1 + faultId — a node that is alive but has a faulted
 *     FaultSource reporting a NodeFaultCode (NodeStatus.h). aggregateFaults() picks the primary
 *     faultId, makeNodeHealthPayload derives DEGRADED. faultMask stays 0 (reserved, see below).
 *   - Board kind (#344): nodeKind — a NodeKind (NodeStatus.h) set by the node's firmware role,
 *     never derived from NODE_ID. Pre-#344 firmware sends 0 here = NodeKind::UNKNOWN.
 * PanelBridge caches HEALTH_1–HEALTH_63 per node and forwards them in _NODE_STATUS, together with
 * its own HEALTH_0 as the node-0 row.
 *
 * @note The STM32F103 internal sensor is UNCALIBRATED (no factory trim): ~±few °C absolute,
 *       measures DIE temperature (not ambient) with a self-heat offset. Use for relative
 *       trend and per-node overheat flagging, not precise ambient measurement.
 * @note Rail voltage/current is intentionally NOT here — that is the PDU's dedicated power
 *       telemetry (TELEM_n / TelemPayload, #216; INA180A2 current sense on #202), not generic
 *       per-node health.
 */
struct __attribute__((packed)) NodeHealthPayload {
    uint8_t  nodeId;     ///< 0: node ID — redundant with CAN ID, aids logging
    int8_t   dieTempC;   ///< 1: internal die temp, whole °C (DIE_TEMP_UNAVAILABLE) — #213
    uint8_t  flags;      ///< 2: NodeHealthFlag bits (NodeStatus.h): OVERHEAT (#213), DEGRADED (#163)
    uint8_t  faultMask;  ///< 3: fault source/domain bitmap — reserved for future domain bits (0)
    uint8_t  faultId;    ///< 4: NodeFaultCode (NodeStatus.h): 0 = no fault, else primary fault — #163
    uint8_t  nodeKind;   ///< 5: NodeKind (NodeStatus.h): board kind from the firmware role — #344
    uint8_t  rsvd[2];    ///< 6-7: reserved, transmit 0 — future generic health (resetCause, freeRAM, …)
};
static_assert(sizeof(NodeHealthPayload) == 8, "NodeHealthPayload must fill one classic CAN frame");

/**
 * @brief 8-byte payload carried by TELEM_n frames — paged node telemetry (#216).
 *
 * Byte 0 is a TelemPage (NodeStatus.h) naming the layout of the other seven bytes; page ids are
 * global and append-only, so a page means the same thing whichever node sends it. Multi-byte
 * fields inside a page are little-endian. PanelBridge forwards every TELEM_n frame to the host
 * unread (`_NODE_TELEM`), so a new page type needs no bridge change — the client owns the decode.
 *
 * First user: the PDU monitoring node — pages PDU_RAIL_12V / PDU_RAIL_5V / PDU_STATUS. Generic
 * node health (die temp, fault id) stays in HEALTH_n and is never repeated here.
 */
struct __attribute__((packed)) TelemPayload {
    uint8_t page;     ///< 0: TelemPage (NodeStatus.h); 0x00 is invalid
    uint8_t body[7];  ///< 1-7: page-defined, little-endian multi-byte fields
};
static_assert(sizeof(TelemPayload) == 8, "TelemPayload must fill one classic CAN frame");

/**
 * @brief 8-byte payload carried by READY_n — the node's boot announcement (#295).
 *
 * Sent once per boot, after the boot EVT burst, so every reflash, reset or power cycle announces
 * the firmware the node now runs. PanelBridge caches the version per node and reports it in
 * `_NODE_STATUS` (v4). Firmware older than this payload sends READY_n empty (DLC 0): the bridge
 * then reports the version as not reported (0.0.0).
 *
 * Sent again, flagged ReadyFlag::RESYNC, in answer to every SYNC_REQ after that (#383): a bridge
 * that booted after the node missed its boot READY and would otherwise never learn its version.
 */
struct __attribute__((packed)) ReadyPayload {
    uint8_t fwMajor;     ///< 0: firmware release version (FirmwareVersion.h)
    uint8_t fwMinor;     ///< 1
    uint8_t fwPatch;     ///< 2
    uint8_t bootloader;  ///< 3: installed bootloader's protocol version, 0 = none (#365)
    uint8_t flags;       ///< 4: ReadyFlag bits (#383); 0 = the boot READY
    uint8_t rsvd[3];     ///< 5-7: reserved, transmit 0
};

/// ReadyPayload `flags` bits (#383).
enum class ReadyFlag : uint8_t {
    RESYNC = 0x01,  // an answer to SYNC_REQ, not a boot: the bridge caches the version but must not
                    // broadcast SYNC_REQ for it (that would answer itself forever)
};
static_assert(sizeof(ReadyPayload) == 8, "ReadyPayload must fill one classic CAN frame");

/** @brief CAN bus health states. Reported to STM32Board via onStatusChange(). */
enum class CanStatus {
    STARTING,   ///< CAN peripheral configured, not yet started
    NORMAL,     ///< Bus active, no errors
    TX_ERROR,   ///< TEC > 0 — transmit errors accumulating
    BUS_OFF,    ///< CAN controller halted — bus-off condition
};

// ── Fixed CAN arbitration IDs (PanelBridge -> All) ───────────────────────────

static constexpr uint32_t CAN_ID_CTRL_BCAST = 0x010;  ///< Broadcast ControlPacketPair to all panels
static constexpr uint32_t CAN_ID_TEST_SEQ   = 0x011;  ///< RTT throughput test
static constexpr uint32_t CAN_ID_SYNC_REQ   = 0x012;  ///< Request nodes to re-send their state (see below)

/// SYNC_REQ payload (#387): byte 0 = the NODE_ID it is for — only that node answers. An empty
/// SYNC_REQ (DLC 0) is for every node; PanelBridge no longer sends one (it syncs one node at a
/// time), but nodes still honour it.
static constexpr uint8_t SYNC_REQ_LEN_TARGETED = 1;

// ── CAN ID functions (per-node IDs computed from NODE_ID) ─────────────────────

/** @brief Heartbeat frame ID for node n. Range 0x100-0x13F; n=GATEWAY_NODE_ID (0) is PanelBridge. */
constexpr uint32_t canIdHb(uint8_t n)    { return 0x100 + n; }

/** @brief Node-health/thermal telemetry frame ID for node n. Range 0x140-0x17F; n=0 is PanelBridge.
 *         Carries NodeHealthPayload (internal die temp; degraded/faultId once #163 lands). */
constexpr uint32_t canIdHealth(uint8_t n) { return 0x140 + n; }

/** @brief Paged node-telemetry frame ID for node n (TelemPayload, #216). Range 0x181-0x1BF.
 *         Sits with HB (0x100) and HEALTH (0x140) in the node-status block; PanelBridge forwards
 *         it to the host unread as `_NODE_TELEM`. */
constexpr uint32_t canIdTelem(uint8_t n)  { return 0x180 + n; }

/** @brief Input event frame ID for node n. Range 0x201-0x23F. */
constexpr uint32_t canIdEvt(uint8_t n)   { return 0x200 + n; }

/** @brief Relative-input event frame ID for node n. RotaryEncoder REL mode: payload value is a
 *         signed ±step (int16); the bridge formats it `%+d` for a DCS-BIOS variable_step control.
 *         Range 0x501-0x53F. */
constexpr uint32_t canIdEvtRel(uint8_t n) { return 0x500 + n; }

/** @brief Directional-input event frame ID for node n. RotaryEncoder DIR mode: payload value is
 *         a signed ±1 (int16); the bridge formats it `INC`/`DEC` for a DCS-BIOS fixed_step control.
 *         Range 0x601-0x63F. */
constexpr uint32_t canIdEvtDir(uint8_t n) { return 0x600 + n; }

/** @brief Action-input event frame ID for node n. ActionButton: the payload value is a selector,
 *         not a magnitude — 0 means `TOGGLE`, the only argument DCS-BIOS's action interface
 *         defines. Any other value is malformed and the bridge drops it. Range 0x701-0x73F.
 *
 *         This frame exists because an action is a *keyword*, not a number: the ABS value space is
 *         already spoken for as literal set_state values, so `TOGGLE` has no integer to ride on —
 *         the same reason DIR exists for `INC`/`DEC`. */
constexpr uint32_t canIdEvtAction(uint8_t n) { return 0x700 + n; }

/** @brief TEST_SEQ echo frame ID for node n. Range 0x301-0x33F. */
constexpr uint32_t canIdEcho(uint8_t n)  { return 0x300 + n; }

/** @brief Boot-complete READY frame ID for node n. Range 0x401-0x43F. */
constexpr uint32_t canIdReady(uint8_t n) { return 0x400 + n; }

// 0x740-0x7BF and 0x7C0-0x7C1: the bootloader's frames (#363) — BOOT_CMD_n = 0x740+n (bridge → node
// n), BOOT_RSP_n = 0x780+n (node n → bridge), and the unconfigured-board pair. The lowest priority on
// the bus, so update traffic never delays a control. Defined in BootProtocol.h, not here.

// ── controlId namespace ───────────────────────────────────────────────────────
// Payload controlIds live inside ControlPacket data bytes. They are intentionally
// separate from the 11-bit CAN arbitration IDs above.
// CTRL_ID_HID_MIN / CTRL_ID_HID_MAX are defined as macros in HIDControls.h (included above).
// Do not redeclare them here — macro expansion would cause a syntax error.

static constexpr uint16_t CTRL_ID_DCS_MIN  = 0x8000;  ///< DCS-BIOS range start
static constexpr uint16_t CTRL_ID_DCS_MAX  = 0x86FF;  ///< DCS-BIOS range end

// ── UART diagnostic framing constants (PanelBridge -> SimGateway) ─────────────
// Used by SimGateway to parse the UART diagnostic stream from PanelBridge.

static constexpr uint8_t DIAG_MAGIC = 0xAA;  ///< Frame sync byte
static constexpr uint8_t DIAG_RTT   = 0x01;  ///< Round-trip time measurement frame
static constexpr uint8_t DIAG_HB    = 0x02;  ///< Sub-node heartbeat frame
static constexpr uint8_t DIAG_ERR   = 0x03;  ///< CAN error counter frame
static constexpr uint8_t DIAG_EVT   = 0x04;  ///< Sub-node input event forwarded upstream

// ── Callback types ────────────────────────────────────────────────────────────

/** @brief Fired when CAN bus status changes. Register via onStatusChange(). */
using CanStatusCallback  = void(*)(CanStatus status);

/** @brief Fired when a CAN frame is received. Register via onReceive(). */
using CanRxCallback      = void(*)(uint32_t canId, const uint8_t* data, uint8_t len);

/** @brief Fired when SYNC_REQ is received. Register via onSyncReq(). */
using CanSyncReqCallback = void(*)();

// ── Runtime API (STM32 only) ──────────────────────────────────────────────────
//
// Library-internal: the node libraries (PanelGroup, PanelBridge, PduNode) drive the bus through
// these, not sketches. Not covered by the compatibility policy (#295) — except sendBatched(), which
// a custom control class uses to send its value (with ControlPacket and the canIdEvt* functions
// above, which are covered with the wire format).

#ifdef ARDUINO_ARCH_STM32

namespace CANProtocol {

    // ── Filter configuration (call before start()) ────────────────────────────

    /**
     * @brief Accept all incoming CAN frames. Use for PanelBridge.
     *
     * Configures a pass-all hardware mask filter. Mandatory IDs (CTRL_BCAST, TEST_SEQ,
     * SYNC_REQ) are included but have no effect with pass-all active.
     */
    void filterAcceptAll();

    /**
     * @brief Accept a specific CAN ID. Use for PanelGroup nodes.
     *
     * Adds one ID to the hardware filter list. Call multiple times for multiple IDs.
     * CTRL_BCAST (0x010), TEST_SEQ (0x011), and SYNC_REQ (0x012) are always included
     * automatically by start() — do not add them manually.
     *
     * @param canId 11-bit standard CAN ID to accept.
     */
    void filterAcceptId(uint32_t canId);

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Start the CAN peripheral and apply registered filters.
     *
     * Must be called after STM32Board::begin() and after all filter and callback
     * registrations. Always adds CTRL_BCAST, TEST_SEQ, and SYNC_REQ to the active
     * filter — mandatory for all nodes, cannot be excluded — plus HB_0 on every node except
     * PanelBridge (NODE_ID 0), for serviceNodeStatus()'s master-loss watchdog, and the node's own
     * `HB_NODE_ID`, for its duplicate NODE_ID check (#344).
     *
     * Resets the serviceNodeStatus() timers, which arm on its first call. Fires the
     * onStatusChange callback with NORMAL on success.
     */
    void start();

    /**
     * @brief Start in silent loopback mode — for bench testing only.
     *
     * Identical to start() but uses CAN_MODE_SILENT_LOOPBACK: frames transmitted by
     * this node are received back internally without going on the physical bus.
     * The duplicate NODE_ID check discounts the node's own looped-back heartbeats, so only an
     * extra `HB_NODE_ID` a test sends itself reads as a duplicate.
     *
     * @note Never call this in production firmware.
     */
    void startLoopback();

    /**
     * @brief Process RX callbacks and batched-ControlPacket deadlines. Call once per loop().
     *
     * Drains all frames received since the last call:
     * - SYNC_REQ: fires onSyncReq(), not forwarded to onReceive().
     * - TEST_SEQ: auto-replies with ECHO carrying the same payload, not forwarded.
     * - HB_0 on a non-bridge node: stamped for serviceNodeStatus()'s watchdog, then onReceive().
     * - The node's own `HB_NODE_ID`: stamped for serviceNodeStatus()'s duplicate NODE_ID check
     *   (#344), then onReceive().
     * - The node's own `BOOT_CMD_n` (not on the bridge): a CONNECT resets into the bootloader when one
     *   is installed (STM32Board::enterBootloader(), #365); never forwarded.
     * - All other frames: fires onReceive().
     *
     * Also services sendBatched() deadlines: a half-full ControlPacketPair that has
     * waited two drain() calls is flushed with slot B set to the null sentinel.
     */
    void drain();

    // ── Transmit ──────────────────────────────────────────────────────────────

    /**
     * @brief Send a CAN frame.
     *
     * Sends immediately if a TX mailbox is free; otherwise enqueues in the TX ring
     * buffer (~16 entries), drained automatically via TX-complete interrupt.
     * CTRL_BCAST coalesces stale queued state (newest wins). EVT/control frames
     * retry up to 3 attempts then drop. All drops increment txDropCount().
     *
     * @param canId  11-bit standard CAN ID.
     * @param data   Pointer to payload bytes.
     * @param len    Payload length in bytes (0-8).
     */
    void send(uint32_t canId, const uint8_t* data, uint8_t len);

    /**
     * @brief Submit one ControlPacket to a CANProtocol-owned ControlPacketPair batch.
     *
     * Valid only for CAN_ID_CTRL_BCAST and the batched event frames — canIdEvt(n)
     * (absolute), canIdEvtRel(n) (RotaryEncoder REL), canIdEvtDir(n) (RotaryEncoder
     * DIR), and canIdEvtAction(n) (ActionButton) — each with its own pending slot. Pairs
     * two consecutive packets into one 8-byte frame. If slot B does not arrive within two
     * drain() calls, slot A is sent with slot B set to the null sentinel (controlId == 0x0000).
     *
     * A frame ID with no batch slot is **silently discarded** — adding a batched frame means
     * registering it in _batches[] in begin(), not just defining its canId helper.
     *
     * @param canId  CAN_ID_CTRL_BCAST, canIdEvt(NODE_ID), canIdEvtRel(NODE_ID),
     *               canIdEvtDir(NODE_ID), or canIdEvtAction(NODE_ID).
     * @param pkt    ControlPacket to batch.
     */
    void sendBatched(uint32_t canId, const ControlPacket& pkt);

    /**
     * @brief Force a half-full ControlPacketPair batch to send immediately.
     *
     * If the named CAN ID has a pending slot A, sends it with slot B as the null
     * sentinel. No-op if no packet is pending.
     *
     * @param canId CAN_ID_CTRL_BCAST, canIdEvt(NODE_ID), canIdEvtRel(NODE_ID),
     *              canIdEvtDir(NODE_ID), or canIdEvtAction(NODE_ID).
     */
    void flushBatched(uint32_t canId);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    /**
     * @brief Register a CAN bus status change callback.
     *
     * Fires immediately with the current status upon registration (STARTING before
     * start() is called), then on each subsequent status transition.
     *
     * @param cb Callback to invoke with the new CanStatus.
     */
    void onStatusChange(CanStatusCallback cb);

    /**
     * @brief Register a SYNC_REQ handler.
     *
     * Fired by drain() when a SYNC_REQ (0x012) for this node arrives — one naming its NODE_ID, or an
     * untargeted one. PanelGroup registers this to re-send all its inputs. Once the node's transmit
     * queue has emptied afterwards, drain() sends a READY flagged ReadyFlag::RESYNC: the end of the
     * node's answer, which the bridge waits for before syncing the next node (#383, #387).
     *
     * @param cb Callback to invoke on SYNC_REQ receipt.
     */
    void onSyncReq(CanSyncReqCallback cb);

    /**
     * @brief Register a general-purpose RX frame handler.
     *
     * Fired by drain() for all frames except SYNC_REQ and TEST_SEQ.
     * Only one handler per node — PanelGroup and PanelBridge each register their own.
     *
     * @param cb Callback invoked with canId, data pointer, and payload length.
     */
    void onReceive(CanRxCallback cb);

    // ── Diagnostics ───────────────────────────────────────────────────────────

    /** @brief Return the CAN Transmit Error Counter (0-255) from ESR. */
    uint8_t tec();

    /** @brief Return the CAN Receive Error Counter (0-255) from ESR. */
    uint8_t rec();

    /** @brief Return true if the CAN controller is in bus-off state. */
    bool busOff();

    /**
     * @brief Build the standard 8-byte heartbeat payload for the current node.
     *
     * Fills uptime, CAN health flags, and ESR-derived TEC/REC from CANProtocol state.
     *
     * @param nodeId  Node ID (1-63 PanelGroup; GATEWAY_NODE_ID for PanelBridge).
     * @param rxCount Caller-owned receive counter to embed in the payload.
     * @return        Fully populated HeartbeatPayload ready to send as HB_n.
     */
    HeartbeatPayload makeHeartbeatPayload(uint8_t nodeId, uint16_t rxCount);

    /**
     * @brief Build the 8-byte node-health payload for the current node.
     *
     * The CAN/DCS packing boundary: takes the internal die temperature (via
     * STM32Board::readDieTempC()) and the node's aggregated fault (via NodeStatus::aggregateFaults())
     * and packs them into a NodeHealthPayload. Sets the overheat flag (bit0) only when NODE_OVERHEAT_C
     * is defined and dieTempC meets it; sets the DEGRADED flag (bit1) and faultId when `fault` is not
     * NONE (the enum→byte cast happens here), and nodeKind from `kind`. faultMask + reserved bytes
     * stay 0.
     *
     * @param nodeId   Node ID (1-63 PanelGroup; GATEWAY_NODE_ID PanelBridge).
     * @param kind     The node's board kind (#344) — from its firmware role, never from nodeId.
     * @param dieTempC Internal die temp in whole °C (DIE_TEMP_UNAVAILABLE = unavailable).
     * @param fault    Aggregated node fault (NodeFaultCode::NONE = healthy). Default NONE.
     * @return         Fully populated NodeHealthPayload ready to send as HEALTH_n.
     */
    NodeHealthPayload makeNodeHealthPayload(uint8_t nodeId, NodeKind kind, int8_t dieTempC,
                                            NodeFaultCode fault = NodeFaultCode::NONE);

    /**
     * @brief Build the 8-byte READY_n payload: this build's firmware release version (#295).
     * @param flags ReadyFlag bits — 0 for the boot READY.
     * @return ReadyPayload with fwMajor/fwMinor/fwPatch from FirmwareVersion.h, the bootloader
     *         version (STM32Board::bootloaderVersion()), @p flags, reserved bytes 0.
     */
    ReadyPayload makeReadyPayload(uint8_t flags = 0);

    /**
     * @brief Send this node's boot READY_n (`canIdReady(NODE_ID)`) carrying makeReadyPayload().
     *
     * Call once at the end of the node's setup(), after its boot EVT burst. PanelBridge marks the
     * node alive, caches its version and queues the node for a sync. Bounded retry like every other
     * special frame (02-can-protocol.md). From then on drain() ends every answer to a SYNC_REQ
     * with a READY flagged ReadyFlag::RESYNC (#383, #387) — nothing for the caller to do.
     */
    void sendReady();

    /** @brief Return the cumulative TX queue drop count since startup. */
    uint32_t txDropCount();

    /**
     * @brief Return the cumulative RX drop count since startup (#387): frames lost because the
     *        software RX ring was full when they arrived. Should stay 0; nonzero means loop() falls
     *        behind the bus.
     */
    uint32_t rxDropCount();

    /**
     * @brief Ask one node to re-send its state: SYNC_REQ naming @p nodeId (#387). PanelBridge only.
     * @param nodeId 1-63.
     */
    void sendSyncReq(uint8_t nodeId);

    // ── Node status service (every node) ──────────────────────────────────────

    /**
     * @brief Run the node-generic status duties. Call once per loop(), after drain().
     *
     * Every node on the bus owes the same status traffic, whatever else it does (#340):
     * - `HB_NODE_ID` every 500 ms (makeHeartbeatPayload), carrying `rxCount`.
     * - `HEALTH_NODE_ID` every 1000 ms (makeNodeHealthPayload) with `kind`, the internal die temp and
     *   the node fault, plus the DiagSerial fault edge log (tag `BRIDGE` on NODE_ID 0, `NODE`
     *   otherwise). Compiled out with `-DNODE_HEALTH_TELEM=0`.
     * - Duplicate NODE_ID check (#344), on every node: a node never receives its own frames, so a
     *   frame on its own `HB_NODE_ID` (start() adds it to the filter) means another board has the
     *   same NODE_ID. The node fault is then `NodeFaultCode::DUPLICATE_NODE_ID`, ahead of every
     *   FaultSource (the NODE_ID is wrong, so nothing else this ID reports can be trusted); it
     *   clears DUP_NODE_TIMEOUT_MS after the last foreign frame. aggregateFaults() supplies the
     *   fault otherwise.
     * - Master-loss watchdog, on every node except PanelBridge (NODE_ID 0): once a master
     *   heartbeat (`HB_0`) has been seen, master loss is set after MASTER_TIMEOUT_MS of silence and
     *   cleared by the next `HB_0`.
     * - WARNING on a non-bridge node = master loss OR overheat (overheat only when a build defines
     *   `NODE_OVERHEAT_C`). The service tracks the two causes separately and writes the shared
     *   latch only when their OR changes, so a returning master never clears a live overheat, and
     *   overheat clears once the die cools. On PanelBridge an overheat is raise-only, because its
     *   latch also carries PanelBridge's dead-node WARNING.
     *
     * The first call arms the HB/HEALTH timers — the first loop() after setup(), i.e. boot
     * complete — so the first HB goes out ~500 ms and the first HEALTH ~1000 ms after it.
     *
     * `HB_0` arrival is recorded by drain(), which still passes the frame to onReceive().
     * start() adds `HB_0` to the hardware filter on non-bridge nodes, so callers never
     * register it by hand.
     *
     * @param kind    This node's board kind (#344), sent in every HEALTH_n. Each node library
     *                passes its own (PanelGroup → PANEL_GROUP, PduNode → PDU, PanelBridge →
     *                BRIDGE), so a sketch never sets it. Required: a role can't forget it.
     * @param rxCount Caller-owned accepted RX count for the heartbeat (PanelGroup: accepted
     *                CTRL_BCAST frames). Pass 0 when the node keeps no count.
     *
     * @note Samples millis() itself, after drain() has stamped any `HB_0`, and compares elapsed
     *       time signed — so a millisecond ticking during drain() cannot read as a timeout (#225).
     */
    void serviceNodeStatus(NodeKind kind, uint16_t rxCount = 0);

    /**
     * @brief The HEALTH_n payload serviceNodeStatus() last built for this node (#344).
     *
     * PanelBridge reports it as its own node-0 `_NODE_STATUS` row. Before the first HEALTH_n
     * (or with `-DNODE_HEALTH_TELEM=0`) it reads not-yet-seen: dieTempC DIE_TEMP_UNAVAILABLE,
     * everything else 0 (nodeKind UNKNOWN) — the same rule the bridge applies to a node it has no HEALTH_n from.
     */
    const NodeHealthPayload& lastNodeHealth();

    /** @brief Master-loss timeout: 3× the 500 ms `HB_0` cadence, matching PanelBridge's own
     *         node timeout ratio (3000 ms / 500 ms). */
    static constexpr uint32_t MASTER_TIMEOUT_MS = 1500;

    /** @brief Duplicate NODE_ID clear time (#344): DUPLICATE_NODE_ID drops this long after the
     *         last foreign frame on our own `HB_NODE_ID` — 6× the 500 ms HB cadence, matching
     *         PanelBridge's own node timeout. A live duplicate re-raises within one HB period. */
    static constexpr uint32_t DUP_NODE_TIMEOUT_MS = 3000;

} // namespace CANProtocol

#endif // ARDUINO_ARCH_STM32
