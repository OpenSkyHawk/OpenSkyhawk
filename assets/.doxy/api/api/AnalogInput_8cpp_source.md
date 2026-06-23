

# File AnalogInput.cpp

[**File List**](files.md) **>** [**AnalogInput**](dir_36f5dbe195072643095357faabfc57db.md) **>** [**AnalogInput.cpp**](AnalogInput_8cpp.md)

[Go to the documentation of this file](AnalogInput_8cpp.md)


```C++

#ifdef ARDUINO_ARCH_STM32

#include "AnalogInput.h"
#include <CANProtocol.h>  // sendBatched, canIdEvt, ControlPacket
#include <STM32Board.h>

namespace OpenSkyhawk {

AnalogInput::AnalogInput(uint16_t controlId, PinRef pin, bool reverse,
                         uint16_t minRaw, uint16_t maxRaw,
                         uint16_t hysteresis, uint8_t ewmaShift)
    : _controlId(controlId),
      _pin(pin),
      _reverse(reverse),
      _minRaw(minRaw),
      _maxRaw(maxRaw),
      _hysteresis(hysteresis),
      _ewmaShift(ewmaShift > MAX_EWMA_SHIFT ? MAX_EWMA_SHIFT : ewmaShift),
      _acc(0),
      _smoothed(0),
      _lastSent(0),
      _lastReadMs(0),
      _initialized(false) {}

void AnalogInput::configure() {
    _pin.configureAsInput();
}

uint16_t AnalogInput::readScaled() {
    uint16_t raw =
#ifdef ANALOGINPUT_TEST
        _testRawSet ? _testRaw :
#endif
        _pin.readAnalog();

    if (raw < _minRaw) raw = _minRaw;
    else if (raw > _maxRaw) raw = _maxRaw;

    // (raw - minRaw) ≤ 65535, × 65535 ≤ 4.29e9 → fits uint32; no Arduino map() overflow.
    uint32_t span   = (uint32_t)_maxRaw - _minRaw;
    uint16_t scaled = span ? (uint16_t)((uint32_t)(raw - _minRaw) * 65535u / span) : 0;
    return _reverse ? (uint16_t)(65535u - scaled) : scaled;
}

bool AnalogInput::shouldEmit(uint16_t v) const {
    // Ports DcsBios PotentiometerEWMA: emit on > hysteresis movement, or at a rail moving into it.
    return (_lastSent > v && (uint16_t)(_lastSent - v) > _hysteresis)
        || (v > _lastSent && (uint16_t)(v - _lastSent) > _hysteresis)
        || (v > (uint16_t)(65535u - _hysteresis) && v > _lastSent)
        || (v < _hysteresis && v < _lastSent);
}

void AnalogInput::emit(uint16_t v, bool init) {
    _lastSent = v;
    CANProtocol::sendBatched(canIdEvt(NODE_ID), ControlPacket{_controlId, v});
#ifdef ANALOGINPUT_TEST
    _emitCount++;
#endif
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[ANA] 0x")); d.print(_controlId, HEX);
        d.print(F(": "));       d.print(v);
        if (init) d.print(F(" (init)"));
        d.println();
    }
}

void AnalogInput::sample() {
    uint16_t scaled = readScaled();
    _acc     += (int32_t)scaled - (_acc >> _ewmaShift);   // integer EWMA: α = 1/2^ewmaShift
    _smoothed = (uint16_t)(_acc >> _ewmaShift);
    if (shouldEmit(_smoothed)) emit(_smoothed);
}

void AnalogInput::forceReport() {
    uint16_t scaled = readScaled();
    _acc         = (int32_t)scaled << _ewmaShift;   // seed EWMA so smoothed == the current reading
    _smoothed    = scaled;
    _lastReadMs  = millis();
    _initialized = true;
    emit(scaled, true);                             // baseline, unconditional
}

void AnalogInput::poll() {
    if (!_initialized) return;

    uint32_t now = millis();
    if (now - _lastReadMs < POLL_MS) return;        // throttle ADC reads
    _lastReadMs = now;
    sample();
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


