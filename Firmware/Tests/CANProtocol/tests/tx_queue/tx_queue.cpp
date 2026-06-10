// CANProtocol — TX ring buffer test
//
// Payload bytes preserved end-to-end through ring buffer.
// CTRL_BCAST coalescing: rapid burst → last payload wins.
// TX overflow: burst of 30 frames → txDropCount() > 0.
//
// Expected output:
//   Payload preserved:    PASS
//   CTRL_BCAST coalesced: PASS   (last value == 20)
//   TX drops on burst:    <n>    (n > 0)

#include <STM32Board.h>
#include <CANProtocol.h>
#include <string.h>

static const uint8_t kMagic[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
static bool     _payloadOk    = false;
static bool     _coalescedOk  = false;
static uint16_t _lastSentVal  = 0;

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == canIdHb(NODE_ID) && len == 8) {
        _payloadOk = (memcmp(data, kMagic, 8) == 0);
    }
    if (canId == CAN_ID_CTRL_BCAST && len == 8) {
        ControlPacketPair pair;
        memcpy(&pair, data, 8);
        // Coalescing: last received value should match last sent
        if (pair.a.value == _lastSentVal) _coalescedOk = true;
    }
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== tx_queue ===");

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    CANProtocol::filterAcceptAll();
    CANProtocol::startLoopback();

    // Test 1: payload preservation
    CANProtocol::send(canIdHb(NODE_ID), kMagic, 8);

    // Test 2: CTRL_BCAST coalescing — burst of 20, last should win in ring
    for (uint16_t i = 1; i <= 20; i++) {
        ControlPacketPair p = {{0x8010, i}, {0x0000, 0}};
        CANProtocol::send(CAN_ID_CTRL_BCAST, reinterpret_cast<uint8_t*>(&p), 8);
        _lastSentVal = i;
    }

    // Test 3: overflow — 30 rapid sends exhaust 3 mailboxes + 16-entry ring = drops after 19
    uint32_t dropsBefore = CANProtocol::txDropCount();
    uint8_t d[8] = {};
    for (int i = 0; i < 30; i++) {
        CANProtocol::send(canIdReady(NODE_ID), d, 8);
    }
    uint32_t dropsAfter = CANProtocol::txDropCount();

    auto& diag = STM32Board::diagSerial();
    diag.print(F("[setup] drops from burst: "));
    diag.println(dropsAfter - dropsBefore);
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    static bool _reported = false;
    if (!_reported && millis() > 1000) {
        _reported = true;
        auto& d = STM32Board::diagSerial();
        d.println(F("--- tx_queue results ---"));
        d.print(F("Payload preserved:    ")); d.println(_payloadOk   ? F("PASS") : F("FAIL"));
        d.print(F("CTRL_BCAST coalesced: ")); d.println(_coalescedOk ? F("PASS") : F("FAIL"));
        d.print(F("txDropCount total:    ")); d.println(CANProtocol::txDropCount());
    }
}
