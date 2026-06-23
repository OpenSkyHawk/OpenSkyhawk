// AnalogInput — force_report test
//
// forceReport() samples fresh (bypassing the throttle), seeds the EWMA to the reading, and emits
// the current value unconditionally — even when unchanged. A poll() immediately after is throttled
// (the read was just taken) so it emits nothing. Verified via emitCount()/value().
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No jumpers / pot needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogInput/AnalogInput.h>

static constexpr uint16_t CTRL_ID = 0x567A;

OpenSkyhawk::AnalogInput gAna(CTRL_ID, PinRef(PA1));

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogInput force_report ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAna.configure();
    CANProtocol::start();

    gAna.debugSetRaw(25000); gAna.forceReport();
    check("forceReport: 1 EVT, value 25000", gAna.emitCount() == 1 && gAna.value() == 25000);

    gAna.forceReport();   // same value — still emits
    check("second forceReport same value: 2 EVTs", gAna.emitCount() == 2 && gAna.value() == 25000);

    gAna.poll();          // throttled (read just taken) → no extra EVT
    check("poll() right after forceReport: no extra EVT", gAna.emitCount() == 2);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
