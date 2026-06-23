// RotaryEncoder — cw_detent test
//
// A full clockwise quadrature cycle (Gray states 0->1->3->2->0) accumulates 4 steps. At
// OpenSkyhawk::FOUR_STEPS_PER_DETENT that is one detent → one CW (1) EVT. debugSeed sets the start state,
// debugStep injects each transition; assertions are on emitCount()/lastDir().
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No encoder hardware needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

static constexpr uint16_t CTRL_ID = 0x567B;

OpenSkyhawk::RotaryEncoder gEnc(CTRL_ID, PinRef(PA0), PinRef(PA1), OpenSkyhawk::FOUR_STEPS_PER_DETENT);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== RotaryEncoder cw_detent ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gEnc.configure();
    CANProtocol::start();

    gEnc.debugSeed(0);
    gEnc.debugStep(1); gEnc.debugStep(3); gEnc.debugStep(2); gEnc.debugStep(0);   // CW cycle
    check("CW full cycle -> 1 EVT, dir CW(1)", gEnc.emitCount() == 1 && gEnc.lastDir() == 1);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
