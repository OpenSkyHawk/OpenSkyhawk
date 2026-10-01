

# File AngleSensorInput.h

[**File List**](files.md) **>** [**AngleSensorInput**](dir_204c2e67ee983494592248b9f39f712d.md) **>** [**AngleSensorInput.h**](AngleSensorInput_8h.md)

[Go to the documentation of this file](AngleSensorInput_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Inputs/AnalogInput/AnalogInput.h>  // family base (InputBase, PinRef via PanelGroup.h)

namespace OpenSkyhawk {

class AngleSensorInput : public AnalogInput {
public:
    static constexpr float COUNTS_PER_DEG = 65536.0f / 360.0f;

    AngleSensorInput(uint16_t controlId, PinRef pin, float centerDeg, float travelDeg,
                     uint16_t pollMs = DEFAULT_POLL_MS);

protected:
    uint16_t readRaw() override;

private:
    uint16_t _centerRaw;   
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


