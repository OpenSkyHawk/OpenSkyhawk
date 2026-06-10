// CANProtocol — filter configuration test
//
// filterAcceptId: accepted ID arrives in onReceive; non-accepted ID is blocked.
// Mandatory IDs (CTRL_BCAST, TEST_SEQ, SYNC_REQ) always present regardless of filter.
//
// Expected output:
//   HB arrived (accepted):         PASS
//   READY blocked (not in filter): PASS
//   CTRL_BCAST (mandatory):        PASS

#include <STM32Board.h>
#include <CANProtocol.h>

static bool _hbArrived    = false;
static bool _readyArrived = false;
static bool _ctrlArrived  = false;

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == canIdHb(NODE_ID))    _hbArrived    = true;
    if (canId == canIdReady(NODE_ID)) _readyArrived = true;
    if (canId == CAN_ID_CTRL_BCAST)   _ctrlArrived  = true;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== loopback_filter ===");

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    CANProtocol::filterAcceptId(canIdHb(NODE_ID));  // accept HB; start() adds mandatory IDs
    CANProtocol::startLoopback();

    uint8_t d[8] = {};
    CANProtocol::send(canIdHb(NODE_ID),    d, 8);  // in filter — should arrive
    CANProtocol::send(canIdReady(NODE_ID), d, 8);  // not in filter — should be blocked

    ControlPacketPair pair = {{0x8001, 1}, {0x0000, 0}};
    CANProtocol::send(CAN_ID_CTRL_BCAST, reinterpret_cast<uint8_t*>(&pair), 8);  // mandatory
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    static bool _reported = false;
    if (!_reported && millis() > 500) {
        _reported = true;
        auto& d = STM32Board::diagSerial();
        d.println(F("--- loopback_filter results ---"));
        d.print(F("HB arrived (accepted):         ")); d.println(_hbArrived    ? F("PASS") : F("FAIL"));
        d.print(F("READY blocked (not in filter): ")); d.println(!_readyArrived ? F("PASS") : F("FAIL"));
        d.print(F("CTRL_BCAST (mandatory):        ")); d.println(_ctrlArrived  ? F("PASS") : F("FAIL"));
    }
}
