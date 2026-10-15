// Minimal PASS/FAIL reporter for the PowerMonitor tests — prints one line per check on DiagSerial
// and a final ALL PASS / FAIL count.
#pragma once
#include <STM32Board.h>

static int _checks = 0, _fails = 0;

static void check(const char* label, bool ok) {
    auto& d = STM32Board::diagSerial();
    _checks++;
    if (!ok) _fails++;
    d.print(label); d.println(ok ? F(": PASS") : F(": FAIL"));
}

static void checkEq(const char* label, long got, long want) {
    if (got != want) {
        auto& d = STM32Board::diagSerial();
        d.print(F("  got ")); d.print(got); d.print(F(" want ")); d.println(want);
    }
    check(label, got == want);
}

static void checkNear(const char* label, long got, long want, long tol) {
    const bool ok = got >= want - tol && got <= want + tol;
    if (!ok) {
        auto& d = STM32Board::diagSerial();
        d.print(F("  got ")); d.print(got); d.print(F(" want ")); d.print(want);
        d.print(F(" ±")); d.println(tol);
    }
    check(label, ok);
}

static void summary(const char* name) {
    auto& d = STM32Board::diagSerial();
    d.print(F("=== ")); d.print(name);
    if (_fails) { d.print(F(" FAIL ")); d.print(_fails); d.print('/'); d.print(_checks); d.println(F(" ===")); }
    else        { d.print(F(" ALL PASS (")); d.print(_checks); d.println(F(") ===")); }
}

static void begin(const char* name) {
    STM32Board::setDebug(true);
    STM32Board::begin();
    delay(200);
    auto& d = STM32Board::diagSerial();
    d.print(F("=== ")); d.print(name); d.println(F(" ==="));
}
