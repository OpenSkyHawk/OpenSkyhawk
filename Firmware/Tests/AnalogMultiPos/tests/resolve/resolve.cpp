// AnalogMultiPos — resolve test
//
// Explicit ladder {4000, 16000, 42000, 56000} (N=4, all valid, deadband=1000). Asserts the band
// math via debugResolve() — pure logic, no CAN, no hardware:
//   posVal → its index; band edges; deadband gap → NO_POSITION; range ends.
//
// Bands (deadband 1000): 0=[0,9000] 1=[11000,28000] 2=[30000,48000] 3=[50000,65535];
// gaps at the midpoints (10000, 29000, 49000) ± 1000.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogMultiPos/AnalogMultiPos.h>

using namespace OpenSkyhawk;

static constexpr uint16_t CTRL_ID = 0x803b;
static const uint16_t POSVALS[] = { 4000, 16000, 42000, 56000 };
AnalogMultiPos gSel(CTRL_ID, PinRef(PA1), 4, POSVALS);

static constexpr uint16_t NOPOS = MultiPosInput::NO_POSITION;   // 'NC' collides with STM32 PinName

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogMultiPos resolve ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // Each detent value resolves to its index.
    check("4000 -> 0",  gSel.debugResolve(4000)  == 0);
    check("16000 -> 1", gSel.debugResolve(16000) == 1);
    check("42000 -> 2", gSel.debugResolve(42000) == 2);
    check("56000 -> 3", gSel.debugResolve(56000) == 3);

    // Range ends clamp to the outer positions.
    check("0 -> 0",     gSel.debugResolve(0)     == 0);
    check("65535 -> 3", gSel.debugResolve(65535) == 3);

    // Boundary at midpoint 10000: band0 ends 9000, band1 starts 11000, gap between.
    check("9000 -> 0 (band0 edge)",       gSel.debugResolve(9000)  == 0);
    check("10000 -> NO_POSITION (gap)",   gSel.debugResolve(10000) == NOPOS);
    check("11000 -> 1 (band1 edge)",      gSel.debugResolve(11000) == 1);

    // A gap at the top boundary 49000 too.
    check("49000 -> NO_POSITION (gap)",   gSel.debugResolve(49000) == NOPOS);
    check("50000 -> 3 (band3 edge)",      gSel.debugResolve(50000) == 3);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
