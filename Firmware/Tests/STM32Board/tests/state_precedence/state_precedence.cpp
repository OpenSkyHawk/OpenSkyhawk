// STM32Board — LED state precedence test (STM32BOARD_TEST)
//
// Drives the derived-state inputs (CanStatus, link-active, warning) in combination and
// asserts the effective LedState via STM32Board::currentState(). No CAN bus or second
// node required — onCanStatus()/setLinkActive()/setWarning() are called directly.
//
// Precedence (highest first):
//   BUS_OFF > CAN_ERROR > BOOTING > WARNING > CONNECTED > NORMAL
//
// All checks run in setup(); results print immediately over DiagSerial.

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
    STM32Board::begin();   // CanStatus seeded STARTING → BOOTING
    auto& d = STM32Board::diagSerial();
    d.println(F("=== state_precedence ==="));

    check("begin -> BOOTING", STM32Board::currentState() == LedState::BOOTING);

    STM32Board::onCanStatus(CanStatus::NORMAL);
    check("NORMAL, no data -> NORMAL", STM32Board::currentState() == LedState::NORMAL);

    STM32Board::setLinkActive(true);
    check("NORMAL + link -> CONNECTED", STM32Board::currentState() == LedState::CONNECTED);

    STM32Board::onCanStatus(CanStatus::TX_ERROR);
    check("TX_ERROR masks CONNECTED -> CAN_ERROR", STM32Board::currentState() == LedState::CAN_ERROR);

    STM32Board::onCanStatus(CanStatus::NORMAL);
    check("CAN recovers, link still live -> CONNECTED re-engages",
          STM32Board::currentState() == LedState::CONNECTED);

    STM32Board::setWarning(true);
    check("WARNING outranks CONNECTED", STM32Board::currentState() == LedState::WARNING);

    STM32Board::onCanStatus(CanStatus::BUS_OFF);
    check("BUS_OFF outranks WARNING", STM32Board::currentState() == LedState::BUS_OFF);

    STM32Board::onCanStatus(CanStatus::NORMAL);
    check("WARNING still set, masks CONNECTED -> WARNING",
          STM32Board::currentState() == LedState::WARNING);

    STM32Board::setWarning(false);
    check("clear WARNING, link live -> CONNECTED", STM32Board::currentState() == LedState::CONNECTED);

    STM32Board::setLinkActive(false);
    check("drop link -> NORMAL", STM32Board::currentState() == LedState::NORMAL);

    summary();
}

void loop() {}
