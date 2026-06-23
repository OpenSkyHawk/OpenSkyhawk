

# File AnalogInput.h

[**File List**](files.md) **>** [**AnalogInput**](dir_36f5dbe195072643095357faabfc57db.md) **>** [**AnalogInput.h**](AnalogInput_8h.md)

[Go to the documentation of this file](AnalogInput_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase, PinRef

namespace OpenSkyhawk {

class AnalogInput : public InputBase {
public:
    static constexpr uint16_t DEFAULT_HYSTERESIS = 128;  
    static constexpr uint8_t  DEFAULT_EWMA_SHIFT = 3;    
    static constexpr uint8_t  MAX_EWMA_SHIFT     = 15;   
    static constexpr uint16_t POLL_MS            = 8;    

    AnalogInput(uint16_t controlId, PinRef pin, bool reverse = false,
                uint16_t minRaw = 0, uint16_t maxRaw = 65535,
                uint16_t hysteresis = DEFAULT_HYSTERESIS, uint8_t ewmaShift = DEFAULT_EWMA_SHIFT);

    void poll() override;

    void forceReport() override;

    void configure() override;

#ifdef ANALOGINPUT_TEST
    void debugSetRaw(uint16_t raw) { _testRaw = raw; _testRawSet = true; }
    void debugStep() { sample(); }
    uint16_t value() const { return _lastSent; }
    uint16_t smoothed() const { return _smoothed; }
    uint16_t emitCount() const { return _emitCount; }
#endif

private:
    void     sample();                       
    uint16_t readScaled();                   
    bool     shouldEmit(uint16_t v) const;   
    void     emit(uint16_t v, bool init = false);

    uint16_t _controlId;
    PinRef   _pin;
    bool     _reverse;
    uint16_t _minRaw;
    uint16_t _maxRaw;
    uint16_t _hysteresis;
    uint8_t  _ewmaShift;
    int32_t  _acc;          
    uint16_t _smoothed;     
    uint16_t _lastSent;     
    uint32_t _lastReadMs;
    bool     _initialized;  
#ifdef ANALOGINPUT_TEST
    uint16_t _emitCount  = 0;
    uint16_t _testRaw    = 0;
    bool     _testRawSet = false;
#endif
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


