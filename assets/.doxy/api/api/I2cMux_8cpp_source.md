

# File I2cMux.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md) **>** [**I2cMux**](dir_b0e3ddf276daac85bddb20c46644a5c8.md) **>** [**I2cMux.cpp**](I2cMux_8cpp.md)

[Go to the documentation of this file](I2cMux_8cpp.md)


```C++

#ifdef ARDUINO_ARCH_STM32

#include "I2cMux.h"

namespace OpenSkyhawk {

I2cMux::I2cMux(uint8_t addr, TwoWire& wire)
    : _addr(addr), _wire(&wire), _lastChannel(-1) {}

bool I2cMux::select(uint8_t channel, bool force) {
    if (channel > 7) channel = 7;
    if (!force && static_cast<int8_t>(channel) == _lastChannel) return true;  // skip redundant write (unless forced)
    _wire->beginTransmission(_addr);
    _wire->write(static_cast<uint8_t>(1u << channel));
    bool ok = (_wire->endTransmission() == 0);
    _lastChannel = ok ? static_cast<int8_t>(channel) : -1;  // invalidate cache on NAK → next select re-routes (mux reset)
    return ok;
}

void I2cMux::disableAll() {
    _wire->beginTransmission(_addr);
    _wire->write(static_cast<uint8_t>(0x00));
    _wire->endTransmission();
    _lastChannel = -1;
}

bool I2cMux::deviceAcks(uint8_t addr7) {
    _wire->beginTransmission(addr7);
    return _wire->endTransmission() == 0;
}

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


