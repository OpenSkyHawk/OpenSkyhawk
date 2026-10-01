// SwitchWithCover2Pos — sequence_on test
//
// Flipping the physical switch ON must produce two frames in order, at least COVER_DELAY_MS
// (200 ms) apart: the cover opens first, then the switch throws. That order is what makes the
// sim's cover animate and satisfies any sim logic gated on the cover being open.
//
// The 200 ms gap is also the thing most easily lost: a sequencer that collapsed to a single poll()
// or used delay() would fail here on the measured timestamps, not on the frame contents.
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
    STM32Board::diagSerial().println("=== SwitchWithCover2Pos sequence_on ===");
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

    // Settle the boot baseline (switch off → switch 0, cover 0) and discard it.
    gSw.forceReport();
    pump(600);
    gCount = 0;

    // ── The sequence under test ──────────────────────────────────────────────
    digitalWrite(PIN_CTRL, LOW);    // switch ON
    pump(700);                      // debounce 20 ms + first frame + 200 ms + second frame

    check("on: exactly two frames        ", gCount == 2);
    if (gCount == 2) {
        check("on: cover first, value 1     ", gFrames[0].id == COVER_ID  && gFrames[0].value == 1);
        check("on: switch second, value 1   ", gFrames[1].id == SWITCH_ID && gFrames[1].value == 1);
        uint32_t gap = gFrames[1].ms - gFrames[0].ms;
        STM32Board::diagSerial().print("gap ms: "); STM32Board::diagSerial().println(gap);
        check("on: gap >= COVER_DELAY_MS    ", gap >= OpenSkyhawk::SwitchWithCover2Pos::COVER_DELAY_MS);
        check("on: gap not wildly over      ", gap < 300);
    } else {
        for (uint8_t i = 0; i < gCount; i++) {
            STM32Board::diagSerial().print("  frame 0x"); STM32Board::diagSerial().print(gFrames[i].id, HEX);
            STM32Board::diagSerial().print(" = ");        STM32Board::diagSerial().println(gFrames[i].value);
        }
        pass = false;
    }

    // Settled: no further frames once the machine reached its target.
    gCount = 0;
    pump(400);
    check("on: settled, no extra frames  ", gCount == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
