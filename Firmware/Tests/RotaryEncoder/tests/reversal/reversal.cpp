// RotaryEncoder — reversal test
//
// A CW detent then a CCW detent emit +step (CW) then -step (CCW). Confirms direction tracking across a
// reversal with no stuck state. OpenSkyhawk::EncoderStepsPerDetent::Four.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No encoder hardware needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

static constexpr uint16_t CTRL_ID = 0x567B;

OpenSkyhawk::RotaryEncoder gEnc(CTRL_ID, PinRef(PA0), PinRef(PA1), OpenSkyhawk::EncoderStepsPerDetent::Four);

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
    check("CW detent -> REL +step", gEnc.emitCount() == 1 && gEnc.lastValue() == 3200);

    gEnc.debugStep(2); gEnc.debugStep(3); gEnc.debugStep(1); gEnc.debugStep(0);   // CCW detent
    check("then CCW detent -> REL -step", gEnc.emitCount() == 2 && gEnc.lastValue() == -3200);

    CANProtocol::flushBatched(canIdEvtRel(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
