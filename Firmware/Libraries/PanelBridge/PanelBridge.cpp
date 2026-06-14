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
#include <CANProtocol.h>
#include <STM32Board.h>
#include <A4EC_InputMap.h>
#include <HIDControls.h>   // OSH_NODE_REQ_ADDR / OSH_NODE_MSG_NAME (node-status reporting, #86)
#include <string.h>

namespace {

// ── Node tracking ─────────────────────────────────────────────────────────────

static constexpr uint8_t  MAX_NODE_ID   = 63;
static constexpr uint32_t HB_TIMEOUT_MS = 3000;

struct NodeState {
    bool     alive;
    bool     everSeen;
    uint32_t lastSeenMs;
#ifdef PANELBRIDGE_NODE_STATUS
    HeartbeatPayload last;   // last HB payload, surfaced to the host (#86)
#endif
};

static NodeState      _nodes[MAX_NODE_ID] = {};   // index = nodeId - 1
static void (*_cbAlive)(uint8_t)          = nullptr;
static void (*_cbDead)(uint8_t)           = nullptr;

#ifdef PANELBRIDGE_NODE_STATUS
// ── Node-status reporting (#86) ──────────────────────────────────────────────
// Surface node presence + health to the host (OpenSkyhawk Client) as DCS-BIOS
// command messages on a reserved control name. Owned entirely by PanelBridge;
// SimGateway relays the ASCII verbatim. See HIDControls.h for the wire format.

// Emit one node's status: _OSH_NODE <nodeId present flags uptime rxCount esr> (18 hex chars).
static void emitNode(uint8_t nodeId, bool present) {
    const HeartbeatPayload& hb = _nodes[nodeId - 1].last;
    char hex[19];
    snprintf(hex, sizeof(hex), "%02X%02X%02X%04X%04X%04X",
             (unsigned)nodeId, present ? 1u : 0u, (unsigned)hb.flags,
             (unsigned)hb.uptime, (unsigned)hb.rxCount, (unsigned)hb.esr);
    DcsBios::sendDcsBiosMessage(OSH_NODE_MSG_NAME, hex);
}

// Emit the full roster (request response / boot seed): one _OSH_NODE per alive
// node, then _OSH_NODE_END <count> so the host knows the burst is complete and
// can reconcile (prune nodes absent from it). count=0 = no panels connected.
static void emitAllNodes() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_NODE_ID; i++)
        if (_nodes[i].alive) { emitNode(i + 1, true); count++; }
    char arg[4];
    snprintf(arg, sizeof(arg), "%u", (unsigned)count);
    DcsBios::sendDcsBiosMessage(OSH_NODE_END_MSG_NAME, arg);
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
#ifdef PANELBRIDGE_NODE_STATUS
    // Reserved: the host's node-status request — handled by NodeStatusReqListener,
    // never broadcast onto CAN.
    if (address == OSH_NODE_REQ_ADDR) return;
#endif
    ControlPacket pkt;
    pkt.controlId = address;
    pkt.value     = value;
    CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, pkt);
}

static void dispatchDcsInput(uint16_t controlId, uint16_t value) {
    // Binary search in A4EC_INPUT_MAP (sorted ascending by cmdId)
    int lo = 0, hi = (int)A4EC_INPUT_MAP_SIZE - 1;
    const DcsBiosInputEntry* entry = nullptr;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if      (A4EC_INPUT_MAP[mid].cmdId == controlId) { entry = &A4EC_INPUT_MAP[mid]; break; }
        else if (A4EC_INPUT_MAP[mid].cmdId <  controlId)   lo = mid + 1;
        else                                                hi = mid - 1;
    }
    if (!entry) {
        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[BRIDGE] unknown DCS ctrl=0x")); d.println(controlId, HEX);
        }
        return;
    }

    char multiBuf[7];  // "65535\0" + guard
    const char* arg = nullptr;

    switch (entry->type) {
        case InputType::SWITCH:
            if (value > 1) {
                if (STM32Board::isDebug()) {
                    auto& d = STM32Board::diagSerial();
                    d.print(F("[BRIDGE] bad val ctrl=0x")); d.print(controlId, HEX);
                    d.print(F(" val=")); d.println(value);
                }
                return;
            }
            arg = (value == 0) ? entry->arg0 : entry->arg1;
            break;
        case InputType::ACTION:
            if (value == 0) return;  // ignore release
            arg = entry->arg0;
            break;
        case InputType::ENCODER:
            if (value > 1) {
                if (STM32Board::isDebug()) {
                    auto& d = STM32Board::diagSerial();
                    d.print(F("[BRIDGE] bad val ctrl=0x")); d.print(controlId, HEX);
                    d.print(F(" val=")); d.println(value);
                }
                return;
            }
            arg = (value == 0) ? entry->arg0 : entry->arg1;
            break;
        case InputType::ACCEL_ENCODER:
            if      (value == 0) arg = entry->arg0;
            else if (value == 1) arg = entry->arg1;
            else if (value == 2) arg = entry->arg0fast;
            else if (value == 3) arg = entry->arg1fast;
            else {
                if (STM32Board::isDebug()) {
                    auto& d = STM32Board::diagSerial();
                    d.print(F("[BRIDGE] bad val ctrl=0x")); d.print(controlId, HEX);
                    d.print(F(" val=")); d.println(value);
                }
                return;
            }
            break;
        case InputType::MULTIPOS:
        case InputType::ANALOG:
            snprintf(multiBuf, sizeof(multiBuf), "%u", (unsigned)value);
            arg = multiBuf;
            break;
        default:
            return;
    }

    if (!arg) {
        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[BRIDGE] null arg ctrl=0x")); d.println(controlId, HEX);
        }
        return;
    }

    DcsBios::sendDcsBiosMessage(entry->name, arg);
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[BRIDGE] DCS -> \"")); d.print(entry->name);
        d.print(F("\" \"")); d.print(arg); d.println('"');
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

