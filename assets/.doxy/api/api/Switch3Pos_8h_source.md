

# File Switch3Pos.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**Switch3Pos**](dir_344ec6ab1ff9af7411f07538fed6b206.md) **>** [**Switch3Pos.h**](Switch3Pos_8h.md)

[Go to the documentation of this file](Switch3Pos_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                          // PinRef
#include <Inputs/MultiPosInput/MultiPosInput.h>  // MultiPosInput base

namespace OpenSkyhawk {

class Switch3Pos : public MultiPosInput {
public:
    static constexpr uint16_t DEBOUNCE_MS = 20;   

    Switch3Pos(uint16_t controlId, PinRef pinA, PinRef pinB, bool reverse = false,
               uint16_t debounceMs = DEBOUNCE_MS);

    void configure() override;

protected:
    uint16_t readRaw() override;

private:
    PinRef _pinA;     
    PinRef _pinB;     
    bool   _reverse;  
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


