

# File LED.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Outputs**](dir_529c528362a647a34d31d0b3b420ca72.md) **>** [**LED**](dir_014b7653223add72b0ed2d7a88fd1566.md) **>** [**LED.cpp**](LED_8cpp.md)

[Go to the documentation of this file](LED_8cpp.md)


```C++
#ifdef ARDUINO_ARCH_STM32

#include "LED.h"
#include <STM32Board.h>

OpenSkyhawk::LED::LED(uint16_t controlId, uint16_t mask, PinRef pin, bool reverse)
    : _controlId(controlId), _mask(mask), _pin(pin), _reverse(reverse) {}

void OpenSkyhawk::LED::configure() {
    _pin.configureAsOutput();
    _pin.write(_reverse); // reverse=false → LOW (off); reverse=true → HIGH (off)
}

void OpenSkyhawk::LED::onControlPacket(uint16_t controlId, uint16_t value) {
    if (controlId != _controlId) return;
    bool on = (value & _mask) != 0;
    if (_hasState && on == _lastOn) return;
    _lastOn   = on;
    _hasState = true;
    _pin.write(_reverse ? !on : on);
    if (STM32Board::isDebug()) {
        auto& d = STM32Board::diagSerial();
        d.print(F("[LED] 0x")); d.print(_controlId, HEX);
        d.println(on ? F(": ON") : F(": OFF"));
    }
}

#endif // ARDUINO_ARCH_STM32
```


