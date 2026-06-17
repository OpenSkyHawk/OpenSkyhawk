// STM32Board — animation timing test (STM32BOARD_TEST)
//
// Confirms the NEW solid state (CONNECTED) does not blink, while NORMAL still toggles —
// i.e. _recompute() does not reset the blink phase when the state is unchanged, and
// _blinkPeriodFor(CONNECTED) == 0. Reads the LED pins directly (push-pull output level
// is reflected by digitalRead) across >1 s windows.

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
    auto& d = STM32Board::diagSerial();
    d.println(F("=== animation_timing ==="));

    // CONNECTED = solid green. With continuous link refresh, green stays HIGH and red LOW
    // across >1 s — no toggling.
    STM32Board::onCanStatus(CanStatus::NORMAL);
    STM32Board::setLinkActive(true);
    bool greenAlwaysHigh = true, redAlwaysLow = true;
    uint32_t t0 = millis();
    while (millis() - t0 < 1200) {
        STM32Board::setLinkActive(true);  // refresh so it never decays
        STM32Board::tick();
        if (digitalRead(STM32Board::PIN_LED_GREEN) != HIGH) greenAlwaysHigh = false;
        if (digitalRead(STM32Board::PIN_LED_RED)   != LOW)  redAlwaysLow   = false;
    }
    check("CONNECTED state held", STM32Board::currentState() == LedState::CONNECTED);
    check("CONNECTED green solid HIGH", greenAlwaysHigh);
    check("CONNECTED red off", redAlwaysLow);

    // NORMAL = slow green blink (~1000 ms). Over >1 s green must show both HIGH and LOW;
    // red stays off.
    STM32Board::setLinkActive(false);
    STM32Board::onCanStatus(CanStatus::NORMAL);
    bool sawHigh = false, sawLow = false, redLow = true;
    uint32_t t1 = millis();
    while (millis() - t1 < 1200) {
        STM32Board::tick();
        if (digitalRead(STM32Board::PIN_LED_GREEN) == HIGH) sawHigh = true; else sawLow = true;
        if (digitalRead(STM32Board::PIN_LED_RED)   != LOW)  redLow  = false;
    }
    check("NORMAL green blinks (HIGH seen)", sawHigh);
    check("NORMAL green blinks (LOW seen)",  sawLow);
    check("NORMAL red off", redLow);

    summary();
}

void loop() {}
