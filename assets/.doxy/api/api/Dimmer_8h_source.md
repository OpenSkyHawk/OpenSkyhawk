

# File Dimmer.h

[**File List**](files.md) **>** [**Dimmer**](dir_3d0eb80e711f6ad414d4aa2e54b5567f.md) **>** [**Dimmer.h**](Dimmer_8h.md)

[Go to the documentation of this file](Dimmer_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Outputs/AnalogOutput/AnalogOutput.h>  // family base (OutputBase, PinRef via PanelGroup.h)

namespace OpenSkyhawk {

class Dimmer : public AnalogOutput {
public:
    using ScaleFn = uint16_t (*)(uint16_t value);

    Dimmer(uint16_t controlId, PinRef pin, ScaleFn scale = nullptr);

    void configure() override;

#ifdef DIMMER_TEST
    uint16_t writeCount() const { return _writeCount; }
    uint8_t lastDuty() const { return _lastDuty; }
    bool enabled() const { return _enabled; }
#endif

protected:
    void apply(uint16_t value) override;

private:
    PinRef   _pin;
    ScaleFn  _scale;                
    bool     _enabled  = false;     
    uint8_t  _lastDuty = 0;         
    bool     _hasState = false;     
#ifdef DIMMER_TEST
    uint16_t _writeCount = 0;       
#endif
};

} // namespace OpenSkyhawk

#endif // ARDUINO_ARCH_STM32
```


