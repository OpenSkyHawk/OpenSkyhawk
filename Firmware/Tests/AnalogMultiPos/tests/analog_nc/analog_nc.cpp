// AnalogMultiPos — analog_nc test
//
// posVals {4000, 16000, ANALOG_NC, 42000, 56000}: position 2 has no detent. It is never emitted;
// its neighbours (1, 3) span its place, and positions 3/4 keep their indices. Pure debugResolve.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogMultiPos/AnalogMultiPos.h>

using namespace OpenSkyhawk;

static constexpr uint16_t CTRL_ID = 0x803b;
static const uint16_t POSVALS[] = { 4000, 16000, ANALOG_NC, 42000, 56000 };
AnalogMultiPos gSel(CTRL_ID, PinRef(PA1), 5, POSVALS);

static constexpr uint16_t NOPOS = MultiPosInput::NO_POSITION;

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogMultiPos analog_nc ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    check("4000 -> 0",  gSel.debugResolve(4000)  == 0);
    check("16000 -> 1", gSel.debugResolve(16000) == 1);
    check("42000 -> 3 (NC pos 2 skipped)", gSel.debugResolve(42000) == 3);
    check("56000 -> 4", gSel.debugResolve(56000) == 4);

    // Neighbours span the NC position (midpoint of 16000 & 42000 = 29000).
    check("28000 -> 1", gSel.debugResolve(28000) == 1);
    check("30000 -> 3", gSel.debugResolve(30000) == 3);
    check("29000 -> NOPOS (gap over NC)", gSel.debugResolve(29000) == NOPOS);

    // Index 2 is never returned for any reading.
    bool never2 = true;
    for (uint32_t v = 0; v <= 65535; v += 251)
        if (gSel.debugResolve(static_cast<uint16_t>(v)) == 2) { never2 = false; break; }
    check("index 2 (NC) never resolved", never2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
