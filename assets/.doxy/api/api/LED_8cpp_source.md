

# File LED.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**LED.cpp**](LED_8cpp.md)

[Go to the documentation of this file](LED_8cpp.md)


```C++
#ifdef ARDUINO_ARCH_STM32

#include "LED.h"

OpenSkyhawk::LED::LED(uint16_t controlId, uint16_t mask, PinRef pin, bool reverse)
    : _controlId(controlId), _mask(mask), _pin(pin), _reverse(reverse) {}

void OpenSkyhawk::LED::configure() {
    _pin.configureAsOutput();
    _pin.write(_reverse); // reverse=false → LOW (off); reverse=true → HIGH (off)
}

void OpenSkyhawk::LED::onControlPacket(uint16_t controlId, uint16_t value) {
    if (controlId != _controlId) return;
    bool on = (value & _mask) != 0;
    _pin.write(_reverse ? !on : on);
}

#endif // ARDUINO_ARCH_STM32
```


