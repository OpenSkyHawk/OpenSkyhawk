// SwitchWithCover2Pos — sequence_off test
//
// Flipping the physical switch OFF must reverse the order: the switch drops first, then the cover
// closes behind it, at least COVER_DELAY_MS (200 ms) apart. Closing the cover before the switch
// was off would leave the sim with a shut cover over a live switch.
//
//
// Hardware: STM32. PB0→PA0 jumper wire required (PB0 drives the switch level, PA0 is the input),
// the same rig as the Switch2Pos suite. CAN in silent loopback — no bus, no PanelBridge. Do NOT
// run it on a board wired to a live bus: a loopback node never ACKs.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchWithCover2Pos/SwitchWithCover2Pos.h>

static constexpr uint16_t SWITCH_ID = 0xAB01;
static constexpr uint16_t COVER_ID  = 0xAB02;
static constexpr uint8_t  PIN_CTRL  = PB0;   // output — drives the switch level
static constexpr uint8_t  PIN_SW    = PA0;   // input  — the class reads this

struct Frame { uint16_t id; uint16_t value; uint32_t ms; };
static Frame   gFrames[8];
static uint8_t gCount = 0;

static void record(uint16_t id, uint16_t value) {
    if (gCount < 8) gFrames[gCount++] = { id, value, millis() };
}

static void onCan(uint32_t canId, const uint8_t* data, uint8_t len) {
    if (canId != canIdEvt(NODE_ID) || len < 8) return;
    const ControlPacketPair* pair = reinterpret_cast<const ControlPacketPair*>(data);
    if (pair->a.controlId == SWITCH_ID || pair->a.controlId == COVER_ID) record(pair->a.controlId, pair->a.value);
    if (pair->b.controlId == SWITCH_ID || pair->b.controlId == COVER_ID) record(pair->b.controlId, pair->b.value);
}

static void flushDrain() {
    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    delay(2);
    CANProtocol::drain();
}

OpenSkyhawk::SwitchWithCover2Pos gSw(SWITCH_ID, COVER_ID, PinRef(PIN_SW));

static void pump(uint32_t ms) {
    uint32_t t0 = millis();
    while (millis() - t0 < ms) { gSw.poll(); flushDrain(); }
}

// The whole suite is meaningless if the PB0->PA0 bridge is not conducting: every assertion below
// depends on the switch level actually changing. Drive the pin both ways and confirm the input
// follows, so a rig fault says so in one line instead of surfacing as a puzzling frame count.
static bool rigOk() {
    pinMode(PIN_SW, INPUT);
    digitalWrite(PIN_CTRL, HIGH);
    delay(2);
    const bool followsHigh = digitalRead(PIN_SW) == HIGH;
    digitalWrite(PIN_CTRL, LOW);
    delay(2);
    const bool followsLow = digitalRead(PIN_SW) == LOW;
    return followsHigh && followsLow;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== SwitchWithCover2Pos sequence_off ===");
    STM32Board::diagSerial().println("Hardware: PB0->PA0 jumper wire required.");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(PIN_CTRL, OUTPUT);
    if (!rigOk()) {
        STM32Board::diagSerial().println("RIG FAULT: PB0->PA0 bridge not conducting - check the jumper");
        STM32Board::diagSerial().println("=== FAIL ===");
        return;
    }

    digitalWrite(PIN_CTRL, HIGH);   // inactive (active-LOW default)
    gSw.configure();

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvt(NODE_ID));
    CANProtocol::startLoopback();

    // Start from the switch ON and settled, so the OFF sequence is what gets measured.
    digitalWrite(PIN_CTRL, LOW);    // switch ON
    gSw.forceReport();
    pump(700);
    gCount = 0;

    // ── The sequence under test ──────────────────────────────────────────────
    digitalWrite(PIN_CTRL, HIGH);   // switch OFF
    pump(700);

    check("off: exactly two frames       ", gCount == 2);
    if (gCount == 2) {
        check("off: switch first, value 0   ", gFrames[0].id == SWITCH_ID && gFrames[0].value == 0);
        check("off: cover second, value 0   ", gFrames[1].id == COVER_ID  && gFrames[1].value == 0);
        uint32_t gap = gFrames[1].ms - gFrames[0].ms;
        STM32Board::diagSerial().print("gap ms: "); STM32Board::diagSerial().println(gap);
        check("off: gap >= COVER_DELAY_MS   ", gap >= OpenSkyhawk::SwitchWithCover2Pos::COVER_DELAY_MS);
        check("off: gap not wildly over     ", gap < 300);
    } else {
        for (uint8_t i = 0; i < gCount; i++) {
            STM32Board::diagSerial().print("  frame 0x"); STM32Board::diagSerial().print(gFrames[i].id, HEX);
            STM32Board::diagSerial().print(" = ");        STM32Board::diagSerial().println(gFrames[i].value);
        }
        pass = false;
    }

    gCount = 0;
    pump(400);
    check("off: settled, no extra frames ", gCount == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
