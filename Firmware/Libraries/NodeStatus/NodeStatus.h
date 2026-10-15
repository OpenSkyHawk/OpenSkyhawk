/**
 * @file NodeStatus.h
 * @brief Neutral node-status contract for every OpenSkyhawk node (PanelGroup, PanelBridge, PDU).
 *
 * Owns the cross-node **status API** — not just health:
 *   - the `_NODE_STATUS` DCS-BIOS host-reporting contract (proto version, request address, message
 *     names) — **the canonical source the client's `sync-a4ec.ts` parses**;
 *   - `NodeHealthFlag` (HEALTH_n flag bits) and `NodeFaultCode` (the compact wire faultId dictionary
 *     the client maps to labels);
 *   - the `_NODE_TELEM` host contract and the TELEM_n page vocabulary (`TelemPage`, `PduRailFlag`,
 *     `PduStatusFlag`) — #216;
 *   - `FaultSource` — the interface a fault-producing object implements so a node-level aggregator
 *     can roll it up. A `FaultSource` is *one piece* of node status, not the whole story.
 *
 * This is the neutral home for node-status vocabulary so it isn't shaped around any one node
 * flavour (PanelGroup/OutputBase) or the HID namespace. `NodeHealthPayload` (the CAN HEALTH_n
 * frame struct) stays in `CANProtocol.h`; the fault *detail* strings stay local (DiagSerial),
 * never on the wire. Header-only contract + a tiny `FaultSource` registry (`NodeStatus.cpp`).
 *
 * @version 0.1.0
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>

// ── _NODE_STATUS DCS-BIOS host-reporting contract (#86) ───────────────────────
// PanelBridge reports connected nodes + health to the host over DCS-BIOS. Bump
// NODE_STATUS_PROTO_VERSION on any _NODE_STATUS wire change so the client's sync fails loudly.
// Full 26-hex field decode: FirmwarePlan/04-dcs-bios-integration.md.
#define NODE_STATUS_PROTO_VERSION 2
#define NODE_STATUS_REQ_ADDR      0x86FE     // host→device roster-request export address
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"

// ── _NODE_TELEM DCS-BIOS host-reporting contract (#216) ───────────────────────
// PanelBridge forwards every TELEM_n CAN frame to the host unread, as one DCS-BIOS message per
// frame: `_NODE_TELEM <nodeId(2 hex)><8 frame bytes as 16 hex, wire order>` (18 hex chars). Pushed
// as frames arrive — no request, no cache, no end marker. Versioned separately from
// _NODE_STATUS: a new TelemPage is additive and needs no bump; bump on a change to the message
// framing or to an existing page's layout. Full page layouts: FirmwarePlan/02-can-protocol.md.
#define NODE_TELEM_PROTO_VERSION  1
#define NODE_TELEM_MSG_NAME       "_NODE_TELEM"

/** @brief HEALTH_n `flags` bits — node-level health conditions (maps to NodeHealthPayload.flags). */
enum class NodeHealthFlag : uint8_t {
    OVERHEAT = 0x01,  // die temp >= NODE_OVERHEAT_C (opt-in, #213)
    DEGRADED = 0x02,  // a FaultSource reports faultCode() != NodeFaultCode::NONE (#163)
};

/**
 * @brief HEALTH_n `faultId` dictionary (#163) — cross-node fault codes.
 *
 * Coarse, one active at a time on the wire; the exact device is logged locally on DiagSerial,
 * not the frame. The client maps id → human label (SkyHawkClient#40). Each node type contributes
 * the codes relevant to its fault sources; append as new sources appear.
 */
enum class NodeFaultCode : uint8_t {
    NONE           = 0x00,
    I2C_PERIPHERAL = 0x01,  // an I2C device (OLED/mux/expander) tripped its I2cHealth breaker
    OVER_VOLTAGE   = 0x02,  // rail over-voltage (PDU)
    UNDER_VOLTAGE  = 0x03,  // rail under-voltage (PDU)
    SHORT_CIRCUIT  = 0x04,  // rail short / over-current (PDU)
    HOST_LINK_LOST = 0x05,  // PanelBridge lost the host serial link
    FUSE_OPEN      = 0x06,  // rail fuse blown — rail read ~0 V while the node is powered (PDU)
    OVER_TEMPERATURE = 0x07,  // board temperature at the critical threshold (PDU NTC)
    SENSOR_FAULT   = 0x08,  // a measurement channel reads implausibly (railed / open / shorted)
    // 0x09–0xFF reserved for future fault sources
};

