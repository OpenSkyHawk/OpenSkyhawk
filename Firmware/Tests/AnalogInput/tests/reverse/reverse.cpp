// AnalogInput — reverse test
//
// reverse = true inverts the mapping: minRaw → 65535, maxRaw → 0. Verified via value() after
// forceReport (seeds the EWMA immediately). No analog hardware — debugSetRaw injects.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No jumpers / pot needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogInput/AnalogInput.h>

static constexpr uint16_t CTRL_ID = 0x567A;

OpenSkyhawk::AnalogInput gAna(CTRL_ID, PinRef(PA1), /*reverse=*/true);   // full range 0..65535

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogInput reverse ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAna.configure();
    CANProtocol::start();

    gAna.debugSetRaw(0);     gAna.forceReport();
    check("reverse: 0 -> 65535", gAna.value() == 65535);

    gAna.debugSetRaw(65535); gAna.forceReport();
    check("reverse: 65535 -> 0", gAna.value() == 0);

    gAna.debugSetRaw(16384); gAna.forceReport();   // 65535 - 16384 = 49151
    check("reverse: quarter -> three-quarter", gAna.value() > 48000 && gAna.value() < 50000);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
