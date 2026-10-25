/**
 * @file NodeStatus.h
 * @brief Neutral node-status contract for every OpenSkyhawk node (PanelGroup, PanelBridge, PDU).
 *
 * Owns the cross-node **status API** — not just health:
 *   - the `_NODE_STATUS` DCS-BIOS host-reporting contract (proto version, request address, message
 *     names, `GATEWAY_NODE_ID`, row layout `NodeStatusField`) — **the canonical source the client's
 *     `sync-a4ec.ts` parses**;
 *   - `HeartbeatFlag` + the `HB_ESR_*` split (HB_n flags / esr fields);
 *   - `NodeHealthFlag` (HEALTH_n flag bits), `DIE_TEMP_UNAVAILABLE`, and `NodeFaultCode` (the compact
 *     wire faultId dictionary the client maps to labels);
 *   - `NodeKind` (HEALTH_n board-kind byte — what board a NODE_ID is, announced by its firmware);
 *   - the `_NODE_TELEM` host contract and the TELEM_n page vocabulary (`TelemPage`, `PduRailFlag`,
 *     `PduStatusFlag`, the per-page field offsets, `NTC_TEMP_UNAVAILABLE`) — #216;
 *
 * Every wire value is a named `enum class X : T { NAME = literal }` or `constexpr T NAME = literal`
 * so the client sync can read it (#346) — never only a comment, a bare literal or a format string.
 *   - `FaultSource` — the interface a fault-producing object implements so a node-level aggregator
 *     can roll it up. A `FaultSource` is *one piece* of node status, not the whole story.
 *
 * This is the neutral home for node-status vocabulary so it isn't shaped around any one node
 * flavour (PanelGroup/OutputBase) or the HID namespace. `NodeHealthPayload` (the CAN HEALTH_n
 * frame struct) stays in `CANProtocol.h`; the fault *detail* strings stay local (DiagSerial),
 * never on the wire. Header-only contract + a tiny `FaultSource` registry (`NodeStatus.cpp`).
 *
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#pragma once
#include <stdint.h>

// ── _NODE_STATUS DCS-BIOS host-reporting contract (#86) ───────────────────────
// PanelBridge reports connected nodes + health to the host over DCS-BIOS. Bump
// NODE_STATUS_PROTO_VERSION on any _NODE_STATUS wire change so the client's sync fails loudly.
// Full 34-hex field decode: FirmwarePlan/04-dcs-bios-integration.md.
#define NODE_STATUS_PROTO_VERSION 4
// Host→device roster-request export address (#295, v4). No DCS-BIOS module exports here: every
// aircraft ends by 0xB2FB and MetadataEnd is 0xFFFE (checked against DCS-BIOS v0.11.8). The old
// 0x86FE is a live address for the AH-64D and the Mirage F1, so flying either answered every
// export of that control with a roster burst. The A4EC refresh warns if a new DCS-BIOS release ever
// puts an aircraft here (tools/gen_a4ec, --modules-dir).
#define NODE_STATUS_REQ_ADDR      0xFF00
#define NODE_STATUS_MSG_NAME      "_NODE_STATUS"
#define NODE_STATUS_END_MSG_NAME  "_NODE_STATUS_END"

/**
 * @brief Node ID of the Gateway board's PanelBridge MCU — the CAN master.
 *
 * It sends HB_0 / HEALTH_0, and its own row leads every `_NODE_STATUS` roster reply (#344). CAN
 * nodes take 1–63 (Firmware/NODE_IDS.md).
 */
constexpr uint8_t GATEWAY_NODE_ID = 0;

/// `_NODE_STATUS` row length in bytes, sent as twice as many hex chars (proto v4: 17 → 34 hex).
constexpr uint8_t NODE_STATUS_ROW_BYTES = 17;

