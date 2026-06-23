// Switch3Pos — forceReport test
//
// forceReport() resolves the current position and emits immediately; it re-emits even when the
// position is unchanged, and arms poll(). Verified via position()/emitCount().
//
// Rig: this STM32 on the CAN bus with the PanelBridge. Jumper PB0->PA0 (pin A), PB1->PA1 (pin B).

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/Switch3Pos/Switch3Pos.h>

static constexpr uint16_t CTRL_ID = 0x5679;
static constexpr uint8_t  PIN_A = PA0, PIN_B = PA1;
static constexpr uint8_t  DRV_A = PB0, DRV_B = PB1;

static void setPins(uint8_t aLevel, uint8_t bLevel) {
    digitalWrite(DRV_A, aLevel);
    digitalWrite(DRV_B, bLevel);
    delayMicroseconds(100);
}
static void txPush() { CANProtocol::flushBatched(canIdEvt(NODE_ID)); }

OpenSkyhawk::Switch3Pos gSw(CTRL_ID, PinRef(PIN_A), PinRef(PIN_B));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Switch3Pos force_report ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(DRV_A, OUTPUT);
    pinMode(DRV_B, OUTPUT);
    setPins(HIGH, LOW);                       // pin B active
    gSw.configure();
    CANProtocol::start();

    gSw.forceReport(); txPush();
    check("forceReport pin B: 1 EVT, index 2", gSw.emitCount() == 1 && gSw.position() == 2);

    // Same position again — forceReport still emits.
    gSw.forceReport(); txPush();
    check("second forceReport same pos: 2 EVTs total", gSw.emitCount() == 2 && gSw.position() == 2);

    // poll() armed, no change → no extra EVT.
    gSw.poll();
    check("poll() after forceReport, no change: no extra EVT", gSw.emitCount() == 2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
