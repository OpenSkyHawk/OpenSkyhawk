

# File AnalogOutput.h

[**File List**](files.md) **>** [**AnalogOutput**](dir_7be86be934beb3a22c91bb10ef55a8df.md) **>** [**AnalogOutput.h**](AnalogOutput_8h.md)

[Go to the documentation of this file](AnalogOutput_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // OutputBase

namespace OpenSkyhawk {

class AnalogOutput : public OutputBase {
public:
    void onControlPacket(uint16_t controlId, uint16_t value) final;

protected:
    AnalogOutput(uint16_t controlId, uint16_t mask = 0xFFFF, uint8_t shift = 0);

    virtual void apply(uint16_t value) = 0;

private:
    uint16_t _controlId;
    uint16_t _mask;
    uint8_t  _shift;
    uint16_t _lastValue = 0;      
    bool     _hasValue  = false;  
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
```