/**
 * @brief `_NODE_STATUS` row field offsets, in bytes of the decoded row (hex index = 2 × offset).
 *
 * Each field is its numeric value as fixed-width uppercase hex, most-significant byte first, so
 * the u16 fields read big-endian here although they are little-endian on CAN. A field runs to the
 * next offset; the last runs to NODE_STATUS_ROW_BYTES. The first six come from the node's HB_n
 * (plus `present`), the next five from its HEALTH_n, the last three from its READY_n (v4, #295).
 * Fields are only ever appended: a reader parses the fields it knows from a longer row and ignores
 * the rest. (Not `NODE_ID`: every node build defines that as a macro.)
 */
enum class NodeStatusField : uint8_t {
    ID           = 0,   // u8: node ID; GATEWAY_NODE_ID = the bridge's own row, 1-63 = CAN nodes
    PRESENT      = 1,   // u8: 01 alive, 00 removed
    HB_FLAGS     = 2,   // u8: HeartbeatFlag bits
    UPTIME       = 3,   // u16: seconds since boot, wraps at 65535
    RX_COUNT     = 5,   // u16: node-owned accepted RX count, wraps at 65535
    ESR          = 7,   // u16: TEC at HB_ESR_TEC_SHIFT, REC at HB_ESR_REC_SHIFT
    DIE_TEMP_C   = 9,   // i8: whole degC, DIE_TEMP_UNAVAILABLE = not seen
    HEALTH_FLAGS = 10,  // u8: NodeHealthFlag bits
    FAULT_MASK   = 11,  // u8: reserved, 00
    FAULT_ID     = 12,  // u8: NodeFaultCode
    NODE_KIND    = 13,  // u8: NodeKind
    FW_MAJOR     = 14,  // u8: firmware release version from the node's READY_n; 00 00 00 = not reported
    FW_MINOR     = 15,  // u8
    FW_PATCH     = 16,  // u8
};

// ── _NODE_TELEM DCS-BIOS host-reporting contract (#216) ───────────────────────
// PanelBridge forwards every TELEM_n CAN frame to the host unread, as one DCS-BIOS message per
// frame: `_NODE_TELEM <nodeId(2 hex)><8 frame bytes as 16 hex, wire order>` (18 hex chars). Pushed
// as frames arrive — no request, no cache, no end marker. Versioned separately from
// _NODE_STATUS: a new TelemPage is additive and needs no bump; bump on a change to the message
// framing or to an existing page's layout. Full page layouts: FirmwarePlan/02-can-protocol.md.
#define NODE_TELEM_PROTO_VERSION  1
#define NODE_TELEM_MSG_NAME       "_NODE_TELEM"

/// `_NODE_TELEM` row length in bytes, sent as twice as many hex chars (9 → 18 hex).
constexpr uint8_t NODE_TELEM_ROW_BYTES = 9;

/** @brief `_NODE_TELEM` row field offsets, in bytes of the decoded row (hex index = 2 × offset). */
enum class NodeTelemField : uint8_t {
    ID      = 0,  // u8: sending node ID (from the CAN ID)
    FRAME   = 1,  // 8 bytes: the TELEM_n frame unread, wire order (TelemPage at FRAME + 0)
};

/** @brief HB_n `flags` bits — CAN controller error state (maps to HeartbeatPayload.flags). */
enum class HeartbeatFlag : uint8_t {
    BOFF = 0x01,  // CAN controller is bus-off
    EPVF = 0x02,  // error passive: TEC or REC >= 128
};

/// HB_n `esr` field: bit position of the transmit error counter (TEC, low byte).
constexpr uint8_t HB_ESR_TEC_SHIFT = 0;
/// HB_n `esr` field: bit position of the receive error counter (REC, high byte).
constexpr uint8_t HB_ESR_REC_SHIFT = 8;

/** @brief HEALTH_n `flags` bits — node-level health conditions (maps to NodeHealthPayload.flags). */
enum class NodeHealthFlag : uint8_t {
    OVERHEAT = 0x01,  // die temp >= NODE_OVERHEAT_C (opt-in, #213)
    DEGRADED = 0x02,  // a FaultSource reports faultCode() != NodeFaultCode::NONE (#163)
};

