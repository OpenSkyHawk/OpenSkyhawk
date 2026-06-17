// STM32Board — WARNING raise/clear test (STM32BOARD_TEST)
//
// setWarning(bool) is now a clearable latch (default arg keeps the legacy no-arg call
// raising). Verifies WARNING raises, clears back to the CAN-derived state (NORMAL or
// CONNECTED depending on link), and that the no-arg form still raises.

#include <STM32Board.h>
#include <CANProtocol.h>

static int _fails = 0;

static void check(const char* name, bool ok) {
    auto& d = STM32Board::diagSerial();
    d.print(ok ? F("PASS  ") : F("FAIL  "));
    d.println(name);
    if (!ok) _fails++;
}

static void summary() {
    auto& d = STM32Board::diagSerial();
    if (_fails == 0) d.println(F("=== ALL PASS ==="));
    else { d.print(F("=== ")); d.print(_fails); d.println(F(" FAILED ===")); }
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::onCanStatus(CanStatus::NORMAL);
    auto& d = STM32Board::diagSerial();
    d.println(F("=== warning_clear ==="));

    STM32Board::setWarning(true);
    check("setWarning(true) -> WARNING", STM32Board::currentState() == LedState::WARNING);

    STM32Board::setWarning(false);
    check("setWarning(false), no link -> NORMAL", STM32Board::currentState() == LedState::NORMAL);

    // WARNING over CONNECTED, then clearing falls back to CONNECTED (link still live).
    STM32Board::setLinkActive(true);
    STM32Board::setWarning(true);
    check("WARNING over CONNECTED", STM32Board::currentState() == LedState::WARNING);
    STM32Board::setWarning(false);
    check("clear -> CONNECTED (link live)", STM32Board::currentState() == LedState::CONNECTED);

    // Legacy no-arg setWarning() must still raise.
    STM32Board::setLinkActive(false);
    STM32Board::setWarning();
    check("no-arg setWarning() raises -> WARNING", STM32Board::currentState() == LedState::WARNING);
    STM32Board::setWarning(false);
    check("final clear -> NORMAL", STM32Board::currentState() == LedState::NORMAL);

    summary();
}

void loop() {}
