// Switch3Pos — reverse test
//
// reverse = true inverts the active level: a pin is active when it reads HIGH (not LOW). The
// resolved index is unchanged; only the electrical sense flips. Verified via position(); CAN in
// NORMAL mode (node ACKs the PanelBridge).
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

OpenSkyhawk::Switch3Pos gSw(CTRL_ID, PinRef(PIN_A), PinRef(PIN_B), /*reverse=*/true);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== Switch3Pos reverse ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(DRV_A, OUTPUT);
    pinMode(DRV_B, OUTPUT);
    gSw.configure();
    CANProtocol::start();

    setPins(LOW, LOW);                        // reverse: neither active
    gSw.forceReport(); txPush();
    check("reverse: neither HIGH -> centre (1)", gSw.position() == 1);

    setPins(HIGH, LOW);                       // pin A active (HIGH)
    gSw.forceReport(); txPush();
    check("reverse: pin A HIGH -> 0", gSw.position() == 0);

    setPins(LOW, HIGH);                       // pin B active (HIGH)
    gSw.forceReport(); txPush();
    check("reverse: pin B HIGH -> 2", gSw.position() == 2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
