// RotaryEncoder — reversal test
//
// A CW detent then a CCW detent emit 1 (CW) then 0 (CCW). Confirms direction tracking across a
// reversal with no stuck state. OpenSkyhawk::FOUR_STEPS_PER_DETENT.
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
    STM32Board::diagSerial().println("=== RotaryEncoder reversal ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gEnc.configure();
    CANProtocol::start();

    gEnc.debugSeed(0);
    gEnc.debugStep(1); gEnc.debugStep(3); gEnc.debugStep(2); gEnc.debugStep(0);   // CW detent
    check("CW detent -> dir 1", gEnc.emitCount() == 1 && gEnc.lastDir() == 1);

    gEnc.debugStep(2); gEnc.debugStep(3); gEnc.debugStep(1); gEnc.debugStep(0);   // CCW detent
    check("then CCW detent -> dir 0", gEnc.emitCount() == 2 && gEnc.lastDir() == 0);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
