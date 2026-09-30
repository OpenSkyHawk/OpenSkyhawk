

# File IntegerOutput.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Outputs**](dir_529c528362a647a34d31d0b3b420ca72.md) **>** [**IntegerOutput**](dir_751c3ce74b607cd528cf65a1346ab5a9.md) **>** [**IntegerOutput.h**](IntegerOutput_8h.md)

[Go to the documentation of this file](IntegerOutput_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Outputs/AnalogOutput/AnalogOutput.h>  // family base

namespace OpenSkyhawk {

class IntegerOutput : public AnalogOutput {
public:
    using Callback = void (*)(uint16_t value);

    IntegerOutput(uint16_t controlId, Callback callback, uint16_t mask = 0xFFFF, uint8_t shift = 0);

protected:
    void apply(uint16_t value) override;

private:
    Callback _callback;
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
```


