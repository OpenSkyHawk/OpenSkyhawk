/**
 * @file MultiPosInput.cpp
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#ifdef ARDUINO_ARCH_STM32

#include "MultiPosInput.h"
#include <CANProtocol.h>  // sendBatched, canIdEvt, ControlPacket
#include <STM32Board.h>

namespace OpenSkyhawk {

MultiPosInput::MultiPosInput(uint16_t controlId, uint8_t numPositions, uint16_t debounceMs)
    : _controlId(controlId),
      _numPositions(numPositions),
      _debounceMs(debounceMs),
      _lastPos(0),
      _pendingPos(0),
      _debounceStartMs(0),
      _initialized(false) {}

void MultiPosInput::emit(uint16_t pos, bool init) {
    CANProtocol::sendBatched(canIdEvt(NODE_ID),
                             ControlPacket{_controlId, pos});
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[MUL] 0x")); d.print(_controlId, HEX);
        d.print(F(": "));       d.print(pos);
        if (init) d.print(F(" (init)"));
        d.println();
    }
}

void MultiPosInput::forceReport() {
    uint16_t raw = readRaw();
    if (raw == NO_POSITION) raw = 0;   // nothing active at boot → default to position 0
    _lastPos         = raw;
    _pendingPos      = raw;
    _debounceStartMs = millis();
    _initialized     = true;
    emit(raw, true);
}

void MultiPosInput::poll() {
    if (!_initialized) return;

    uint16_t raw = readRaw();
    if (raw == NO_POSITION) raw = _lastPos;   // hold last confirmed position

    if (raw != _pendingPos) {
        _pendingPos      = raw;
        _debounceStartMs = millis();
    } else if (_pendingPos != _lastPos &&
               millis() - _debounceStartMs >= _debounceMs) {
        _lastPos = _pendingPos;
        emit(_lastPos);
    }
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
