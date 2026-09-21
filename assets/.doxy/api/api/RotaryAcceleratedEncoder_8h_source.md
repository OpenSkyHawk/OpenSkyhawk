

# File RotaryAcceleratedEncoder.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**RotaryAcceleratedEncoder**](dir_d67d57cd87e8f385bd5b0c09cadb634b.md) **>** [**RotaryAcceleratedEncoder.h**](RotaryAcceleratedEncoder_8h.md)

[Go to the documentation of this file](RotaryAcceleratedEncoder_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Inputs/RotaryEncoder/RotaryEncoder.h>

namespace OpenSkyhawk {

class RotaryAcceleratedEncoder : public RotaryEncoder {
public:
    RotaryAcceleratedEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent, int16_t step, int16_t fastStep)
        : RotaryEncoder(controlId, pinA, pinB, stepsPerDetent, EncoderMode::Rel, step,
                        /*momentumFilter*/ true, fastStep) {}

    RotaryAcceleratedEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                             EncoderStepsPerDetent stepsPerDetent, EncoderMode mode)
        : RotaryEncoder(controlId, pinA, pinB, stepsPerDetent, mode, DEFAULT_STEP,
                        /*momentumFilter*/ true, /*fastStep*/ 0) {}
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


