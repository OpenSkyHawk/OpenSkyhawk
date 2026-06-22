

# File SwitchMultiPos.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**SwitchMultiPos**](dir_4dc253f801dfeffbf99c560a0635ade6.md) **>** [**SwitchMultiPos.h**](SwitchMultiPos_8h.md)

[Go to the documentation of this file](SwitchMultiPos_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                          // PinRef
#include <Inputs/MultiPosInput/MultiPosInput.h>  // MultiPosInput base

namespace OpenSkyhawk {

class SwitchMultiPos : public MultiPosInput {
public:
    static constexpr uint16_t DEBOUNCE_MS = 20;

    SwitchMultiPos(uint16_t controlId, const PinRef* pins, uint8_t numPins, bool reverse = false);

    void configure() override;

protected:
    uint16_t readRaw() override;

private:
    const PinRef* _pins;     
    uint8_t       _numPins;  
    bool          _reverse;  
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


