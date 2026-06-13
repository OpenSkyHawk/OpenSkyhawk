// PanelGroup — CAN drain test
//
// Verifies: CANProtocol::drain() routes received frames to the registered
// onReceive callback, and that frames sent via loopback arrive exactly once.
// Also verifies that drain() is non-blocking (returns promptly with nothing
// queued) and that multiple frames are all delivered.
//
// Hardware: STM32. No CAN bus connected — uses CAN loopback mode.

#include <Arduino.h>
#include <STM32Board.h>
#include <PanelGroup.h>

static uint8_t  rxCount    = 0;
static uint32_t lastRxId   = 0;
static uint8_t  lastRxLen  = 0;
static uint8_t  lastRxData[8] = {};

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    rxCount++;
    lastRxId  = canId;
    lastRxLen = len;
    if (len > 8) len = 8;
    memcpy(lastRxData, data, len);
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== PanelGroup can_drain ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptAll();  // test covers drain routing, not filtering
    CANProtocol::startLoopback();

    // ── Drain with nothing queued — must not hang ───────────────────────────

    uint32_t t0 = millis();
    CANProtocol::drain();
    uint32_t elapsed = millis() - t0;
    check("Empty drain returns promptly (<5 ms)", elapsed < 5);
    check("No spurious frames on empty drain",    rxCount == 0);

    // ── Single frame roundtrip ───────────────────────────────────────────────
    // delay(2): CAN frame takes ~100 µs to loop back at 500 kbps.

    const uint8_t payload1[] = { 0xA1, 0xB2, 0xC3, 0xD4 };
    CANProtocol::send(canIdEvt(NODE_ID), payload1, sizeof(payload1));
    delay(2);
    CANProtocol::drain();

    check("Single frame received",              rxCount == 1);
    check("CAN ID preserved",                   lastRxId == canIdEvt(NODE_ID));
    check("DLC preserved (4)",                  lastRxLen == 4);
    check("Payload byte 0",                     lastRxData[0] == 0xA1);
    check("Payload byte 3",                     lastRxData[3] == 0xD4);

    // ── Zero-length frame (READY / SYNC_REQ style) ──────────────────────────

    rxCount = 0;
    static const uint8_t kEmpty[1] = {};
    CANProtocol::send(canIdReady(NODE_ID), kEmpty, 0);
    delay(2);
    CANProtocol::drain();

    check("DLC=0 frame received",   rxCount == 1);
    check("DLC=0 length correct",   lastRxLen == 0);

    // ── Multiple frames — all delivered ─────────────────────────────────────

    rxCount = 0;
    const uint8_t p2[] = { 0x11, 0x22 };
    const uint8_t p3[] = { 0x33, 0x44, 0x55 };
    CANProtocol::send(canIdEvt(NODE_ID),  p2, sizeof(p2));
    CANProtocol::send(canIdHb(NODE_ID),   p3, sizeof(p3));
    delay(2);
    CANProtocol::drain();

    check("Both frames delivered (rxCount=2)", rxCount == 2);

    // ── Drain again — queue must be empty now ────────────────────────────────

    rxCount = 0;
    CANProtocol::drain();
    check("Second drain: no duplicates", rxCount == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
