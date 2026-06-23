// AnalogInput — near_rail test
//
// Within `hysteresis` of a rail (0 / 65535), a reading moving toward that rail is emitted even
// though it has not cleared the hysteresis band — so full travel always reaches the endpoint.
// debugStep() runs one EWMA step bypassing the 8 ms throttle. Verified via value()/smoothed().
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
    STM32Board::diagSerial().println("=== AnalogInput near_rail ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAna.configure();
    CANProtocol::start();

    gAna.debugSetRaw(30000); gAna.forceReport();      // mid baseline

    gAna.debugSetRaw(65535);
    for (int i = 0; i < 80; i++) gAna.debugStep();     // climb to the top rail
    check("top rail reached", gAna.value() > 65535 - 200 && gAna.smoothed() > 65535 - 100);

    gAna.debugSetRaw(0);
    for (int i = 0; i < 80; i++) gAna.debugStep();     // fall to the bottom rail
    check("bottom rail reached", gAna.value() < 200 && gAna.smoothed() < 100);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
