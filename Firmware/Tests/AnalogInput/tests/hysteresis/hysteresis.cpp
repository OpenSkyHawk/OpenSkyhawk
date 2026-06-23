// AnalogInput — hysteresis test
//
// A settled value within `hysteresis` (128) counts of the last sent value emits nothing; a change
// that settles beyond it emits and the value tracks. debugStep() runs one EWMA step bypassing the
// 8 ms throttle. Verified via emitCount()/value().
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No jumpers / pot needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogInput/AnalogInput.h>

static constexpr uint16_t CTRL_ID = 0x567A;

OpenSkyhawk::AnalogInput gAna(CTRL_ID, PinRef(PA1));   // defaults: hysteresis 128, EWMA shift 3

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogInput hysteresis ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAna.configure();
    CANProtocol::start();

    gAna.debugSetRaw(20000); gAna.forceReport();
    check("baseline: 1 EVT at 20000", gAna.emitCount() == 1 && gAna.value() == 20000);

    // Sub-hysteresis change (Δ = 50 < 128): EWMA settles to 20050 but nothing is emitted.
    gAna.debugSetRaw(20050);
    for (int i = 0; i < 40; i++) gAna.debugStep();
    check("sub-hysteresis change: no EVT", gAna.emitCount() == 1 && gAna.value() == 20000);

    // Supra-hysteresis change: settles at 50000, emits along the way, value() tracks.
    gAna.debugSetRaw(50000);
    for (int i = 0; i < 80; i++) gAna.debugStep();
    check("supra-hysteresis change: emitted + tracks", gAna.emitCount() > 1 && gAna.value() > 49000);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
