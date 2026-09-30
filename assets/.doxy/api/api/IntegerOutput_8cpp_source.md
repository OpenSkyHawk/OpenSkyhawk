

# File IntegerOutput.cpp

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Outputs**](dir_529c528362a647a34d31d0b3b420ca72.md) **>** [**IntegerOutput**](dir_751c3ce74b607cd528cf65a1346ab5a9.md) **>** [**IntegerOutput.cpp**](IntegerOutput_8cpp.md)

[Go to the documentation of this file](IntegerOutput_8cpp.md)


```C++
#ifdef ARDUINO_ARCH_STM32

#include "IntegerOutput.h"

OpenSkyhawk::IntegerOutput::IntegerOutput(uint16_t controlId, Callback callback,
                                          uint16_t mask, uint8_t shift)
    : AnalogOutput(controlId, mask, shift), _callback(callback) {}

void OpenSkyhawk::IntegerOutput::apply(uint16_t value) {
    if (_callback) _callback(value);
}

#endif // ARDUINO_ARCH_STM32
```


