// CANProtocol — sendBatched / flushBatched test
//
// Two sendBatched() calls → one 8-byte ControlPacketPair (both slots non-null).
// One sendBatched() + flushBatched() → slot B is null sentinel (0x0000).
// One sendBatched() + 2 drain() calls → deadline flush, slot B null.
//
// Expected output:
//   Full pair received:      PASS
//   Null slot B on flush:    PASS
//   Total frames received:   3

#include <STM32Board.h>
#include <CANProtocol.h>
#include <string.h>

static int  _rxCount    = 0;
static bool _fullPairOk = false;
static bool _nullSlotB  = false;

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != CAN_ID_CTRL_BCAST || len != 8) return;
    _rxCount++;
    ControlPacketPair pair;
    memcpy(&pair, data, 8);
    if (pair.a.controlId != 0 && pair.b.controlId != 0) _fullPairOk = true;
    if (pair.b.controlId == 0x0000)                     _nullSlotB  = true;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== tx_batching ===");

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    CANProtocol::filterAcceptAll();
    CANProtocol::startLoopback();

    // Test 1: full pair
    ControlPacket a = {0x8001, 10};
    ControlPacket b = {0x8002, 20};
    CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, a);
    CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, b);

    // Test 2: explicit flush → null slot B
    ControlPacket c = {0x8003, 30};
    CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, c);
    CANProtocol::flushBatched(CAN_ID_CTRL_BCAST);

    // Test 3: deadline flush — one packet left pending; 2 drain() calls in loop() will flush it
    ControlPacket e = {0x8004, 40};
    CANProtocol::sendBatched(CAN_ID_CTRL_BCAST, e);
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();  // also services 2-loop batch deadline

    static bool _reported = false;
    if (!_reported && millis() > 1000) {
        _reported = true;
        auto& d = STM32Board::diagSerial();
        d.println(F("--- tx_batching results ---"));
        d.print(F("Full pair received:    ")); d.println(_fullPairOk ? F("PASS") : F("FAIL"));
        d.print(F("Null slot B on flush:  ")); d.println(_nullSlotB  ? F("PASS") : F("FAIL"));
        d.print(F("Total frames received: ")); d.println(_rxCount);  // expect 3
    }
}
