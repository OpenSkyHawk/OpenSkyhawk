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
 * @version 0.2.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>
#include <HIDControls.h>

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
 * Sent every 500 ms by PanelGroup nodes. PanelBridge reads HB_1–HB_63 to track
 * PanelGroup health and populate diagnostics. HB_0 is reserved but not transmitted.
 */
struct __attribute__((packed)) HeartbeatPayload {
    uint8_t  nodeId;   ///< Node ID — redundant with CAN ID, aids logging
    uint8_t  flags;    ///< bit0=BOFF, bit1=EPVF (Error Passive; TEC >= 128)
    uint16_t uptime;   ///< Seconds since boot, little-endian (wraps at ~18 h)
    uint16_t rxCount;  ///< Caller-owned accepted RX count, little-endian
    uint16_t esr;      ///< (CAN1->ESR >> 16): low byte=TEC, high byte=REC
};

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
static constexpr uint32_t CAN_ID_SYNC_REQ   = 0x012;  ///< Request all nodes to re-poll inputs

// ── CAN ID functions (per-node IDs computed from NODE_ID) ─────────────────────

/** @brief Heartbeat frame ID for node n. Range 0x100-0x13F; n=0 is PanelBridge. */
constexpr uint32_t canIdHb(uint8_t n)    { return 0x100 + n; }

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

/** @brief TEST_SEQ echo frame ID for node n. Range 0x301-0x33F. */
constexpr uint32_t canIdEcho(uint8_t n)  { return 0x300 + n; }

/** @brief Boot-complete READY frame ID for node n. Range 0x401-0x43F. */
constexpr uint32_t canIdReady(uint8_t n) { return 0x400 + n; }

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
     * filter — mandatory for all nodes, cannot be excluded.
     *
     * Fires the onStatusChange callback with NORMAL on success.
     */
    void start();

    /**
     * @brief Start in silent loopback mode — for bench testing only.
     *
     * Identical to start() but uses CAN_MODE_SILENT_LOOPBACK: frames transmitted by
     * this node are received back internally without going on the physical bus.
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
     * Valid only for CAN_ID_CTRL_BCAST and canIdEvt(n). Pairs two consecutive packets
     * into one 8-byte frame. If slot B does not arrive within two drain() calls,
     * slot A is sent with slot B set to the null sentinel (controlId == 0x0000).
     *
     * @param canId  CAN_ID_CTRL_BCAST or canIdEvt(NODE_ID).
     * @param pkt    ControlPacket to batch.
     */
    void sendBatched(uint32_t canId, const ControlPacket& pkt);

    /**
     * @brief Force a half-full ControlPacketPair batch to send immediately.
     *
     * If the named CAN ID has a pending slot A, sends it with slot B as the null
     * sentinel. No-op if no packet is pending.
     *
     * @param canId CAN_ID_CTRL_BCAST or canIdEvt(NODE_ID).
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
     * Fired by drain() when SYNC_REQ (0x012) is received. PanelGroup registers this
     * to trigger a re-poll of all registered input objects.
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
     * @param nodeId  Node ID (1-63 PanelGroup; 0 reserved for PanelBridge).
     * @param rxCount Caller-owned receive counter to embed in the payload.
     * @return        Fully populated HeartbeatPayload ready to send as HB_n.
     */
    HeartbeatPayload makeHeartbeatPayload(uint8_t nodeId, uint16_t rxCount);

    /** @brief Return the cumulative TX queue drop count since startup. */
    uint32_t txDropCount();

} // namespace CANProtocol

#endif // ARDUINO_ARCH_STM32
