// CANProtocol — TX ring buffer test
//
// Test 1: Payload bytes preserved end-to-end (loopback).
// Test 2: CTRL_BCAST coalescing — requires dual-board real bus; SKIP in loopback.
// Test 3: TX overflow drops — requires dual-board real bus; SKIP in loopback.
//
// In CAN_MODE_SILENT_LOOPBACK the hardware frees TX mailboxes (TSR.TME) faster
// than any C loop can saturate them, so the TX ring never fills regardless of
// IRQ state. Tests 2 and 3 are documented here and will be verified during
// dual-board integration testing.
//
// Expected output (single board):
//   Payload preserved:    PASS
//   CTRL_BCAST coalesced: SKIP (loopback)
//   TX overflow drops:    SKIP (loopback)

#include <STM32Board.h>
#include <CANProtocol.h>
#include <string.h>

static const uint8_t kMagic[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
static bool _payloadOk = false;

static void onRx(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId == canIdHb(NODE_ID) && len == 8) {
        _payloadOk = (memcmp(data, kMagic, 8) == 0);
    }
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== tx_queue v4 ===");

    CANProtocol::onStatusChange(STM32Board::onCanStatus);
    CANProtocol::onReceive(onRx);
    CANProtocol::filterAcceptAll();
    CANProtocol::startLoopback();

    CANProtocol::send(canIdHb(NODE_ID), kMagic, 8);
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();

    static bool _reported = false;
    if (!_reported && millis() > 500) {
        _reported = true;
        auto& d = STM32Board::diagSerial();
        d.println(F("--- tx_queue results ---"));
        d.print(F("Payload preserved:    ")); d.println(_payloadOk ? F("PASS") : F("FAIL"));
        d.println(F("CTRL_BCAST coalesced: SKIP (loopback)"));
        d.println(F("TX overflow drops:    SKIP (loopback)"));
    }
}
