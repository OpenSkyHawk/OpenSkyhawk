/**
 * @file Switch3Pos.cpp
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#ifdef ARDUINO_ARCH_STM32

#include "Switch3Pos.h"

namespace OpenSkyhawk {

Switch3Pos::Switch3Pos(uint16_t controlId, PinRef pinA, PinRef pinB, bool reverse,
                       uint16_t debounceMs)
    : MultiPosInput(controlId, 3, debounceMs),
      _pinA(pinA),
      _pinB(pinB),
      _reverse(reverse) {}

void Switch3Pos::configure() {
    _pinA.configureAsInput();
    _pinB.configureAsInput();
}

uint16_t Switch3Pos::readRaw() {
    bool a = _reverse ? _pinA.read() : !_pinA.read();
    bool b = _reverse ? _pinB.read() : !_pinB.read();
    if (a) return 0;   // pin A priority — matches DcsBios both-active behaviour
    if (b) return 2;
    return 1;          // centre — a real position, never NO_POSITION
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
