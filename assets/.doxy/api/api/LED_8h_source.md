

# File LED.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**LED.h**](LED_8h.md)

[Go to the documentation of this file](LED_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // OutputBase, PinRef

namespace OpenSkyhawk {

class LED : public OutputBase {
public:
    LED(uint16_t controlId, uint16_t mask, PinRef pin, bool reverse = false);

    void configure() override;

    void onControlPacket(uint16_t controlId, uint16_t value) override;

private:
    uint16_t _controlId;
    uint16_t _mask;
    PinRef   _pin;
    bool     _reverse;   
    bool     _lastOn   = false;
    bool     _hasState = false;  
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
```


