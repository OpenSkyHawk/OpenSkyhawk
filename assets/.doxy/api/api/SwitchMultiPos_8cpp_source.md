

# File SwitchMultiPos.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**SwitchMultiPos**](dir_4dc253f801dfeffbf99c560a0635ade6.md) **>** [**SwitchMultiPos.cpp**](SwitchMultiPos_8cpp.md)

[Go to the documentation of this file](SwitchMultiPos_8cpp.md)


```C++

#ifdef ARDUINO_ARCH_STM32

#include "SwitchMultiPos.h"

namespace OpenSkyhawk {

SwitchMultiPos::SwitchMultiPos(uint16_t controlId, const PinRef* pins, uint8_t numPins, bool reverse)
    : MultiPosInput(controlId, numPins, DEBOUNCE_MS),
      _pins(pins),
      _numPins(numPins),
      _reverse(reverse) {}

void SwitchMultiPos::configure() {
    // The pin array is caller-owned and const (it is the sketch wiring map). configureAsInput()
    // is non-const but only configures hardware — it mutates no PinRef state — so configure a
    // local copy of each pin, which addresses the same GPIO / MCP23017 channel.
    for (uint8_t i = 0; i < _numPins; i++) {
        PinRef p = _pins[i];
        if (!p.isNC()) p.configureAsInput();
    }
}

uint16_t SwitchMultiPos::readRaw() {
    uint16_t ncIdx = NO_POSITION;          // nothing active → base holds the last position
    for (uint8_t i = 0; i < _numPins; i++) {
        if (_pins[i].isNC()) {
            ncIdx = i;                     // mechanical-only detent → fallback position
            continue;
        }
        bool active = _reverse ? _pins[i].read() : !_pins[i].read();
        if (active) return i;              // one-hot: first active pin wins
    }
    return ncIdx;
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


