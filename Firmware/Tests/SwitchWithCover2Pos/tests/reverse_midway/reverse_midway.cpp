// SwitchWithCover2Pos — reverse_midway test
//
// Flipping back mid-sequence must walk the machine back the way it came, not finish the sequence
// it started. Here the switch goes on — the cover opens — and then goes off again before the
// switch frame is due: the switch frame must never be sent at all (DCS never sees it thrown), and
// the cover must close again.
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

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== SwitchWithCover2Pos reverse_midway ===");
    STM32Board::diagSerial().println("Hardware: PB0->PA0 jumper wire required.");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(PIN_CTRL, OUTPUT);
    digitalWrite(PIN_CTRL, HIGH);   // inactive (active-LOW default)
    gSw.configure();

    CANProtocol::onReceive(onCan);
    CANProtocol::filterAcceptId(canIdEvt(NODE_ID));
    CANProtocol::startLoopback();

    // Settle the boot baseline (switch off) and discard it.
    gSw.forceReport();
    pump(600);
    gCount = 0;

    // ── Flip on, then back before the switch frame is due ────────────────────
    digitalWrite(PIN_CTRL, LOW);    // switch ON
    pump(80);                       // past the 20 ms debounce: cover opens, switch frame not yet due
    check("midway: cover frame sent      ", gCount == 1 && gFrames[0].id == COVER_ID && gFrames[0].value == 1);

    digitalWrite(PIN_CTRL, HIGH);   // switch OFF again, mid-sequence
    pump(700);

    // Walking back from OFF_OPEN means one frame — the cover closing. The switch was never
    // reported thrown, so there is nothing to retract.
    bool sawSwitch = false;
    for (uint8_t i = 0; i < gCount; i++) if (gFrames[i].id == SWITCH_ID) sawSwitch = true;
    check("midway: switch never sent     ", !sawSwitch);
    check("midway: two frames total      ", gCount == 2);
    if (gCount == 2) {
        check("midway: second is cover 0    ", gFrames[1].id == COVER_ID && gFrames[1].value == 0);
    } else {
        for (uint8_t i = 0; i < gCount; i++) {
            STM32Board::diagSerial().print("  frame 0x"); STM32Board::diagSerial().print(gFrames[i].id, HEX);
            STM32Board::diagSerial().print(" = ");        STM32Board::diagSerial().println(gFrames[i].value);
        }
        pass = false;
    }
    check("midway: machine back closed   ", gSw.state() == 0 && gSw.target() == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
