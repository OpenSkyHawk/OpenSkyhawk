// AnalogInput — ewma test
//
// The integer EWMA low-pass (α = 1/2^shift, default 1/8) moves the smoothed value a fraction toward
// each new reading: one step from 0 toward 40000 lands near 40000/8 = 5000; many steps converge.
// debugStep() runs one EWMA step bypassing the 8 ms throttle. Verified via smoothed().
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No jumpers / pot needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogInput/AnalogInput.h>

static constexpr uint16_t CTRL_ID = 0x567A;

OpenSkyhawk::AnalogInput gAna(CTRL_ID, PinRef(PA1));   // default EWMA shift 3 (α = 1/8)

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogInput ewma ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAna.configure();
    CANProtocol::start();

    gAna.debugSetRaw(0); gAna.forceReport();         // smoothed seeded to 0
    check("seed: smoothed 0", gAna.smoothed() == 0);

    gAna.debugSetRaw(40000);
    gAna.debugStep();                                 // one step ≈ 40000/8 = 5000
    check("1 EWMA step: ~1/8 toward target", gAna.smoothed() > 4000 && gAna.smoothed() < 6000);

    for (int i = 0; i < 60; i++) gAna.debugStep();    // converge
    check("converges to ~40000", gAna.smoothed() > 39000);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
