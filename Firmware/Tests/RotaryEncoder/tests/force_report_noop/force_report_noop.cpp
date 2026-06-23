// RotaryEncoder — force_report_noop test
//
// A relative encoder has no absolute baseline, so forceReport() resyncs the state and emits
// NOTHING (unlike the switch/analog inputs). A subsequent transition still decodes and emits.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No encoder hardware needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/RotaryEncoder/RotaryEncoder.h>

static constexpr uint16_t CTRL_ID = 0x567B;

OpenSkyhawk::RotaryEncoder gEnc(CTRL_ID, PinRef(PA0), PinRef(PA1), OpenSkyhawk::ONE_STEP_PER_DETENT);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== RotaryEncoder force_report_noop ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gEnc.configure();
    CANProtocol::start();

    gEnc.forceReport();
    check("forceReport: no EVT (relative control)", gEnc.emitCount() == 0);

    gEnc.debugSeed(0);
    gEnc.debugStep(1);   // one CW transition (ONE_STEP) → emits
    check("transition after forceReport: emits REL +step", gEnc.emitCount() == 1 && gEnc.lastValue() == 3200);

    CANProtocol::flushBatched(canIdEvtRel(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
