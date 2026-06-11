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
static uint8_t  _stage     = 0;
static uint32_t _stageMs   = 0;

static void onSyncReq() {
    _syncReqFired = true;
}

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == CAN_ID_SYNC_REQ)    _syncReqInRx  = true;
    if (canId == CAN_ID_TEST_SEQ)    _testSeqInRx  = true;
    if (canId == canIdEcho(NODE_ID) && len == 8 && data[0] == 0xA5) _echoArrived = true;
    if (canId == canIdHb(NODE_ID))   _normalArrived = true;
}

static void advanceStage() {
    uint8_t d[8] = {};

    _stageMs = millis();
    switch (_stage) {
        case 0:
            CANProtocol::send(CAN_ID_SYNC_REQ, d, 8);  // drain() → onSyncReq only
            _stage = 1;
            break;
        case 1: {
            uint8_t testSeq[8] = {0xA5};
            CANProtocol::send(CAN_ID_TEST_SEQ, testSeq, 8);  // drain() → auto ECHO, not onReceive
            _stage = 2;
            break;
        }
        case 2:
            CANProtocol::send(canIdHb(NODE_ID), d, 8);  // drain() → onReceive
            _stage = 3;
            break;
        default:
            break;
    }
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

    advanceStage();
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    if (_stage == 1 && (_syncReqFired || millis() - _stageMs > 100)) {
        advanceStage();
    } else if (_stage == 2 && (_echoArrived || millis() - _stageMs > 100)) {
        advanceStage();
    } else if (_stage == 3 && (_normalArrived || millis() - _stageMs > 100)) {
        _stage = 4;
        _stageMs = millis();
    }

    static bool _reported = false;
    if (!_reported && _stage == 4 && millis() - _stageMs > 100) {
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
