// CANProtocol — duplicate NODE_ID check (#344)
//
// Single board, silent loopback (own frames come back). NODE_ID=1. A node never receives its own
// frames on a real bus, so a frame on its own HB_1 means another board has NODE_ID 1. In loopback
// the node's own heartbeats DO come back; serviceNodeStatus() discounts those, so only the extra
// HB_1 frames this test sends itself (standing in for a second board) read as a duplicate.
//
// Timeline (t = ms since boot complete; HEALTH_1 every 1000 ms from t≈1000):
//   0..1500      own HB/HEALTH only              → HEALTH faultId 0, kind 1, not DEGRADED
//   1500..2500   foreign HB_1 every 500 ms        → HEALTH at ~2000 + ~3000 carry DUPLICATE_NODE_ID
//   2500..       foreign HB_1 stops (last ~2500)  → clears 3000 ms later; HEALTH at ~6000 is clean
// The node's real FaultSource set is empty here, so any fault seen is the duplicate check's.
//
// Expected output (DiagSerial 115200), all PASS:
//   [NODE] degraded: another board transmits on this NODE_ID (fault 9)
//   [NODE] recovered
//   --- duplicate_node_id results ---
//   no false duplicate from own HBs:  PASS
//   kind PANEL_GROUP in HEALTH:       PASS
//   foreign HB_1 -> DUPLICATE_NODE_ID: PASS
//   foreign HB_1 -> DEGRADED:         PASS
//   held while foreign HB_1 flows:    PASS
//   cleared ~3 s after it stops:      PASS

#include <STM32Board.h>
#include <CANProtocol.h>

static uint32_t _startMs = 0;

static void pass(const __FlashStringHelper* label, bool ok) {
    auto& d = STM32Board::diagSerial();
    d.print(label);
    d.println(ok ? F("PASS") : F("FAIL"));
}

static bool isDup(const NodeHealthPayload& h) {
    return h.faultId == (uint8_t)NodeFaultCode::DUPLICATE_NODE_ID;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== duplicate_node_id ===");

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::startLoopback();  // own HB_1 is added to the filter by start() itself
    _startMs = millis();           // boot complete
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();
    CANProtocol::serviceNodeStatus(NodeKind::PANEL_GROUP);

    const uint32_t t = millis() - _startMs;

    // Second board stand-in: an extra HB_1 every 500 ms in [1500, 2500].
    static uint32_t lastForeign = 0;
    if (t >= 1500 && t <= 2500 && (lastForeign == 0 || t - lastForeign >= 500)) {
        lastForeign = t;
        HeartbeatPayload hb = CANProtocol::makeHeartbeatPayload(NODE_ID, 0);
        CANProtocol::send(canIdHb(NODE_ID), reinterpret_cast<const uint8_t*>(&hb), sizeof(hb));
    }

    // Probes on the last HEALTH_1 built, each latched once.
    static NodeHealthPayload before, during, held, after;
    static bool p1 = false, p2 = false, p3 = false, p4 = false;
    const NodeHealthPayload& h = CANProtocol::lastNodeHealth();
    if (!p1 && t >= 1400) { p1 = true; before = h; }  // HEALTH ~1000: no foreign frame yet
    if (!p2 && t >= 2400) { p2 = true; during = h; }  // HEALTH ~2000: foreign since 1500
    if (!p3 && t >= 3400) { p3 = true; held   = h; }  // HEALTH ~3000: last foreign ~2500
    if (!p4 && t >= 6400) { p4 = true; after  = h; }  // HEALTH ~6000: > 2500 + 3000

    static bool reported = false;
    if (!reported && t >= 6500) {
        reported = true;
        auto& d = STM32Board::diagSerial();
        d.println(F("--- duplicate_node_id results ---"));
        pass(F("no false duplicate from own HBs:  "), before.faultId == 0 && before.flags == 0);
        pass(F("kind PANEL_GROUP in HEALTH:       "), before.nodeKind == (uint8_t)NodeKind::PANEL_GROUP);
        pass(F("foreign HB_1 -> DUPLICATE_NODE_ID: "), isDup(during));
        pass(F("foreign HB_1 -> DEGRADED:         "),
             (during.flags & (uint8_t)NodeHealthFlag::DEGRADED) != 0);
        pass(F("held while foreign HB_1 flows:    "), isDup(held));
        pass(F("cleared ~3 s after it stops:      "), after.faultId == 0 && after.flags == 0);
    }
}