/**
 * @brief TELEM_n page ids (#216) — byte 0 of TelemPayload, naming the layout of bytes 1-7.
 *
 * Global and append-only: a page id means the same layout whichever node sends it, so the client
 * decodes by page, not by node type. 0x00 is invalid. Layouts (little-endian):
 *   PDU_RAIL_12V / PDU_RAIL_5V: [1] PduRailFlag bits · [2-3] vAvgMv u16 · [4-5] vMinMv u16 ·
 *                               [6-7] iAvgMa int16
 *   PDU_STATUS:                 [1] PduStatusFlag bits · [2-3] ntcTempDC int16 (0.1 °C,
 *                               INT16_MIN = unavailable) · [4-5] i12PeakMa u16 · [6-7] i5PeakMa u16
 */
enum class TelemPage : uint8_t {
    INVALID      = 0x00,  // never sent
    PDU_RAIL_12V = 0x01,  // PDU 12 V rail: flags, avg + min voltage, avg current
    PDU_RAIL_5V  = 0x02,  // PDU 5 V rail: flags, avg + min voltage, avg current
    PDU_STATUS   = 0x03,  // PDU board: flags, NTC temperature, per-rail peak current
};

/** @brief PDU_RAIL_* page byte 1 — per-rail condition bits (all zero = healthy). */
enum class PduRailFlag : uint8_t {
    UNDER_V      = 0x01,  // rail below its under-voltage threshold
    OVER_V       = 0x02,  // rail above its over-voltage threshold
    OVER_CURRENT = 0x04,  // sustained current above the over-current threshold
    FUSE_OPEN    = 0x08,  // rail read ~0 V while the node is powered — fuse blown
    SENSOR_FAULT = 0x10,  // voltage or current channel reads implausibly (railed)
};

/** @brief PDU_STATUS page byte 1 — board condition bits (all zero = healthy, calibrated). */
enum class PduStatusFlag : uint8_t {
    UNCAL_VDDA   = 0x01,  // no VDDA calibration stored — readings assume a nominal 3.3 V
    UNCAL_ZERO   = 0x02,  // no current zero-offset calibration stored
    TEMP_WARN    = 0x04,  // board temperature at the warning threshold
    TEMP_CRIT    = 0x08,  // board temperature at the critical threshold
    NTC_FAULT    = 0x10,  // NTC channel open or shorted
};

namespace OpenSkyhawk {

/**
 * @brief A source of node faults — implemented by any object that can fault (#163).
 *
 * Self-registers into a static list at construction; a node-level aggregator walks
 * `FaultSource::head()` to decide node health, pick the primary `faultCode()` for HEALTH_n, and
 * print `faultDetail()` to DiagSerial. Implementers report *cached* state only — cheap, const, no
 * blocking I/O — since the aggregator runs on the periodic health path. Examples: DrumDisplay
 * (I2C), a PDU rail monitor (over/under-voltage/short), a PanelBridge host-link watchdog.
 *
 * @note **Lifetime: a FaultSource must have static/global lifetime** (same rule as OutputBase /
 *       InputBase). Registration is permanent — there is no unregister — so a stack/local
 *       FaultSource would leave a dangling pointer in the registry the aggregator walks. Construct
 *       fault sources as globals (or members of a global object), never on the stack.
 */
class FaultSource {
public:
    /** @brief Current fault code (NodeFaultCode::NONE when healthy). Cheap/const, cached state only. */
    virtual NodeFaultCode faultCode() const { return NodeFaultCode::NONE; }

    /** @brief Human-readable fault detail for the local DiagSerial tap only — never on the wire. */
    virtual const char* faultDetail() const { return ""; }

    /** @brief Head of the self-registered fault-source list. */
    static FaultSource* head();
    /** @brief Next fault source; nullptr at end. */
    FaultSource* next() const;

protected:
    FaultSource();          ///< Registers this instance into the list.
    ~FaultSource() = default;  ///< Protected, non-virtual: a base/mixin, never deleted through this type.

private:
    static FaultSource* _head;
    FaultSource* _next;
};

/**
 * @brief Roll up the registered fault sources into a single node fault code (#163).
 *
 * Walks `FaultSource::head()` and returns the **first** source reporting a non-`NONE`
 * `faultCode()`; if `detailOut` is non-null it is set to that source's `faultDetail()`. Returns
 * `NodeFaultCode::NONE` when every source is healthy. `*detailOut` is **always** a non-null string
 * (`""` when healthy or a source returns null) — callers never null-check.
 *
 * @note Iteration is **registry order = reverse construction order** (the intrusive list pushes at
 *       head), so the last-constructed fault source has priority. With one active fault at a time
 *       on the wire this rarely matters; a node with concurrent faults reports the head-most.
 * @note Cheap/const — sources report cached state only. Called on the periodic health path.
 *
 * @param detailOut  Optional out-param for the local DiagSerial detail string (never null on return).
 * @return           The primary NodeFaultCode, or NONE if no source is faulted.
 */
NodeFaultCode aggregateFaults(const char** detailOut = nullptr);

} // namespace OpenSkyhawk
