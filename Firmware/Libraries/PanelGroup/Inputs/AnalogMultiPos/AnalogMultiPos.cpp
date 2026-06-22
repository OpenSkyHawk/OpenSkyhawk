/**
 * @file AnalogMultiPos.cpp
 * @copyright GPL-2.0-only — see Firmware/LICENSE
 */

#ifdef ARDUINO_ARCH_STM32

#include "AnalogMultiPos.h"

namespace OpenSkyhawk {

AnalogMultiPos::AnalogMultiPos(uint16_t controlId, PinRef pin, uint8_t numPos,
                               const uint16_t* posVals, uint16_t deadband)
    : MultiPosInput(controlId, numPos, 0),   // base debounce = 0; deadband gaps do the filtering
      _pin(pin),
      _posVals(posVals),
      _deadband(deadband),
      _cachedIdx(NO_POSITION),
      _lastReadMs(0) {}

AnalogMultiPos::AnalogMultiPos(uint16_t controlId, PinRef pin, uint8_t numPos, uint16_t deadband)
    : MultiPosInput(controlId, numPos, 0),
      _pin(pin),
      _posVals(nullptr),                       // equal-spacing
      _deadband(deadband),
      _cachedIdx(NO_POSITION),
      _lastReadMs(0) {}

void AnalogMultiPos::configure() {
    _pin.configureAsInput();
}

uint16_t AnalogMultiPos::posValAt(uint8_t i) const {
    if (_posVals) return _posVals[i];
    if (_numPositions <= 1) return 0;
    return static_cast<uint16_t>(static_cast<uint32_t>(i) * 65535u / (_numPositions - 1));
}

bool AnalogMultiPos::isValid(uint8_t i) const {
    return !_posVals || _posVals[i] != ANALOG_NC;
}

uint16_t AnalogMultiPos::resolve(uint16_t raw) const {
    for (uint8_t i = 0; i < _numPositions; i++) {
        if (!isValid(i)) continue;

        // Band edges: half-way to each nearest VALID neighbour, trimmed by the deadband.
        int32_t lo = 0, hi = 65535;
        for (int16_t p = static_cast<int16_t>(i) - 1; p >= 0; p--) {
            if (isValid(static_cast<uint8_t>(p))) {
                lo = (static_cast<int32_t>(posValAt(static_cast<uint8_t>(p))) + posValAt(i)) / 2 + _deadband;
                break;
            }
        }
        for (uint8_t n = i + 1; n < _numPositions; n++) {
            if (isValid(n)) {
                hi = (static_cast<int32_t>(posValAt(i)) + posValAt(n)) / 2 - _deadband;
                break;
            }
        }

        if (static_cast<int32_t>(raw) >= lo && static_cast<int32_t>(raw) <= hi) return i;
    }
    return NO_POSITION;   // in a deadband gap → base holds the last position (hysteresis)
}

uint16_t AnalogMultiPos::readRaw() {
    uint32_t now = millis();
    if (now - _lastReadMs >= POLL_MS) {
        _lastReadMs = now;
        uint16_t raw =
#ifdef ANALOGMULTIPOS_TEST
            _testRawSet ? _testRaw :
#endif
            _pin.readAnalog();
        _cachedIdx = resolve(raw);
    }
    return _cachedIdx;
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
