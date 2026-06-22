// SwitchMultiPos — one_hot test
//
// Drives each of N position pins active (LOW, reverse=false) in turn and asserts the resolved
// index via SwitchMultiPos::position(). CAN runs in NORMAL mode: the node ACKs the (unmodified)
// PanelBridge and the EVTs reach it — verification is on the class's own state, not a captured
// frame, so there is no CAN-loopback fragility.
//
// Rig: this STM32 on the CAN bus with the PanelBridge. Watch the selector move in DCS as a bonus.
//   Jumper PB0->PA0, PB1->PA1, PB10->PA4, PB5->PA5  (PBx outputs simulate the switch lines).

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/SwitchMultiPos/SwitchMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x5678;
static constexpr uint8_t  N       = 4;
static const uint8_t SW_PINS[N]   = { PA0, PA1, PA4, PA5 };
static const uint8_t CTRL_PINS[N] = { PB0, PB1, PB10, PB5 };

// Drive position idx LOW (active), the rest HIGH.
static void setActive(uint8_t idx) {
    for (uint8_t i = 0; i < N; i++) digitalWrite(CTRL_PINS[i], i == idx ? LOW : HIGH);
    delayMicroseconds(100);
}

static const PinRef gPins[N] = { PinRef(SW_PINS[0]), PinRef(SW_PINS[1]), PinRef(SW_PINS[2]), PinRef(SW_PINS[3]) };
OpenSkyhawk::SwitchMultiPos gSel(CTRL_ID, gPins, N);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== SwitchMultiPos one_hot ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    for (uint8_t i = 0; i < N; i++) pinMode(CTRL_PINS[i], OUTPUT);
    gSel.configure();
    CANProtocol::start();                    // normal mode — node ACKs the bridge

    for (uint8_t i = 0; i < N; i++) {
        setActive(i);
        gSel.forceReport();
        CANProtocol::flushBatched(canIdEvt(NODE_ID));   // push the EVT to the bridge (E2E)
        char lbl[40];
        snprintf(lbl, sizeof(lbl), "pos %u active -> index %u", i, gSel.position());
        check(lbl, gSel.position() == i);
    }
    check("4 positions -> 4 EVTs emitted", gSel.emitCount() == N);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
