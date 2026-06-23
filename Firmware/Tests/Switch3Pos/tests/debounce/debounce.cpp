// Switch3Pos — debounce test
//
// A changed position must hold steady for DEBOUNCE_MS (20 ms) before it is confirmed and emitted;
// a transient that returns to the current position before the window expires emits nothing.
// Verified via position()/emitCount(); CAN in NORMAL mode (node ACKs the PanelBridge).
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
    STM32Board::diagSerial().println("=== Switch3Pos debounce ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    pinMode(DRV_A, OUTPUT);
    pinMode(DRV_B, OUTPUT);
    setPins(HIGH, HIGH);                      // centre
    gSw.configure();
    CANProtocol::start();

    gSw.forceReport(); txPush();             // baseline at centre (pos 1)
    uint16_t base = gSw.emitCount();         // == 1

    // Change to pin A — not emitted until the 20 ms window expires.
    setPins(LOW, HIGH);
    gSw.poll();                              // arms debounce
    check("poll() right after change: no EVT yet", gSw.emitCount() == base && gSw.position() == 1);

    delay(25);
    gSw.poll(); txPush();                     // window expired → confirm + emit
    check("after 20 ms: EVT index 0", gSw.emitCount() == base + 1 && gSw.position() == 0);

    // Bounce: from settled 0, blip to centre then back to A within the window → no EVT.
    uint16_t before = gSw.emitCount();
    setPins(HIGH, HIGH); gSw.poll();         // pending = 1, timer restarts
    setPins(LOW, HIGH);  gSw.poll();         // pending = 0 again (== confirmed)
    delay(25);
    gSw.poll();
    check("blip 0->centre->0 within window: no EVT", gSw.emitCount() == before && gSw.position() == 0);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
