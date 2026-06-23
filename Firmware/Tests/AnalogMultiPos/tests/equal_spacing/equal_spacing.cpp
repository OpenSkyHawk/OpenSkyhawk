// AnalogMultiPos — equal_spacing test
//
// Shorthand ctor (no posVals): N=5 → positions evenly spaced 0..65535 → {0,16383,32767,49151,65535}.
// Pure debugResolve.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogMultiPos/AnalogMultiPos.h>

using namespace OpenSkyhawk;

static constexpr uint16_t CTRL_ID = 0x803b;
AnalogMultiPos gSel(CTRL_ID, PinRef(PA1), 5);   // shorthand — equal spacing

static constexpr uint16_t NOPOS = MultiPosInput::NO_POSITION;

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogMultiPos equal_spacing ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    check("0 -> 0",      gSel.debugResolve(0)     == 0);
    check("16383 -> 1",  gSel.debugResolve(16383) == 1);
    check("32767 -> 2",  gSel.debugResolve(32767) == 2);
    check("49151 -> 3",  gSel.debugResolve(49151) == 3);
    check("65535 -> 4",  gSel.debugResolve(65535) == 4);

    // Gap at the midpoint of positions 0 and 1 (8191) ± deadband.
    check("8191 -> NOPOS (gap)", gSel.debugResolve(8191) == NOPOS);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
