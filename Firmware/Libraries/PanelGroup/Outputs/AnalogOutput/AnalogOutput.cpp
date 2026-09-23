#ifdef ARDUINO_ARCH_STM32

#include "AnalogOutput.h"

OpenSkyhawk::AnalogOutput::AnalogOutput(uint16_t controlId, uint16_t mask, uint8_t shift)
    : _controlId(controlId), _mask(mask), _shift(shift) {}

void OpenSkyhawk::AnalogOutput::onControlPacket(uint16_t controlId, uint16_t value) {
    if (controlId != _controlId) return;
    const uint16_t v = (uint16_t)((value & _mask) >> _shift);
    if (_hasValue && v == _lastValue) return;   // change dedup — SYNC_REQ re-broadcasts full state
    _lastValue = v;
    _hasValue  = true;
    apply(v);
}

#endif // ARDUINO_ARCH_STM32
