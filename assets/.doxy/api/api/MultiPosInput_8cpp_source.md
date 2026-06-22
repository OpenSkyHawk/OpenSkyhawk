

# File MultiPosInput.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**MultiPosInput**](dir_7bc1eaced50854697a5557e9b0a7cd3c.md) **>** [**MultiPosInput.cpp**](MultiPosInput_8cpp.md)

[Go to the documentation of this file](MultiPosInput_8cpp.md)


```C++

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
#ifdef MULTIPOS_TEST
    _emitCount++;
#endif
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
```


