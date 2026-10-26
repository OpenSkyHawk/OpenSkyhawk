

# File PanelBridge.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelBridge**](dir_f592a3c441b32532ba8eb6b28add2a90.md) **>** [**PanelBridge.cpp**](PanelBridge_8cpp.md)

[Go to the documentation of this file](PanelBridge_8cpp.md)


```C++
// DcsBios.h defines tryToSendDcsBiosMessage only under DCSBIOS_DEFAULT_SERIAL.
// Pre-declare it so the inline sendDcsBiosMessage() in DcsBios.h compiles without the flag.
// The definition is provided by the sketch TU (compiled with -DDCSBIOS_DEFAULT_SERIAL).
namespace DcsBios {
    bool tryToSendDcsBiosMessage(const char* msg, const char* arg);
}

// Include DcsBios.h WITHOUT DCSBIOS_DEFAULT_SERIAL to avoid ODR violations.
// The sketch TU owns ProtocolParser, DcsBios::setup(), DcsBios::loop(), and the definition
// of tryToSendDcsBiosMessage(). This TU only needs the types and constants.
#ifdef DCSBIOS_DEFAULT_SERIAL
#  undef DCSBIOS_DEFAULT_SERIAL
#  define _PBRIDGE_RESTORE_DCSBIOS
#endif
#include <DcsBios.h>
#ifdef _PBRIDGE_RESTORE_DCSBIOS
#  define DCSBIOS_DEFAULT_SERIAL
#  undef _PBRIDGE_RESTORE_DCSBIOS
#endif

#ifdef ARDUINO_ARCH_STM32

#include "PanelBridge.h"
#include <CANProtocol.h>   // CAN types + NodeHealthPayload
#include <NodeStatus.h>    // NODE_STATUS_REQ_ADDR / NODE_STATUS_MSG_NAME (node-status reporting, #86)
#include <STM32Board.h>
#include <FirmwareVersion.h>  // OPENSKYHAWK_FW_* — the bridge's own node-0 row version (#295)
#include <FirmwareImage.h>    // the image identity block (#362)
#include <BootProtocol.h>     // relaying bootloader frames to and from the Gateway (#365)
#include <A4EC_InputMap.h>
#include <string.h>

// This image's identity block — the bootloader and the uploader check it (#362).
OSH_FIRMWARE_IDENTITY(NodeKind::BRIDGE);

namespace {

// ── Node tracking ─────────────────────────────────────────────────────────────

static constexpr uint8_t  MAX_NODE_ID         = 63;
static constexpr uint32_t HB_TIMEOUT_MS       = 3000;

struct NodeState {
    bool     alive;
    bool     everSeen;
    uint32_t lastSeenMs;
#if PANELBRIDGE_NODE_STATUS
    HeartbeatPayload last;          // last HB payload, surfaced to the host (#86)
    // Cached NodeHealthPayload fields from the node's last HEALTH_n frame (#221 contract):
    int8_t   dieTempC    = DIE_TEMP_UNAVAILABLE; // internal die temp °C, unseen until HEALTH_n — #213
    uint8_t  healthFlags = 0;        // overheat (#213) / degraded (#163) bits
    uint8_t  faultMask   = 0;        // fault source/domain bitmap — reserved #163
    uint8_t  faultId     = 0;        // NodeFaultCode (NodeStatus.h) — #163
    uint8_t  nodeKind    = 0;        // NodeKind (NodeStatus.h), 0 = unseen/unknown — #344
    // Firmware version from the node's last READY_n (#295); 0.0.0 = not reported (no READY seen
    // since the bridge booted, or an empty READY from older firmware). Kept across timeouts.
    uint8_t  fwMajor     = 0;
    uint8_t  fwMinor     = 0;
    uint8_t  fwPatch     = 0;
    // Bootloader (#365): its protocol version (READY_n byte 3, or its own announcement; 0 = none),
    // NODE_BOOT_STATE_APP while the app runs or the BootState it announces, and when update traffic
    // for the node last passed through here (→ NODE_BOOT_STATE_UPDATING).
    uint8_t  bootloader  = 0;
    uint8_t  bootState   = NODE_BOOT_STATE_APP;
    bool     updating    = false;
    uint32_t lastBootMs  = 0;
#endif
};

static NodeState      _nodes[MAX_NODE_ID] = {};   // index = nodeId - 1
static void (*_cbAlive)(uint8_t)          = nullptr;
static void (*_cbDead)(uint8_t)           = nullptr;
static uint8_t        _deadCount          = 0;    // tracked nodes currently timed-out (drives WARNING)

#if PANELBRIDGE_NODE_STATUS
// ── Node-status reporting (#86) ──────────────────────────────────────────────
// Surface node presence + health to the host (OpenSkyhawk Client) as DCS-BIOS
// command messages on a reserved control name. Owned entirely by PanelBridge;
// SimGateway relays the ASCII verbatim. See NodeStatus.h for the wire format.

// Emit one status row (proto v4, NODE_STATUS_ROW_BYTES = 19 bytes as 38 hex chars):
//   _NODE_STATUS <nodeId present flags uptime rxCount esr dieTempC hFlags faultMask faultId nodeKind
//                 fwMajor fwMinor fwPatch bootloader bootState>
// The next five come from the node's HEALTH_n frame (#221 contract), the next three from its READY_n
// (#295; 000000 = not reported), the last two from its bootloader (#365). DIE_TEMP_UNAVAILABLE + nodeKind
// 00 = not-yet-seen; hFlags (DEGRADED) + faultId are populated per node by aggregateFaults (#163);
// faultMask stays 0 (reserved for future fault-domain bits); nodeKind is the board kind (#344).
// The format string is the row layout: each field's width must match NodeStatusField (NodeStatus.h).
using NSF = NodeStatusField;
static_assert((uint8_t)NSF::ID == 0 && (uint8_t)NSF::PRESENT == 1 && (uint8_t)NSF::HB_FLAGS == 2
              && (uint8_t)NSF::UPTIME == 3 && (uint8_t)NSF::RX_COUNT == 5 && (uint8_t)NSF::ESR == 7
              && (uint8_t)NSF::DIE_TEMP_C == 9 && (uint8_t)NSF::HEALTH_FLAGS == 10
              && (uint8_t)NSF::FAULT_MASK == 11 && (uint8_t)NSF::FAULT_ID == 12
              && (uint8_t)NSF::NODE_KIND == 13 && (uint8_t)NSF::FW_MAJOR == 14
              && (uint8_t)NSF::FW_MINOR == 15 && (uint8_t)NSF::FW_PATCH == 16
              && (uint8_t)NSF::BOOTLOADER == 17 && (uint8_t)NSF::BOOT_STATE == 18
              && NODE_STATUS_ROW_BYTES == 19,
              "NodeStatusField moved: update emitRow's snprintf format to match");
static void emitRow(uint8_t nodeId, bool present, const HeartbeatPayload& hb,
                    int8_t dieTempC, uint8_t hFlags, uint8_t faultMask, uint8_t faultId,
                    uint8_t nodeKind, uint8_t fwMajor, uint8_t fwMinor, uint8_t fwPatch,
                    uint8_t bootloader, uint8_t bootState) {
    char hex[2 * NODE_STATUS_ROW_BYTES + 1];
    snprintf(hex, sizeof(hex), "%02X%02X%02X%04X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
             (unsigned)nodeId, present ? 1u : 0u, (unsigned)hb.flags,
             (unsigned)hb.uptime, (unsigned)hb.rxCount, (unsigned)hb.esr,
             (unsigned)(uint8_t)dieTempC, (unsigned)hFlags, (unsigned)faultMask,
             (unsigned)faultId, (unsigned)nodeKind,
             (unsigned)fwMajor, (unsigned)fwMinor, (unsigned)fwPatch,
             (unsigned)bootloader, (unsigned)bootState);
    DcsBios::sendDcsBiosMessage(NODE_STATUS_MSG_NAME, hex);
}

// One CAN node's row (nodeId 1–63), from its cached HB_n + HEALTH_n.
static void emitNode(uint8_t nodeId, bool present) {
    const NodeState& ns = _nodes[nodeId - 1];
    emitRow(nodeId, present, ns.last, ns.dieTempC, ns.healthFlags, ns.faultMask, ns.faultId,
            ns.nodeKind, ns.fwMajor, ns.fwMinor, ns.fwPatch, ns.bootloader,
            ns.bootState | (ns.updating ? NODE_BOOT_STATE_UPDATING : 0));
}

// The bridge's own row, GATEWAY_NODE_ID (#344): its heartbeat state now, and the HEALTH_0 it last
// sent (kind BRIDGE once the first HEALTH_0 has gone out), and the version this build carries (#295).
// Always present — it is the one reporting.
static void emitSelf() {
    const HeartbeatPayload hb = CANProtocol::makeHeartbeatPayload(GATEWAY_NODE_ID, 0);
    const NodeHealthPayload& h = CANProtocol::lastNodeHealth();
    emitRow(GATEWAY_NODE_ID, true, hb, h.dieTempC, h.flags, h.faultMask, h.faultId, h.nodeKind,
            OPENSKYHAWK_FW_MAJOR, OPENSKYHAWK_FW_MINOR, OPENSKYHAWK_FW_PATCH,
            STM32Board::bootloaderVersion(), NODE_BOOT_STATE_APP);
}

// Emit the full roster (request response / boot seed): the bridge's own node-0 row, one
// _NODE_STATUS per alive node, then _NODE_STATUS_END <count> so the host knows the burst is
// complete and can reconcile (prune nodes absent from it). count includes the node-0 row, so
// count=1 = bridge only, no other nodes connected.
static void emitAllNodes() {
    emitSelf();
    uint8_t count = 1;
    for (uint8_t i = 0; i < MAX_NODE_ID; i++)
        if (_nodes[i].alive) { emitNode(i + 1, true); count++; }
    char arg[4];
    snprintf(arg, sizeof(arg), "%u", (unsigned)count);
    DcsBios::sendDcsBiosMessage(NODE_STATUS_END_MSG_NAME, arg);
}

// Forward one TELEM_n frame to the host, unread (#216):
//   _NODE_TELEM <nodeId(2 hex)><8 frame bytes, wire order, 16 hex>
// No cache and no decode — the page byte tells the client the layout, so a new TelemPage needs no
// bridge change. Contract: NODE_TELEM_* + NodeTelemField in NodeStatus.h.
static_assert((uint8_t)NodeTelemField::ID == 0 && (uint8_t)NodeTelemField::FRAME == 1
              && NODE_TELEM_ROW_BYTES == 1 + sizeof(TelemPayload),
              "NodeTelemField moved: update emitTelem's snprintf format to match");
static void emitTelem(uint8_t nodeId, const uint8_t* data) {
    char hex[2 * NODE_TELEM_ROW_BYTES + 1];
    snprintf(hex, sizeof(hex), "%02X%02X%02X%02X%02X%02X%02X%02X%02X",
             (unsigned)nodeId, data[0], data[1], data[2], data[3],
             data[4], data[5], data[6], data[7]);
    DcsBios::sendDcsBiosMessage(NODE_TELEM_MSG_NAME, hex);
}
#endif // PANELBRIDGE_NODE_STATUS

// ── TEST_SEQ state ─────────────────────────────────────────────────────────────

static uint16_t _testSeqNum    = 0;
static uint32_t _testSeqSentMs = 0;

// ── DCS session change ─────────────────────────────────────────────────────────

static int32_t _lastModelTimeSec = -1;  // -1 = unseeded

// ── Helpers ────────────────────────────────────────────────────────────────────

static void broadcastSyncReq() {
    static const uint8_t empty[1] = {};
    CANProtocol::send(CAN_ID_SYNC_REQ, empty, 0);
    STM32Board::log("[BRIDGE] SYNC_REQ broadcast");
}

static void sendHidFrame(uint16_t controlId, uint16_t value) {
    uint8_t frame[6] = {
        0xAA, 0x55,
        (uint8_t)(controlId & 0xFF), (uint8_t)(controlId >> 8),
        (uint8_t)(value     & 0xFF), (uint8_t)(value     >> 8)
    };
    Serial.write(frame, 6);
}

// Internal helper called by BridgeExportListener — separated for testability.
void handleDcsBiosExport(uint16_t address, uint16_t value) {
    if (address == 0x0000) return;   // the ControlPacket empty-slot sentinel; never carried
#if PANELBRIDGE_NODE_STATUS
    // Reserved: the host's node-status request — handled by NodeStatusReqListener,
    // never broadcast onto CAN.
    if (address == NODE_STATUS_REQ_ADDR) return;
#endif
    // Reserved: the bootloader relay window (#365) — handled by BootRelayListener.
    if (address >= OpenSkyhawk::Boot::RELAY_EXPORT_ADDRESS
        && address < OpenSkyhawk::Boot::RELAY_EXPORT_ADDRESS + 2 * OpenSkyhawk::Boot::RELAY_EXPORT_WORDS) return;
    // A real DCS-BIOS export → data is flowing → CONNECTED (green solid). The reserved
    // node-status poll above is excluded so it does not masquerade as live DCS data.
    STM32Board::setLinkActive(true);
    ControlPacket pkt;
    pkt.controlId = address;
    pkt.value     = value;
    CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, pkt);
}

// Binary search A4EC_INPUT_MAP (sorted ascending by cmdId) -> entry, or nullptr.
static const DcsBiosInputEntry* lookupDcsEntry(uint16_t controlId) {
    int lo = 0, hi = (int)A4EC_INPUT_MAP_SIZE - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if      (A4EC_INPUT_MAP[mid].cmdId == controlId) return &A4EC_INPUT_MAP[mid];
        else if (A4EC_INPUT_MAP[mid].cmdId <  controlId)   lo = mid + 1;
        else                                                hi = mid - 1;
    }
    return nullptr;
}

#ifdef PANELBRIDGE_TEST
// Counts DCS-BIOS commands actually put on the wire — not drops, not unknown-controlId. A test
// that only inspects the diag log cannot tell "routed" from "logged and discarded"; this lets one
// assert the command was emitted.
static uint32_t _testDcsSends = 0;
#endif

// Resolve controlId -> DCS-BIOS name and send it with the given arg string. The dispatch FORM
// (the arg) is decided by the caller from the CAN frame the value arrived on (#147); the map
// carries only the name.
static void sendDcs(uint16_t controlId, const char* arg) {
    const DcsBiosInputEntry* entry = lookupDcsEntry(controlId);
    if (!entry) {
        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[BRIDGE] unknown DCS ctrl=0x")); d.println(controlId, HEX);
        }
        return;
    }
    DcsBios::sendDcsBiosMessage(entry->name, arg);
#ifdef PANELBRIDGE_TEST
    ++_testDcsSends;
#endif
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[BRIDGE] DCS -> \"")); d.print(entry->name);
        d.print(F("\" \"")); d.print(arg); d.println('"');
    }
}

// ABS frame (canIdEvt): value is an absolute uint16 -> set_state. Switches, selectors, pots.
static void dispatchDcsInput(uint16_t controlId, uint16_t value) {
    char buf[7];                                        // "65535\0" + guard
    snprintf(buf, sizeof(buf), "%u", (unsigned)value);
    sendDcs(controlId, buf);
}

// REL frame (canIdEvtRel): value is a signed +-step -> variable_step (e.g. "+3200").
static void dispatchDcsRel(uint16_t controlId, uint16_t value) {
    char buf[8];                                        // "-32768\0" + guard
    snprintf(buf, sizeof(buf), "%+d", (int)(int16_t)value);
    sendDcs(controlId, buf);
}

// DIR frame (canIdEvtDir): value is strictly +-1 -> fixed_step (INC / DEC). Anything else is a
// malformed slot -> drop + log, so a corrupt value never turns into a real cockpit step.
static void dispatchDcsDir(uint16_t controlId, uint16_t value) {
    const int16_t v = (int16_t)value;
    if      (v ==  1) sendDcs(controlId, "INC");
    else if (v == -1) sendDcs(controlId, "DEC");
    else if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[BRIDGE] bad dir ctrl=0x")); d.print(controlId, HEX);
        d.print(F(" val=")); d.println(v);
    }
}

// ACTION frame (canIdEvtAction): value is a selector, not a magnitude -> action (TOGGLE).
// 0 is the only defined selector; anything else is a malformed slot -> drop + log, so a corrupt
// value never flips a real cockpit switch.
static void dispatchDcsAction(uint16_t controlId, uint16_t value) {
    if (value == 0) sendDcs(controlId, "TOGGLE");
    else if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[BRIDGE] bad action ctrl=0x")); d.print(controlId, HEX);
        d.print(F(" val=")); d.println(value);
    }
}

static void dispatchEvtSlot(uint16_t controlId, uint16_t value) {
    if (controlId == 0x0000)                                            return;  // null sentinel
    if (controlId >= CTRL_ID_DCS_MIN && controlId <= CTRL_ID_DCS_MAX)
        dispatchDcsInput(controlId, value);
    else if (controlId < CTRL_ID_DCS_MIN)
        sendHidFrame(controlId, value);
    else {
        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[BRIDGE] drop ctrl=0x")); d.println(controlId, HEX);
        }
    }
}

// Relative (canIdEvtRel) slot — DCS-routed only; value reinterpreted signed -> %+d (variable_step).
static void dispatchRelSlot(uint16_t controlId, uint16_t value) {
    if (controlId == 0x0000) return;                                    // null sentinel
    if (controlId >= CTRL_ID_DCS_MIN && controlId <= CTRL_ID_DCS_MAX)
        dispatchDcsRel(controlId, value);
    else if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[BRIDGE] drop rel ctrl=0x")); d.println(controlId, HEX);
    }
}

// Directional (canIdEvtDir) slot — DCS-routed only; value +-1 -> INC/DEC (fixed_step).
static void dispatchDirSlot(uint16_t controlId, uint16_t value) {
    if (controlId == 0x0000) return;                                    // null sentinel
    if (controlId >= CTRL_ID_DCS_MIN && controlId <= CTRL_ID_DCS_MAX)
        dispatchDcsDir(controlId, value);
    else if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[BRIDGE] drop dir ctrl=0x")); d.println(controlId, HEX);
    }
}

// Action (canIdEvtAction) slot — DCS-routed only; value 0 -> TOGGLE (action).
static void dispatchActionSlot(uint16_t controlId, uint16_t value) {
    if (controlId == 0x0000) return;                                    // null sentinel
    if (controlId >= CTRL_ID_DCS_MIN && controlId <= CTRL_ID_DCS_MAX)
        dispatchDcsAction(controlId, value);
    else if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[BRIDGE] drop action ctrl=0x")); d.println(controlId, HEX);
    }
}

// Update liveness state and fire alive callback on dead/unseen → alive transition.
// Returns true if this call caused a transition (caller responsible for SYNC_REQ).
static bool markNodeAlive(uint8_t nodeId, uint32_t now) {
    uint8_t idx     = nodeId - 1;
    bool wasAlive   = _nodes[idx].alive;
    bool wasDead    = _nodes[idx].everSeen && !_nodes[idx].alive;  // timed-out, not just unseen
    _nodes[idx].alive      = true;
    _nodes[idx].everSeen   = true;
    _nodes[idx].lastSeenMs = now;
    if (!wasAlive) {
        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[BRIDGE] node=")); d.print(nodeId); d.println(F(" alive"));
        }
        // A previously-dead node recovered — clear WARNING once none remain dead.
        if (wasDead && _deadCount > 0 && --_deadCount == 0)
            STM32Board::setWarning(false);
        if (_cbAlive) _cbAlive(nodeId);
#if PANELBRIDGE_NODE_STATUS
        emitNode(nodeId, true);   // push presence on dead/unseen → alive (#86)
#endif
        return true;
    }
    return false;
}

// ── Bootloader relay (#365) ───────────────────────────────────────────────────

// Update traffic for node n passed through here: UPDATING on, pushed as a delta when it starts.
static void noteBootTraffic(uint8_t nodeId, uint32_t now) {
#if PANELBRIDGE_NODE_STATUS
    NodeState& ns = _nodes[nodeId - 1];
    ns.lastBootMs = now;
    if (!ns.updating) {
        ns.updating = true;
        if (ns.alive) emitNode(nodeId, true);
    }
#else
    (void)nodeId; (void)now;
#endif
}

// A bootloader frame from the bus → the host, as `_BOOT_RSP <id(3)><len(1)><data hex>`.
static void emitBootRsp(uint32_t canId, const uint8_t* data, uint8_t len) {
    char hex[3 + 1 + 16 + 1];
    int n = snprintf(hex, sizeof(hex), "%03X%X", (unsigned)canId, (unsigned)len);
    for (uint8_t i = 0; i < len && i < 8; i++) n += snprintf(hex + n, sizeof(hex) - n, "%02X", data[i]);
    DcsBios::sendDcsBiosMessage(BOOT_RSP_MSG_NAME, hex);
}

// A node's bootloader spoke: relay it, and keep the node alive in its bootloader state — no
// false-offline, no WARNING while it is being flashed. No SYNC_REQ: a bootloader takes none.
static void onBootRsp(uint32_t canId, const uint8_t* data, uint8_t len, uint32_t now) {
    using namespace OpenSkyhawk::Boot;
    emitBootRsp(canId, data, len);
    if (canId == UNCONFIGURED_RSP_ID) return;   // no NODE_ID yet: nothing to track
    const uint8_t nodeId = (uint8_t)(canId - RSP_ID_BASE);
#if PANELBRIDGE_NODE_STATUS
    NodeState& ns = _nodes[nodeId - 1];
    bool changed = false;
    if (len > (uint8_t)AnnounceField::STATE
        && (data[0] == (uint8_t)Reply::ANNOUNCE || data[0] == (uint8_t)Reply::CONNECTED)) {
        changed = data[(uint8_t)AnnounceField::PROTOCOL] != ns.bootloader
               || data[(uint8_t)AnnounceField::STATE] != ns.bootState;
        ns.bootloader = data[(uint8_t)AnnounceField::PROTOCOL];
        ns.bootState  = data[(uint8_t)AnnounceField::STATE];
    } else if (len >= 2 && data[0] == (uint8_t)Reply::BEGUN && data[1] == (uint8_t)Status::OK) {
        // BEGIN erased the old app — say so now, not at the next announcement.
        changed = ns.bootState != (uint8_t)BootState::APP_INVALID;
        ns.bootState = (uint8_t)BootState::APP_INVALID;
    }
    if (data[0] != (uint8_t)Reply::ANNOUNCE) noteBootTraffic(nodeId, now);   // a reply: a session is on
    if (!markNodeAlive(nodeId, now) && changed) emitNode(nodeId, true);
#else
    markNodeAlive(nodeId, now);
#endif
}

static void onCanRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    uint32_t now = millis();

    // BOOT_RSP_1 – BOOT_RSP_63 + the unconfigured announce: a bootloader talking (#365).
    if ((canId >= OpenSkyhawk::Boot::canIdBootRsp(1) && canId <= OpenSkyhawk::Boot::canIdBootRsp(MAX_NODE_ID))
        || canId == OpenSkyhawk::Boot::UNCONFIGURED_RSP_ID) {
        onBootRsp(canId, data, len, now);
        return;
    }

    // HB_1 – HB_63: update liveness; broadcast SYNC_REQ only on dead→alive transition
    if (canId >= canIdHb(1) && canId <= canIdHb(MAX_NODE_ID)) {
        uint8_t nodeId = (uint8_t)(canId - canIdHb(0));
#if PANELBRIDGE_NODE_STATUS
        NodeState& ns = _nodes[nodeId - 1];
        if (len >= 8) memcpy(&ns.last, data, 8);  // cache health for #86 reporting
        // A heartbeat comes from the app: the node has left its bootloader (#365).
        const bool leftBoot = ns.bootState != NODE_BOOT_STATE_APP || ns.updating;
        ns.bootState = NODE_BOOT_STATE_APP;
        ns.updating  = false;
        if (markNodeAlive(nodeId, now)) broadcastSyncReq();
        else if (leftBoot) emitNode(nodeId, true);
#else
        if (markNodeAlive(nodeId, now)) broadcastSyncReq();
#endif
        return;
    }

    // HEALTH_1 – HEALTH_63: cache internal die temp + health/fault fields for host reporting (#213).
    // Cache only — HB owns liveness/SYNC, so a health frame never triggers markNodeAlive.
    if (canId >= canIdHealth(1) && canId <= canIdHealth(MAX_NODE_ID)) {
#if PANELBRIDGE_NODE_STATUS
        if (len >= sizeof(NodeHealthPayload)) {
            uint8_t nodeId = (uint8_t)(canId - canIdHealth(0));
            NodeHealthPayload h;
            memcpy(&h, data, sizeof(h));
            NodeState& ns = _nodes[nodeId - 1];
            ns.dieTempC    = h.dieTempC;
            ns.healthFlags = h.flags;
            ns.faultMask   = h.faultMask;
            ns.faultId     = h.faultId;
            // Board kind (#344): push a live delta when it changes — the alive delta went out on
            // the node's first HB, before any HEALTH_n, with kind 00.
            if (h.nodeKind != ns.nodeKind) {
                ns.nodeKind = h.nodeKind;
                if (ns.alive) emitNode(nodeId, true);
            }
        }
#endif
        return;
    }

    // TELEM_1 – TELEM_63: paged node telemetry — forwarded to the host unread (#216). Like HEALTH,
    // never a liveness signal (HB owns that).
    if (canId >= canIdTelem(1) && canId <= canIdTelem(MAX_NODE_ID)) {
#if PANELBRIDGE_NODE_STATUS
        if (len == sizeof(TelemPayload) && data[0] != 0)
            emitTelem((uint8_t)(canId - canIdTelem(0)), data);
#endif
        return;
    }

    // EVT_1 – EVT_63: 8-byte ControlPacketPair, dispatch each non-null slot
    if (canId >= canIdEvt(1) && canId <= canIdEvt(MAX_NODE_ID)) {
        if (len < 8) return;
        ControlPacketPair pair;
        memcpy(&pair, data, 8);
        dispatchEvtSlot(pair.a.controlId, pair.a.value);
        if (pair.b.controlId != 0x0000)
            dispatchEvtSlot(pair.b.controlId, pair.b.value);
        return;
    }

    // EVT_REL_1 – EVT_REL_63: 8-byte ControlPacketPair; value reinterpreted signed -> %+d (variable_step)
    if (canId >= canIdEvtRel(1) && canId <= canIdEvtRel(MAX_NODE_ID)) {
        if (len < 8) return;
        ControlPacketPair pair;
        memcpy(&pair, data, 8);
        dispatchRelSlot(pair.a.controlId, pair.a.value);
        if (pair.b.controlId != 0x0000)
            dispatchRelSlot(pair.b.controlId, pair.b.value);
        return;
    }

    // EVT_DIR_1 – EVT_DIR_63: 8-byte ControlPacketPair; value +-1 -> INC/DEC (fixed_step)
    if (canId >= canIdEvtDir(1) && canId <= canIdEvtDir(MAX_NODE_ID)) {
        if (len < 8) return;
        ControlPacketPair pair;
        memcpy(&pair, data, 8);
        dispatchDirSlot(pair.a.controlId, pair.a.value);
        if (pair.b.controlId != 0x0000)
            dispatchDirSlot(pair.b.controlId, pair.b.value);
        return;
    }

    // EVT_ACTION_1 – EVT_ACTION_63: 8-byte ControlPacketPair; value 0 -> TOGGLE (action)
    if (canId >= canIdEvtAction(1) && canId <= canIdEvtAction(MAX_NODE_ID)) {
        if (len < 8) return;
        ControlPacketPair pair;
        memcpy(&pair, data, 8);
        dispatchActionSlot(pair.a.controlId, pair.a.value);
        if (pair.b.controlId != 0x0000)
            dispatchActionSlot(pair.b.controlId, pair.b.value);
        return;
    }

    // ECHO_1 – ECHO_63: consume TEST_SEQ RTT response, log locally
    if (canId >= canIdEcho(1) && canId <= canIdEcho(MAX_NODE_ID)) {
        if (!STM32Board::isDebug() || len < 8) return;
        uint16_t seq;    memcpy(&seq,    data,     2);
        uint32_t sentMs; memcpy(&sentMs, data + 2, 4);
        uint32_t rtt = now - sentMs;
        auto& d = STM32Board::diagSerial();
        d.print(F("[BRIDGE] ECHO node=")); d.print((uint8_t)(canId - canIdEcho(0)));
        d.print(F(" seq="));              d.print(seq);
        d.print(F(" rtt="));              d.print(rtt); d.println(F("ms"));
        return;
    }

    // READY_1 – READY_63: node boot-complete — mark alive, always broadcast SYNC_REQ.
    // SYNC_REQ is unconditional: READY signals a fresh boot regardless of prior state.
    if (canId >= canIdReady(1) && canId <= canIdReady(MAX_NODE_ID)) {
        uint8_t nodeId = (uint8_t)(canId - canIdReady(0));
        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[BRIDGE] READY node=")); d.println(nodeId);
        }
#if PANELBRIDGE_NODE_STATUS
        // Firmware version (#295): every boot re-announces it, so a reflash is seen at once. Cache
        // it before markNodeAlive() so an alive delta already carries it; a node that rebooted
        // inside the heartbeat timeout stays alive, so push the change as its own delta. An empty
        // READY is older firmware: not reported.
        NodeState& ns = _nodes[nodeId - 1];
        ReadyPayload r = {};
        if (len >= 3) memcpy(&r, data, len < sizeof(r) ? len : sizeof(r));
        const bool fwChanged = r.fwMajor != ns.fwMajor || r.fwMinor != ns.fwMinor
                             || r.fwPatch != ns.fwPatch || r.bootloader != ns.bootloader
                             || ns.bootState != NODE_BOOT_STATE_APP;
        ns.fwMajor = r.fwMajor;
        ns.fwMinor = r.fwMinor;
        ns.fwPatch = r.fwPatch;
        ns.bootloader = r.bootloader;   // READY byte 3 (#365); 0 from firmware without it
        ns.bootState  = NODE_BOOT_STATE_APP;
        ns.updating   = false;
        if (!markNodeAlive(nodeId, now) && fwChanged) emitNode(nodeId, true);
#else
        markNodeAlive(nodeId, now);
#endif
        broadcastSyncReq();
        return;
    }
    // All other IDs discarded silently
}

static void checkNodeTimeouts(uint32_t now) {
    for (uint8_t i = 0; i < MAX_NODE_ID; i++) {
#if PANELBRIDGE_NODE_STATUS
        // Update traffic stopped (#365): UPDATING off — a board left in its bootloader now reads as
        // waiting, which the host shows as stuck when it is APP_INVALID / APP_UNSTABLE.
        if (_nodes[i].updating && (int32_t)(now - _nodes[i].lastBootMs) > (int32_t)NODE_BOOT_UPDATING_MS) {
            _nodes[i].updating = false;
            if (_nodes[i].alive) emitNode(i + 1, true);
        }
#endif
        if (!_nodes[i].alive) continue;
        // Signed elapsed: if `now` is (slightly) behind lastSeenMs — e.g. a millisecond ticked
        // between sampling `now` and onCanRx stamping lastSeenMs during the same loop — the unsigned
        // difference would wrap to ~4.29e9 and trip a false timeout. Cast to signed so a small
        // negative reads as "not yet elapsed", not "long overdue" (#225).
        if ((int32_t)(now - _nodes[i].lastSeenMs) > (int32_t)HB_TIMEOUT_MS) {
            _nodes[i].alive = false;
            uint8_t nodeId  = i + 1;
            if (STM32Board::isDebug()) {
                auto& d = STM32Board::diagSerial();
                d.print(F("[BRIDGE] node=")); d.print(nodeId); d.println(F(" dead"));
            }
            _deadCount++;                   // a tracked node died → WARNING (red/green alternating)
            STM32Board::setWarning(true);
            if (_cbDead) _cbDead(nodeId);
#if PANELBRIDGE_NODE_STATUS
            emitNode(nodeId, false);   // push removal on alive → dead (#86)
#endif
        }
    }
}

static void onModelTimeChange(char* value) {
    int32_t sec = 0;
    for (uint8_t i = 0; i < 5 && value[i] != '\0'; i++) {
        if (value[i] == '.') break;
        if (value[i] >= '0' && value[i] <= '9')
            sec = sec * 10 + (value[i] - '0');
    }
    if (_lastModelTimeSec < 0) {
        _lastModelTimeSec = sec;  // seed — no SYNC_REQ on first value
        return;
    }
    if (sec < _lastModelTimeSec) {
        STM32Board::log("[BRIDGE] DCS session change — SYNC_REQ");
        broadcastSyncReq();
    }
    _lastModelTimeSec = sec;
}

// DCS-BIOS export listener — forwards the whole export stream to CTRL_BCAST (#295): every address
// of whatever aircraft is flying, plus CommonData and the metadata, so the nodes decide what they
// use. It starts at 0x0002 because a ControlPacket's controlId 0x0000 is the empty-slot sentinel —
// the one address CAN can't carry (the first two characters of MetadataStart's _ACFT_NAME).
// ExportStreamListener::firstExportStreamListener (Protocol.cpp) is zero-initialized
// before any constructors run, so file-scope registration is safe.
static constexpr uint16_t EXPORT_FIRST_ADDR = 0x0002;
static constexpr uint16_t EXPORT_LAST_ADDR  = 0xFFFE;

class BridgeExportListener : public DcsBios::ExportStreamListener {
public:
    BridgeExportListener() : DcsBios::ExportStreamListener(EXPORT_FIRST_ADDR, EXPORT_LAST_ADDR) {}
    void onDcsBiosWrite(unsigned int address, unsigned int data) override {
        handleDcsBiosExport((uint16_t)address, (uint16_t)data);
    }
};

static BridgeExportListener     _exportListener;
static DcsBios::StringBuffer<5> _modelTimeBuf(CommonData_MOD_TIME_A, onModelTimeChange);

#if PANELBRIDGE_NODE_STATUS
// Host roster request (#86): the client writes NODE_STATUS_REQ_ADDR on the DCS-BIOS
// export stream; PanelBridge replies with the full roster. handleDcsBiosExport()
// drops this address so it is never broadcast onto CAN.
class NodeStatusReqListener : public DcsBios::ExportStreamListener {
public:
    NodeStatusReqListener()
        : DcsBios::ExportStreamListener(NODE_STATUS_REQ_ADDR, NODE_STATUS_REQ_ADDR) {}
    void onDcsBiosWrite(unsigned int, unsigned int) override { emitAllNodes(); }
};
static NodeStatusReqListener _nodeStatusReqListener;
#endif

// Host → bootloader relay (#365): one export record per CAN frame in the reserved window — id,
// length, then one data byte per word (BootProtocol.h says why) — put on the bus once the record's
// last word arrives. Only BOOT_CMD_1-63 and
// the unconfigured command ID pass: the host can't inject anything else onto CAN.
class BootRelayListener : public DcsBios::ExportStreamListener {
public:
    BootRelayListener()
        : DcsBios::ExportStreamListener(OpenSkyhawk::Boot::RELAY_EXPORT_ADDRESS,
                                        OpenSkyhawk::Boot::RELAY_EXPORT_ADDRESS + 2 * (OpenSkyhawk::Boot::RELAY_EXPORT_WORDS - 1)) {}
    void onDcsBiosWrite(unsigned int address, unsigned int data) override {
        using namespace OpenSkyhawk::Boot;
        const uint8_t w = (uint8_t)((address - RELAY_EXPORT_ADDRESS) / 2);
        if (w == 0) _have = 0;
        _words[w] = (uint16_t)data;
        _have |= 1u << w;
        if (w != RELAY_EXPORT_WORDS - 1 || _have != (1u << RELAY_EXPORT_WORDS) - 1) return;
        const uint16_t id  = _words[0];
        const uint8_t  len = _words[1] < 8 ? (uint8_t)_words[1] : 8;
        const bool toNode  = id >= canIdBootCmd(1) && id <= canIdBootCmd(MAX_NODE_ID);
        if (!toNode && id != UNCONFIGURED_CMD_ID) return;
        uint8_t bytes[8];
        for (uint8_t i = 0; i < 8; i++) bytes[i] = (uint8_t)_words[2 + i];
        CANProtocol::send(id, bytes, len);
        if (toNode) noteBootTraffic((uint8_t)(id - CMD_ID_BASE), millis());
    }
private:
    uint16_t _words[OpenSkyhawk::Boot::RELAY_EXPORT_WORDS] = {};
    uint16_t _have = 0;
};
static BootRelayListener _bootRelayListener;

} // anonymous namespace

namespace PanelBridge {

void onNodeAlive(NodeCallback cb) { _cbAlive = cb; }
void onNodeDead(NodeCallback cb)  { _cbDead  = cb; }

void setup() {
    STM32Board::begin();
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.println(F("=============================="));
        d.println(F("  PanelBridge  NODE_ID=0"));
        d.println(F("=============================="));
    }
    Serial.begin(250000);
    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::filterAcceptAll();
    CANProtocol::onReceive(onCanRx);
    CANProtocol::start();
    broadcastSyncReq();
#if PANELBRIDGE_NODE_STATUS
    emitAllNodes();   // seed the host with the current roster (node-0 row only at cold boot) (#86)
#endif
    STM32Board::markBootHealthy();   // the bootloader's crash-loop counter back to 0 (#364)
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();
    // Sample `now` AFTER draining CAN RX. onCanRx (called from drain) stamps each node's
    // lastSeenMs with its own millis(); if `now` were sampled before drain, a millisecond ticking
    // over during drain leaves now < lastSeenMs, and the unsigned `now - lastSeenMs` in
    // checkNodeTimeouts underflows to ~4.29e9 > HB_TIMEOUT_MS — a false one-loop "dead" flap on a
    // node that is transmitting normally (#225).
    uint32_t now = millis();
    checkNodeTimeouts(now);

    // Node status (#340): HB_0 every 500 ms and HEALTH_0 every 1 s. HB_0 is an unconditional
    // liveness beacon — unlike CTRL_BCAST, which only moves when a DCS export changes — so nodes
    // detect master loss even when the sim is idle/paused. The bridge is a node too: its own
    // FaultSources (e.g. a future host-link watchdog) roll up into HEALTH_0. No master watchdog
    // runs here (NODE_ID 0 is the master); the bridge keeps no RX count for its heartbeat.
    CANProtocol::serviceNodeStatus(NodeKind::BRIDGE);

    auto& diag = STM32Board::diagSerial();
    while (diag.available()) {
        char c = (char)diag.read();
        if (c != 'T') continue;
        _testSeqNum++;
        _testSeqSentMs = millis();
        uint8_t payload[8] = {};
        payload[0] = (uint8_t)(_testSeqNum & 0xFF);
        payload[1] = (uint8_t)(_testSeqNum >> 8);
        payload[2] = (uint8_t)(_testSeqSentMs & 0xFF);
        payload[3] = (uint8_t)((_testSeqSentMs >>  8) & 0xFF);
        payload[4] = (uint8_t)((_testSeqSentMs >> 16) & 0xFF);
        payload[5] = (uint8_t)((_testSeqSentMs >> 24) & 0xFF);
        // payload[6..7] = reserved = 0
        CANProtocol::send(CAN_ID_TEST_SEQ, payload, 8);
        if (STM32Board::isDebug()) {
            diag.print(F("[BRIDGE] TEST_SEQ seq=")); diag.println(_testSeqNum);
        }
    }
}

} // namespace PanelBridge

#ifdef PANELBRIDGE_TEST
namespace PanelBridge {

void testDispatchEvt(uint16_t controlId, uint16_t value) {
    dispatchEvtSlot(controlId, value);
}

void testDispatchRel(uint16_t controlId, uint16_t value) {
    dispatchRelSlot(controlId, value);
}

void testDispatchDir(uint16_t controlId, uint16_t value) {
    dispatchDirSlot(controlId, value);
}

void testDispatchAction(uint16_t controlId, uint16_t value) {
    dispatchActionSlot(controlId, value);
}

void testFeedCanFrame(uint32_t canId, const uint8_t* data, uint8_t len) {
    onCanRx(canId, data, len);
}

uint32_t testDcsSendCount() {
    return _testDcsSends;
}

void testHandleExport(uint16_t address, uint16_t value) {
    handleDcsBiosExport(address, value);
}

#if PANELBRIDGE_NODE_STATUS
void testFeedHeartbeat(uint8_t nodeId, uint8_t flags, uint16_t uptime,
                       uint16_t rxCount, uint16_t esr) {
    HeartbeatPayload hb{ nodeId, flags, uptime, rxCount, esr };
    memcpy(&_nodes[nodeId - 1].last, &hb, sizeof(hb));
    markNodeAlive(nodeId, millis());
}

void testFeedHealth(uint8_t nodeId, int8_t dieTempC,
                    uint8_t hFlags, uint8_t faultMask, uint8_t faultId, uint8_t nodeKind) {
    // Mirrors the HEALTH_n branch of onCanRx: no liveness change; a kind change pushes a delta.
    NodeState& ns = _nodes[nodeId - 1];
    ns.dieTempC    = dieTempC;
    ns.healthFlags = hFlags;
    ns.faultMask   = faultMask;
    ns.faultId     = faultId;
    if (nodeKind != ns.nodeKind) {
        ns.nodeKind = nodeKind;
        if (ns.alive) emitNode(nodeId, true);
    }
}

void testRequestNodeStatus() { emitAllNodes(); }

void testCheckTimeouts(uint32_t now) { checkNodeTimeouts(now); }
#endif // PANELBRIDGE_NODE_STATUS

} // namespace PanelBridge
#endif // PANELBRIDGE_TEST

#endif // ARDUINO_ARCH_STM32
```


