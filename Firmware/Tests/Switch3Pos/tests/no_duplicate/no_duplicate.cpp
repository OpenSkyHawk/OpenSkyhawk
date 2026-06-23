// Switch3Pos — no_duplicate test
//
// A held position emits exactly once; repeated poll() with no change produces no further EVT.
// Verified via emitCount(); CAN in NORMAL mode (node ACKs the PanelBridge).
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
    STM32Board::diagSerial().println("=== Switch3Pos no_duplicate ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(DRV_A, OUTPUT);
    pinMode(DRV_B, OUTPUT);
    setPins(LOW, HIGH);                       // pin A active
    gSw.configure();
    CANProtocol::start();

    gSw.forceReport(); txPush();             // baseline pos 0
    check("forceReport -> 1 EVT, pos 0", gSw.emitCount() == 1 && gSw.position() == 0);

    for (uint8_t i = 0; i < 10; i++) { gSw.poll(); delay(5); }   // > debounce window, no change
    txPush();
    check("held pos 0: no duplicate EVT", gSw.emitCount() == 1 && gSw.position() == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