// Update liveness state and fire alive callback on dead/unseen → alive transition.
// Returns true if this call caused a transition (caller responsible for SYNC_REQ).
static bool markNodeAlive(uint8_t nodeId, uint32_t now) {
    uint8_t idx   = nodeId - 1;
    bool wasAlive = _nodes[idx].alive;
    _nodes[idx].alive      = true;
    _nodes[idx].everSeen   = true;
    _nodes[idx].lastSeenMs = now;
    if (!wasAlive) {
        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[BRIDGE] node=")); d.print(nodeId); d.println(F(" alive"));
        }
        if (_cbAlive) _cbAlive(nodeId);
#ifdef PANELBRIDGE_NODE_STATUS
        emitNode(nodeId, true);   // push presence on dead/unseen → alive (#86)
#endif
        return true;
    }
    return false;
}

static void onCanRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    uint32_t now = millis();

    // HB_1 – HB_63: update liveness; broadcast SYNC_REQ only on dead→alive transition
    if (canId >= canIdHb(1) && canId <= canIdHb(MAX_NODE_ID)) {
        uint8_t nodeId = (uint8_t)(canId - canIdHb(0));
#ifdef PANELBRIDGE_NODE_STATUS
        if (len >= 8) memcpy(&_nodes[nodeId - 1].last, data, 8);  // cache health for #86 reporting
#endif
        if (markNodeAlive(nodeId, now)) broadcastSyncReq();
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
        markNodeAlive(nodeId, now);
        broadcastSyncReq();
        return;
    }
    // All other IDs discarded silently
}

static void checkNodeTimeouts(uint32_t now) {
    for (uint8_t i = 0; i < MAX_NODE_ID; i++) {
        if (!_nodes[i].alive) continue;
        if (now - _nodes[i].lastSeenMs > HB_TIMEOUT_MS) {
            _nodes[i].alive = false;
            uint8_t nodeId  = i + 1;
            if (STM32Board::isDebug()) {
                auto& d = STM32Board::diagSerial();
                d.print(F("[BRIDGE] node=")); d.print(nodeId); d.println(F(" dead"));
            }
            if (_cbDead) _cbDead(nodeId);
#ifdef PANELBRIDGE_NODE_STATUS
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

// DCS-BIOS export listener — intercepts 0x8000–0x86FF and batches to CTRL_BCAST.
// ExportStreamListener::firstExportStreamListener (Protocol.cpp) is zero-initialized
// before any constructors run, so file-scope registration is safe.
class BridgeExportListener : public DcsBios::ExportStreamListener {
public:
    BridgeExportListener() : DcsBios::ExportStreamListener(0x8000, 0x86FF) {}
    void onDcsBiosWrite(unsigned int address, unsigned int data) override {
        handleDcsBiosExport((uint16_t)address, (uint16_t)data);
    }
};

static BridgeExportListener     _exportListener;
static DcsBios::StringBuffer<5> _modelTimeBuf(CommonData_MOD_TIME_A, onModelTimeChange);

#ifdef PANELBRIDGE_NODE_STATUS
// Host roster request (#86): the client writes OSH_NODE_REQ_ADDR on the DCS-BIOS
// export stream; PanelBridge replies with the full roster. handleDcsBiosExport()
// drops this address so it is never broadcast onto CAN.
class NodeStatusReqListener : public DcsBios::ExportStreamListener {
public:
    NodeStatusReqListener()
        : DcsBios::ExportStreamListener(OSH_NODE_REQ_ADDR, OSH_NODE_REQ_ADDR) {}
    void onDcsBiosWrite(unsigned int, unsigned int) override { emitAllNodes(); }
};
static NodeStatusReqListener _nodeStatusReqListener;
#endif

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
#ifdef PANELBRIDGE_NODE_STATUS
    emitAllNodes();   // seed the host with the current roster (empty at cold boot) (#86)
#endif
}

void loop() {
    uint32_t now = millis();
    STM32Board::tick();
    CANProtocol::drain();
    checkNodeTimeouts(now);

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

void testHandleExport(uint16_t address, uint16_t value) {
    handleDcsBiosExport(address, value);
}

#ifdef PANELBRIDGE_NODE_STATUS
void testFeedHeartbeat(uint8_t nodeId, uint8_t flags, uint16_t uptime,
                       uint16_t rxCount, uint16_t esr) {
    HeartbeatPayload hb{ nodeId, flags, uptime, rxCount, esr };
    memcpy(&_nodes[nodeId - 1].last, &hb, sizeof(hb));
    markNodeAlive(nodeId, millis());
}

void testRequestNodeStatus() { emitAllNodes(); }

void testCheckTimeouts(uint32_t now) { checkNodeTimeouts(now); }
#endif // PANELBRIDGE_NODE_STATUS

} // namespace PanelBridge
#endif // PANELBRIDGE_TEST

#endif // ARDUINO_ARCH_STM32
