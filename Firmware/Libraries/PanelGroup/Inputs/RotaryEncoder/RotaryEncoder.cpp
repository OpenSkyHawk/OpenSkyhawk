/**
 * @file RotaryEncoder.cpp
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#ifdef ARDUINO_ARCH_STM32

#include "RotaryEncoder.h"
#include <CANProtocol.h>  // sendBatched, canIdEvt, ControlPacket
#include <STM32Board.h>

namespace OpenSkyhawk {

RotaryEncoder::RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             StepsPerDetent stepsPerDetent)
    : _controlId(controlId),
      _pinA(pinA),
      _pinB(pinB),
      _stepsPerDetent((uint8_t)stepsPerDetent),
      _lastState(0),
      _delta(0),
      _initialized(false) {}

void RotaryEncoder::configure() {
    _pinA.configureAsInput();
    _pinB.configureAsInput();
}

uint8_t RotaryEncoder::readState() {
    return (uint8_t)(((_pinA.read() ? 1u : 0u) << 1) | (_pinB.read() ? 1u : 0u));
}

void RotaryEncoder::emit(uint16_t dir) {
    CANProtocol::sendBatched(canIdEvt(NODE_ID), ControlPacket{_controlId, dir});
#ifdef ROTARYENCODER_TEST
    _emitCount++;
    _lastDir = (int8_t)dir;
#endif
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[ENC] 0x")); d.print(_controlId, HEX);
        d.print(F(": "));       d.println(dir);   // 1 = CW, 0 = CCW
    }
}

void RotaryEncoder::decode(uint8_t state) {
    // Quadrature transition table — ported verbatim from DcsBios RotaryEncoder (Encoders.h).
    switch (_lastState) {
        case 0: if (state == 2) _delta--; if (state == 1) _delta++; break;
        case 1: if (state == 0) _delta--; if (state == 3) _delta++; break;
        case 2: if (state == 3) _delta--; if (state == 0) _delta++; break;
        case 3: if (state == 1) _delta--; if (state == 2) _delta++; break;
    }
    _lastState = state;

    if (_delta >= (int8_t)_stepsPerDetent) {       // clockwise
        emit(1);
        _delta -= (int8_t)_stepsPerDetent;
    }
    if (_delta <= -(int8_t)_stepsPerDetent) {      // counter-clockwise
        emit(0);
        _delta += (int8_t)_stepsPerDetent;
    }
}

void RotaryEncoder::forceReport() {
    _lastState   = readState();   // resync so the first poll sees no spurious transition
    _delta       = 0;
    _initialized = true;
    // No EVT — a relative encoder has no absolute state to report at boot / SYNC.
}

void RotaryEncoder::poll() {
    if (!_initialized) return;
    decode(readState());
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
