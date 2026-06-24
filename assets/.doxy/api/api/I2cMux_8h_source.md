

# File I2cMux.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Helpers**](dir_9e93d9a1721bcf27b2030ff612e0fc11.md) **>** [**I2cMux**](dir_b0e3ddf276daac85bddb20c46644a5c8.md) **>** [**I2cMux.h**](I2cMux_8h.md)

[Go to the documentation of this file](I2cMux_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <Wire.h>

namespace OpenSkyhawk {

class I2cMux {
public:
    explicit I2cMux(uint8_t addr = 0x70, TwoWire& wire = Wire);

    bool select(uint8_t channel, bool force = false);

    void disableAll();

    bool deviceAcks(uint8_t addr7);

private:
    uint8_t  _addr;         // TCA9548A 7-bit address
    TwoWire* _wire;         // bus the mux is on
    int8_t   _lastChannel;  // -1 = nothing selected yet; cache to skip redundant writes
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


