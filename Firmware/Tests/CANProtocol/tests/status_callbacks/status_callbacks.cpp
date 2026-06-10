// CANProtocol — status callback test
//
// onStatusChange fires STARTING immediately on registration (before start()).
// Fires NORMAL immediately after start().
// Does not fire again without a status change.
//
// All checks run in setup() — results printed immediately.
//
// Expected output:
//   STARTING fired on register: PASS
//   Count after register (1):   PASS
//   NORMAL fired after start(): PASS
//   Count after start (2):      PASS

#include <STM32Board.h>
#include <CANProtocol.h>

static int       _callCount    = 0;
static bool      _startingFired = false;
static bool      _normalFired   = false;

static void onStatus(CanStatus s) {
    _callCount++;
    if (s == CanStatus::STARTING) _startingFired = true;
    if (s == CanStatus::NORMAL)   _normalFired   = true;
}

void setup() {
    STM32Board::setDebug(true);
    STM32Board::begin();
    STM32Board::log("=== status_callbacks ===");

    CANProtocol::onStatusChange(onStatus);     // fires STARTING immediately
    int countAfterRegister = _callCount;       // expect 1

    CANProtocol::filterAcceptAll();
    CANProtocol::startLoopback();              // fires NORMAL immediately
    int countAfterStart = _callCount;          // expect 2

    auto& d = STM32Board::diagSerial();
    d.println(F("--- status_callbacks results ---"));
    d.print(F("STARTING fired on register: ")); d.println(_startingFired           ? F("PASS") : F("FAIL"));
    d.print(F("Count after register (1):   ")); d.println(countAfterRegister == 1  ? F("PASS") : F("FAIL"));
    d.print(F("NORMAL fired after start(): ")); d.println(_normalFired             ? F("PASS") : F("FAIL"));
    d.print(F("Count after start (2):      ")); d.println(countAfterStart == 2     ? F("PASS") : F("FAIL"));
}

void loop() {
    STM32Board::tick();
    CANProtocol::drain();
}
