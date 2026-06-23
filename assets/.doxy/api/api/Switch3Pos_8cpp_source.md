

# File Switch3Pos.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**Switch3Pos**](dir_344ec6ab1ff9af7411f07538fed6b206.md) **>** [**Switch3Pos.cpp**](Switch3Pos_8cpp.md)

[Go to the documentation of this file](Switch3Pos_8cpp.md)


```C++

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
```


