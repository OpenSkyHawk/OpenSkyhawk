// Switch3Pos — positions test
//
// Drives the two pins to each of the three switch states and asserts the resolved index via
// Switch3Pos::position(): pin A active (LOW) -> 0, neither -> 1 (centre), pin B active -> 2.
// CAN runs in NORMAL mode: the node ACKs the (unmodified) PanelBridge — verification is on the
// class's own state, not a captured frame.
//
// Rig: this STM32 on the CAN bus with the PanelBridge. Jumper PB0->PA0 (pin A), PB1->PA1 (pin B).

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/Switch3Pos/Switch3Pos.h>

static constexpr uint16_t CTRL_ID = 0x5679;
static constexpr uint8_t  PIN_A = PA0, PIN_B = PA1;   // Switch3Pos inputs
static constexpr uint8_t  DRV_A = PB0, DRV_B = PB1;   // test outputs (jumpered to the inputs)

// Drive pin A / pin B levels (LOW = active for reverse = false).
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
    STM32Board::diagSerial().println("=== Switch3Pos positions ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(DRV_A, OUTPUT);
    pinMode(DRV_B, OUTPUT);
    gSw.configure();
    CANProtocol::start();                    // normal mode — node ACKs the bridge

    setPins(HIGH, HIGH);                      // neither active
    gSw.forceReport(); txPush();
    check("neither active -> centre (1)", gSw.position() == 1);

    setPins(LOW, HIGH);                       // pin A active
    gSw.forceReport(); txPush();
    check("pin A active -> 0", gSw.position() == 0);

    setPins(HIGH, LOW);                       // pin B active
    gSw.forceReport(); txPush();
    check("pin B active -> 2", gSw.position() == 2);

    check("3 forceReports -> 3 EVTs", gSw.emitCount() == 3);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
