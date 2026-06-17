// STM32Board — CONNECTED link-decay test (STM32BOARD_TEST)
//
// setLinkActive(true) enters CONNECTED; with no further calls the link decays back to
// NORMAL after LINK_DECAY_MS (500 ms), evaluated inside tick(). A refresh inside the
// window holds CONNECTED. Uses real millis() — the windows are ≤1 s so the test finishes
// in a couple of seconds on-target.

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
    d.println(F("=== link_decay ==="));

    STM32Board::setLinkActive(true);
    check("link set -> CONNECTED", STM32Board::currentState() == LedState::CONNECTED);

    // Hold within the 500 ms window (no refresh) — must stay CONNECTED.
    uint32_t t0 = millis();
    while (millis() - t0 < 200) STM32Board::tick();
    check("held <500ms -> still CONNECTED", STM32Board::currentState() == LedState::CONNECTED);

    // Cross the decay threshold — must fall back to NORMAL.
    while (millis() - t0 < 700) STM32Board::tick();
    check("idle >500ms -> decays to NORMAL", STM32Board::currentState() == LedState::NORMAL);

    // Continuous refresh defeats decay — stays CONNECTED across >500 ms.
    STM32Board::setLinkActive(true);
    uint32_t t1 = millis();
    while (millis() - t1 < 700) { STM32Board::setLinkActive(true); STM32Board::tick(); }
    check("continuous refresh -> stays CONNECTED", STM32Board::currentState() == LedState::CONNECTED);

    summary();
}

void loop() {}
