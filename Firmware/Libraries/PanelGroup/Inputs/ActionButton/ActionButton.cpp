/**
 * @file ActionButton.cpp
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#ifdef ARDUINO_ARCH_STM32

#include "ActionButton.h"
#include <CANProtocol.h>  // sendBatched, canIdEvtAction, ControlPacket
#include <STM32Board.h>

namespace OpenSkyhawk {

/** @brief Payload selector for the DCS-BIOS `TOGGLE` action argument. */
static constexpr uint16_t ACTION_TOGGLE = 0;

ActionButton::ActionButton(uint16_t controlId, PinRef pin, bool reverse)
    : _controlId(controlId),
      _pin(pin),
      _reverse(reverse),
      _lastPressed(false),
      _lastFireMs(0),
      _initialized(false) {}

bool ActionButton::readPressed() const {
    return _reverse ? _pin.read() : !_pin.read();
}

void ActionButton::configure() {
    _pin.configureAsInput();
}

void ActionButton::forceReport() {
    // Deliberately silent — emitting here would flip the sim switch on every SYNC_REQ.
    // Seeding the baseline is the point: a button held at boot must not read as a press edge.
    _lastPressed = readPressed();
    _initialized = true;

    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[ACT] 0x")); d.print(_controlId, HEX);
        d.print(F(": baseline ")); d.print(_lastPressed ? F("pressed") : F("released"));
        d.println(F(" (no emit)"));
    }
}

void ActionButton::poll() {
    if (!_initialized) return;

    // Post-fire suppression, not a stability window: the press edge is the event, so waiting for
    // the level to settle would only add latency. Elapsed-time compare is wrap-correct.
    if (millis() - _lastFireMs < DEBOUNCE_MS) return;

    const bool pressed = readPressed();
    if (pressed == _lastPressed) return;   // no edge — covers the whole hold, however long

    if (pressed) {                         // press edge only; release just re-arms
        CANProtocol::sendBatched(canIdEvtAction(NODE_ID),
                                 ControlPacket{_controlId, ACTION_TOGGLE});
        _lastFireMs = millis();

        if (STM32Board::isDebug()) {
            auto& d = STM32Board::diagSerial();
            d.print(F("[ACT] 0x")); d.print(_controlId, HEX);
            d.println(F(": TOGGLE"));
        }
    }
    _lastPressed = pressed;
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
