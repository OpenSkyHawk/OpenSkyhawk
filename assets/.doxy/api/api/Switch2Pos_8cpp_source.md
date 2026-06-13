

# File Switch2Pos.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Switch2Pos.cpp**](Switch2Pos_8cpp.md)

[Go to the documentation of this file](Switch2Pos_8cpp.md)


```C++

#ifdef ARDUINO_ARCH_STM32

#include <Switch2Pos.h>
#include <CANProtocol.h>  // sendBatched, canIdEvt, ControlPacket

namespace OpenSkyhawk {

Switch2Pos::Switch2Pos(uint16_t controlId, PinRef pin)
    : Switch2Pos(controlId, pin, false) {}

Switch2Pos::Switch2Pos(uint16_t controlId, PinRef pin, bool reverse)
    : _controlId(controlId),
      _pin(pin),
      _reverse(reverse),
      _lastConfirmed(false),
      _pendingRaw(false),
      _debounceStartMs(0),
      _initialized(false) {}

void Switch2Pos::configure() {
    _pin.configureAsInput();
}

void Switch2Pos::forceReport() {
    bool current     = _reverse ? _pin.read() : !_pin.read();
    _lastConfirmed   = current;
    _pendingRaw      = current;
    _debounceStartMs = millis();
    _initialized     = true;
    CANProtocol::sendBatched(canIdEvt(NODE_ID),
                             ControlPacket{_controlId, static_cast<uint16_t>(current ? 1u : 0u)});
}

void Switch2Pos::poll() {
    if (!_initialized) return;

    bool raw = _reverse ? _pin.read() : !_pin.read();

    if (raw != _pendingRaw) {
        _pendingRaw      = raw;
        _debounceStartMs = millis();
    } else if (_pendingRaw != _lastConfirmed &&
               millis() - _debounceStartMs >= DEBOUNCE_MS) {
        _lastConfirmed = _pendingRaw;
        CANProtocol::sendBatched(canIdEvt(NODE_ID),
                                 ControlPacket{_controlId, static_cast<uint16_t>(_lastConfirmed ? 1u : 0u)});
    }
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


