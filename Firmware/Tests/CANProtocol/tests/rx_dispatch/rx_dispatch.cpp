// CANProtocol — RX dispatch test
//
// SYNC_REQ fires onSyncReq, not onReceive.
// TEST_SEQ auto-sends ECHO (same payload) and is not forwarded to onReceive.
// ECHO from TEST_SEQ arrives in onReceive.
// Normal frames reach onReceive.
//
// Expected output:
//   SYNC_REQ fired onSyncReq:  PASS
//   SYNC_REQ not in onReceive: PASS
//   TEST_SEQ not in onReceive: PASS
//   ECHO arrived in onReceive: PASS
//   Normal frame in onReceive: PASS

#include <STM32Board.h>
#include <CANProtocol.h>

static bool _syncReqFired  = false;
static bool _syncReqInRx   = false;
static bool _testSeqInRx   = false;
static bool _echoArrived   = false;
static bool _normalArrived = false;

static void onSyncReq() {
    _syncReqFired = true;
}

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == CAN_ID_SYNC_REQ)    _syncReqInRx  = true;
    if (canId == CAN_ID_TEST_SEQ)    _testSeqInRx  = true;
    if (canId == canIdEcho(NODE_ID)) _echoArrived  = true;
    if (canId == canIdHb(NODE_ID))   _normalArrived = true;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== rx_dispatch ===");

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onSyncReq(onSyncReq);
    CANProtocol::onReceive(onRx);
    CANProtocol::filterAcceptAll();
    CANProtocol::startLoopback();

    uint8_t d[8] = {};
    CANProtocol::send(CAN_ID_SYNC_REQ,  d, 8);  // drain() → onSyncReq only
    CANProtocol::send(CAN_ID_TEST_SEQ,  d, 8);  // drain() → auto ECHO, not onReceive
    CANProtocol::send(canIdHb(NODE_ID), d, 8);  // drain() → onReceive
    // ECHO loops back and arrives in onReceive on the next drain() iteration
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    static bool _reported = false;
    if (!_reported && millis() > 500) {
        _reported = true;
        auto& d = STM32Board::diagSerial();
        d.println(F("--- rx_dispatch results ---"));
        d.print(F("SYNC_REQ fired onSyncReq:  ")); d.println(_syncReqFired  ? F("PASS") : F("FAIL"));
        d.print(F("SYNC_REQ not in onReceive: ")); d.println(!_syncReqInRx  ? F("PASS") : F("FAIL"));
        d.print(F("TEST_SEQ not in onReceive: ")); d.println(!_testSeqInRx  ? F("PASS") : F("FAIL"));
        d.print(F("ECHO arrived in onReceive: ")); d.println(_echoArrived   ? F("PASS") : F("FAIL"));
        d.print(F("Normal frame in onReceive: ")); d.println(_normalArrived ? F("PASS") : F("FAIL"));
    }
}
