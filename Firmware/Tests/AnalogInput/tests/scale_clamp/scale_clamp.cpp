// AnalogInput — scale_clamp test
//
// Injected raw ADC values map to 0..65535 across [minRaw, maxRaw]; out-of-range readings clamp to
// the rails. forceReport() seeds the EWMA to the current reading, so value() == the scaled result
// immediately. No analog hardware — debugSetRaw injects; assertions are on value().
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No jumpers / pot needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogInput/AnalogInput.h>

static constexpr uint16_t CTRL_ID = 0x567A;

// minRaw = 1000, maxRaw = 60000 → a sub-range calibration.
OpenSkyhawk::AnalogInput gAna(CTRL_ID, PinRef(PA1), /*reverse=*/false, 1000, 60000);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogInput scale_clamp ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAna.configure();
    CANProtocol::start();

    gAna.debugSetRaw(1000);  gAna.forceReport();
    check("min 1000 -> 0", gAna.value() == 0);

    gAna.debugSetRaw(60000); gAna.forceReport();
    check("max 60000 -> 65535", gAna.value() == 65535);

    gAna.debugSetRaw(500);   gAna.forceReport();
    check("below min -> 0 (clamp)", gAna.value() == 0);

    gAna.debugSetRaw(65000); gAna.forceReport();
    check("above max -> 65535 (clamp)", gAna.value() == 65535);

    gAna.debugSetRaw(30500); gAna.forceReport();   // (30500-1000)*65535/59000 = 32767
    check("mid -> ~half", gAna.value() > 32000 && gAna.value() < 33500);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
