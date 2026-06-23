

# File AnalogMultiPos.h

[**File List**](files.md) **>** [**AnalogMultiPos**](dir_869066037a4dfe81b59e09c740cc62d3.md) **>** [**AnalogMultiPos.h**](AnalogMultiPos_8h.md)

[Go to the documentation of this file](AnalogMultiPos_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>                          // PinRef
#include <Inputs/MultiPosInput/MultiPosInput.h>  // MultiPosInput base

namespace OpenSkyhawk {

static constexpr uint16_t ANALOG_NC = MultiPosInput::NO_POSITION;

class AnalogMultiPos : public MultiPosInput {
public:
    static constexpr uint16_t DEFAULT_DEADBAND = 1000;  
    static constexpr uint16_t POLL_MS          = 8;     

    AnalogMultiPos(uint16_t controlId, PinRef pin, uint8_t numPos,
                   const uint16_t* posVals, uint16_t deadband = DEFAULT_DEADBAND);

    AnalogMultiPos(uint16_t controlId, PinRef pin, uint8_t numPos,
                   uint16_t deadband = DEFAULT_DEADBAND);

    void configure() override;

    void forceReport() override;

#ifdef ANALOGMULTIPOS_TEST
    uint16_t debugResolve(uint16_t raw) const { return resolve(raw); }
    void debugSetRaw(uint16_t raw) { _testRaw = raw; _testRawSet = true; _forceRead = true; }
#endif

protected:
    uint16_t readRaw() override;   // POLL_MS-throttled ADC read → resolve()

private:
    uint16_t resolve(uint16_t raw) const;   
    uint16_t posValAt(uint8_t i) const;     
    bool     isValid(uint8_t i) const;      

    PinRef          _pin;
    const uint16_t* _posVals;     
    uint16_t        _deadband;
    uint16_t        _cachedIdx;   
    uint32_t        _lastReadMs;
    bool            _forceRead = true;  
#ifdef ANALOGMULTIPOS_TEST
    uint16_t        _testRaw    = 0;
    bool            _testRawSet = false;
#endif
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


