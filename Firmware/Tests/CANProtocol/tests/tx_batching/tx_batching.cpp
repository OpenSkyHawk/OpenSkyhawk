// CANProtocol — sendBatched / flushBatched test
//
// Two sendBatched() calls → one 8-byte ControlPacketPair (both slots non-null).
// One sendBatched() + flushBatched() → slot B is null sentinel (0x0000).
// One sendBatched() + 2 drain() calls → deadline flush, slot B null.
//
// ACTION frame: full pair, explicit flush, and deadline flush on canIdEvtAction(NODE_ID) —
// proves the 5th _batches[] slot exists (a frame with no slot is silently discarded).
//
// Expected output:
//   Full pair received:      PASS
//   Null slot B on flush:    PASS
//   Total frames received:   3
//   Action frame accepted:   PASS
//   Action frames received:  3

#include <STM32Board.h>
#include <CANProtocol.h>
#include <string.h>

static int  _rxCount    = 0;
static bool _fullPairOk = false;
static bool _nullSlotB  = false;

// ACTION frame (canIdEvtAction) — a frame with no slot in _batches[] is silently discarded by
// sendBatched(), so "any frame arrived at all" is what proves the batch slot was registered.
static int  _actionCount   = 0;
static bool _actionPairOk  = false;
static bool _actionNullB   = false;

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (len != 8) return;
    ControlPacketPair pair;
    memcpy(&pair, data, 8);

    if (canId == CAN_ID_CTRL_BCAST) {
        _rxCount++;
        if (pair.a.controlId != 0 && pair.b.controlId != 0) _fullPairOk = true;
        if (pair.b.controlId == 0x0000)                     _nullSlotB  = true;
    } else if (canId == canIdEvtAction(NODE_ID)) {
        _actionCount++;
        if (pair.a.controlId != 0 && pair.b.controlId != 0) _actionPairOk = true;
        if (pair.b.controlId == 0x0000)                     _actionNullB  = true;
    }
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

    // Test 4: ACTION frame has its own batch slot — full pair
    ControlPacket f = {0x8005, 0};
    ControlPacket g = {0x8006, 0};
    CANProtocol::sendBatched(canIdEvtAction(NODE_ID), f);
    CANProtocol::sendBatched(canIdEvtAction(NODE_ID), g);

    // Test 5: ACTION explicit flush → null slot B
    ControlPacket h = {0x8007, 0};
    CANProtocol::sendBatched(canIdEvtAction(NODE_ID), h);
    CANProtocol::flushBatched(canIdEvtAction(NODE_ID));

    // Test 6: ACTION deadline flush — left pending for the 2-drain deadline in loop()
    ControlPacket i = {0x8008, 0};
    CANProtocol::sendBatched(canIdEvtAction(NODE_ID), i);
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();  // also services 2-loop batch deadline

    static bool _reported = false;
    if (!_reported && ((_rxCount >= 3 && _actionCount >= 3) || millis() > 2000)) {
        _reported = true;
        auto& d = STM32Board::diagSerial();
        d.println(F("--- tx_batching results ---"));
        d.print(F("Full pair received:    ")); d.println(_fullPairOk ? F("PASS") : F("FAIL"));
        d.print(F("Null slot B on flush:  ")); d.println(_nullSlotB  ? F("PASS") : F("FAIL"));
        d.print(F("Total frames received: ")); d.println(_rxCount);  // expect 3
        d.println(F("--- ACTION frame (5th batch slot) ---"));
        d.print(F("Action frame accepted: ")); d.println(_actionCount > 0 ? F("PASS") : F("FAIL — no batch slot"));
        d.print(F("Action full pair:      ")); d.println(_actionPairOk ? F("PASS") : F("FAIL"));
        d.print(F("Action null slot B:    ")); d.println(_actionNullB  ? F("PASS") : F("FAIL"));
        d.print(F("Action frames received:")); d.println(_actionCount); // expect 3
    }
}
