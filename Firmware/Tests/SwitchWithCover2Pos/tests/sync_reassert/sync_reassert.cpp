// SwitchWithCover2Pos — sync_reassert test
//
// forceReport() is the boot and SYNC_REQ baseline. DCS-BIOS re-sends nothing on reset; we re-assert
// BOTH controls in the correct order (FirmwarePlan D5), because a missed input otherwise leaves
// DCS wrong until the switch is next touched. Checked in both resting positions.
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
    STM32Board::diagSerial().println("=== SwitchWithCover2Pos sync_reassert ===");
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

    // ── Switch resting OFF: forceReport() must re-assert switch 0 then cover 0 ───
    digitalWrite(PIN_CTRL, HIGH);   // inactive
    delayMicroseconds(100);
    gCount = 0;
    gSw.forceReport();
    pump(700);

    check("sync off: two frames          ", gCount == 2);
    if (gCount == 2) {
        check("sync off: switch 0 first     ", gFrames[0].id == SWITCH_ID && gFrames[0].value == 0);
        check("sync off: cover 0 second     ", gFrames[1].id == COVER_ID  && gFrames[1].value == 0);
        check("sync off: gap >= delay       ",
              gFrames[1].ms - gFrames[0].ms >= OpenSkyhawk::SwitchWithCover2Pos::COVER_DELAY_MS);
    } else { pass = false; }

    // ── Switch resting ON: the pair comes back cover-first ──────────────────────
    digitalWrite(PIN_CTRL, LOW);    // active
    delayMicroseconds(100);
    gCount = 0;
    gSw.forceReport();
    pump(700);

    check("sync on: two frames           ", gCount == 2);
    if (gCount == 2) {
        check("sync on: cover 1 first       ", gFrames[0].id == COVER_ID  && gFrames[0].value == 1);
        check("sync on: switch 1 second     ", gFrames[1].id == SWITCH_ID && gFrames[1].value == 1);
        check("sync on: gap >= delay        ",
              gFrames[1].ms - gFrames[0].ms >= OpenSkyhawk::SwitchWithCover2Pos::COVER_DELAY_MS);
    } else { pass = false; }

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
