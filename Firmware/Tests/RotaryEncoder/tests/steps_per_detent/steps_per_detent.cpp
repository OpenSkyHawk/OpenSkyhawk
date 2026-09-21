// RotaryEncoder — steps_per_detent test
//
// At EncoderStepsPerDetent::One every quadrature transition emits. A full CW cycle (4 transitions) →
// 4 CW EVTs. Confirms the stepsPerDetent divisor: a 1-step encoder reports every edge.
//
// Rig: this STM32 alone, CAN in silent loopback (no bus, no PanelBridge, no encoder hardware).
// Do NOT run it on a board wired to a live bus: a loopback node never ACKs, which drives the
// bridge error-passive. PASS/FAIL comes from the encoder's own test seams, not from CAN.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

static constexpr uint16_t CTRL_ID = 0x567B;

OpenSkyhawk::RotaryEncoder gEnc(CTRL_ID, PinRef(PA0), PinRef(PA1), OpenSkyhawk::EncoderStepsPerDetent::One);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== RotaryEncoder steps_per_detent ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gEnc.configure();
    CANProtocol::startLoopback();

    gEnc.debugSeed(0);
    gEnc.debugStep(1); gEnc.debugStep(3); gEnc.debugStep(2); gEnc.debugStep(0);   // CW cycle
    check("ONE_STEP: full CW cycle -> 4 EVTs, REL +step", gEnc.emitCount() == 4 && gEnc.lastValue() == 3200);

    CANProtocol::flushBatched(canIdEvtRel(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