/**
 * @brief HEALTH_n `dieTempC` meaning "unavailable / not yet seen" (`80` on the wire).
 *
 * Sent when the die sensor can't be read, and cached by PanelBridge for a node whose first
 * HEALTH_n hasn't arrived. Written as a literal so the client sync can read it; equal to INT8_MIN.
 */
constexpr int8_t DIE_TEMP_UNAVAILABLE = -128;
static_assert(DIE_TEMP_UNAVAILABLE == INT8_MIN, "DIE_TEMP_UNAVAILABLE is the int8 minimum");

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
    DUPLICATE_NODE_ID = 0x09,  // another board transmits on this node's HB_n — two boards share a NODE_ID (#344)
    // 0x0A–0xFF reserved for future fault sources
};

/**
 * @brief HEALTH_n `nodeKind` (byte 5) — what kind of board this node is (#344).
 *
 * Announced by the board's own firmware role (each node library passes its kind to
 * CANProtocol::serviceNodeStatus()), never derived from the NODE_ID — a PDU can take any ID.
 * Append-only. 0x00 is what firmware older than #344 sends (the byte was reserved), so the client
 * reads it as "unknown" and never adopts on it.
 */
enum class NodeKind : uint8_t {
    UNKNOWN     = 0x00,  // pre-#344 firmware, or no HEALTH_n seen yet
    PANEL_GROUP = 0x01,  // PanelGroup node (PanelGroup library)
    PDU         = 0x02,  // power distribution / monitoring node (PowerMonitor library)
    BRIDGE      = 0x03,  // the SimGateway + PanelBridge board — reported by its PanelBridge MCU (node 0)
    // 0x04–0xFF reserved for future node types
};

/**
 * @brief TELEM_n page ids (#216) — byte 0 of TelemPayload, naming the layout of bytes 1-7.
 *
 * Global and append-only: a page id means the same layout whichever node sends it, so the client
 * decodes by page, not by node type. 0x00 is invalid. Each page's layout is its field-offset enum
 * below (PDU_RAIL_* → PduRailField, PDU_STATUS → PduStatusField); multi-byte fields little-endian.
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

/**
 * @brief PDU_RAIL_12V / PDU_RAIL_5V field offsets, in bytes of the 8-byte TELEM_n frame.
 *
 * Byte 0 is the TelemPage; a field runs to the next offset, the last to byte 7. Little-endian.
 */
enum class PduRailField : uint8_t {
    FLAGS    = 1,  // u8: PduRailFlag bits
    V_AVG_MV = 2,  // u16: window-average rail voltage, mV
    V_MIN_MV = 4,  // u16: window-minimum rail voltage, mV
    I_AVG_MA = 6,  // int16: window-average rail current, mA (signed: zero offset stays visible)
};

/**
 * @brief PDU_STATUS field offsets, in bytes of the 8-byte TELEM_n frame.
 *
 * Byte 0 is the TelemPage; a field runs to the next offset, the last to byte 7. Little-endian.
 */
enum class PduStatusField : uint8_t {
    FLAGS       = 1,  // u8: PduStatusFlag bits
    NTC_TEMP_DC = 2,  // int16: board temperature, 0.1 degC (NTC_TEMP_UNAVAILABLE = no reading)
    I12_PEAK_MA = 4,  // u16: 12 V rail peak current since the last page, mA
    I5_PEAK_MA  = 6,  // u16: 5 V rail peak current since the last page, mA
};

/**
 * @brief PDU_STATUS `ntcTempDC` meaning "unavailable" — no reading yet, or the NTC is open/shorted.
 *
 * `0x8000` on the wire. Written as a literal so the client sync can read it; equal to INT16_MIN.
 */
constexpr int16_t NTC_TEMP_UNAVAILABLE = -32768;
static_assert(NTC_TEMP_UNAVAILABLE == INT16_MIN, "NTC_TEMP_UNAVAILABLE is the int16 minimum");

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
