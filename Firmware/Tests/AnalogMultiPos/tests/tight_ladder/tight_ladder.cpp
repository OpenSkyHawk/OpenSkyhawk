// AnalogMultiPos — tight_ladder test (hardening, audit #6)
//
// Detents closer than 2*deadband would invert the band edges (lo > hi) and become unreachable.
// resolve()'s clamp keeps each position's own value selectable. Here {1000, 2200, 3400} are
// 1200 counts apart — below the default 2*deadband (2000). Pure logic via debugResolve, no
// hardware.

#include <Arduino.h>
#include <STM32Board.h>
#include <Inputs/AnalogMultiPos/AnalogMultiPos.h>

static constexpr uint16_t CTRL_ID = 0x567C;
static const uint16_t POSVALS[] = { 1000, 2200, 3400 };   // 1200 apart < 2*deadband (1000)

OpenSkyhawk::AnalogMultiPos gSel(CTRL_ID, PinRef(PA1), 3, POSVALS);   // default deadband 1000

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::diagSerial().println("=== AnalogMultiPos tight_ladder ===");

    bool pass = true;
    auto check = [&](const char* label, bool ok) {
        if (!ok) pass = false;
        STM32Board::diagSerial().print(label);
        STM32Board::diagSerial().println(ok ? ": PASS" : ": FAIL");
    };

    // Each detent's own value resolves to its index despite the sub-2*deadband spacing
    // (without the clamp, position 1 would resolve to NO_POSITION).
    check("1000 -> 0 (tight, reachable)", gSel.debugResolve(1000) == 0);
    check("2200 -> 1 (tight, reachable)", gSel.debugResolve(2200) == 1);
    check("3400 -> 2 (tight, reachable)", gSel.debugResolve(3400) == 2);
    check("0 -> 0 (range start)", gSel.debugResolve(0) == 0);
    check("65535 -> 2 (range end)", gSel.debugResolve(65535) == 2);

    STM32Board::diagSerial().println(pass ? "=== ALL PASS ===" : "=== FAIL ===");
}

void loop() {}
