// AnalogInput — shift_bounds test
//
// ewmaShift is capped at MAX_EWMA_SHIFT (15) so forceReport's seed `_acc = scaled << ewmaShift`
// cannot overflow the int32 accumulator at full scale (65535 << 16 > INT32_MAX). An out-of-range
// shift is clamped; a full-scale reading must still report 65535, not garbage from an overflow.
//
// Rig: this STM32 on the CAN bus with the PanelBridge (node ACKs). No jumpers / pot needed.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogInput/AnalogInput.h>

static constexpr uint16_t CTRL_ID = 0x567A;

// ewmaShift = 20 → clamped to MAX_EWMA_SHIFT (15).
OpenSkyhawk::AnalogInput gAna(CTRL_ID, PinRef(PA1), false, 0, 65535, 128, /*ewmaShift=*/20);

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogInput shift_bounds ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    gAna.configure();
    CANProtocol::start();

    // forceReport seeds _acc = scaled << ewmaShift — the overflow boundary. Clamped shift → 65535.
    gAna.debugSetRaw(65535); gAna.forceReport();
    check("clamped shift (20->15), full-scale -> 65535 (no overflow)",
          gAna.value() == 65535 && gAna.smoothed() == 65535);

    gAna.debugSetRaw(0); gAna.forceReport();
    check("clamped shift, zero -> 0", gAna.value() == 0 && gAna.smoothed() == 0);

    // Steady full-scale: many EWMA steps at the seed value stay pinned at 65535 (no acc drift).
    gAna.debugSetRaw(65535); gAna.forceReport();
    for (int i = 0; i < 50; i++) gAna.debugStep();
    check("steady full-scale at capped shift: stays 65535", gAna.smoothed() == 65535);

    CANProtocol::flushBatched(canIdEvt(NODE_ID));
    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
