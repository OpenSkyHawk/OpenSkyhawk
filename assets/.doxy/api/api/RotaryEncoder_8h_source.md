

# File RotaryEncoder.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**RotaryEncoder**](dir_d61b64c3ddc6557ee529e3725418e11d.md) **>** [**RotaryEncoder.h**](RotaryEncoder_8h.md)

[Go to the documentation of this file](RotaryEncoder_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase, PinRef

namespace OpenSkyhawk {

enum class EncoderStepsPerDetent : uint8_t {
    One   = 1,
    Two   = 2,
    Four  = 4,
    Eight = 8,
};

enum class EncoderMode : uint8_t {
    Rel,  
    Dir,  
};

class RotaryEncoder : public InputBase {
public:
    static constexpr int16_t DEFAULT_STEP = 3200;  

    RotaryEncoder(uint16_t controlId, PinRef pinA, PinRef pinB,
                  EncoderStepsPerDetent stepsPerDetent = EncoderStepsPerDetent::One,
                  EncoderMode mode = EncoderMode::Rel, int16_t step = DEFAULT_STEP);

    void poll() override;

    void forceReport() override;

    void configure() override;

#ifdef ROTARYENCODER_TEST
    void debugSeed(uint8_t state) { _lastState = (uint8_t)(state & 0x3); _delta = 0; _initialized = true; }
    void debugStep(uint8_t ab) { decode((uint8_t)(ab & 0x3)); }
    uint16_t emitCount() const { return _emitCount; }
    int16_t lastValue() const { return _lastValue; }
    uint32_t lastFrame() const { return _lastFrame; }
#endif

protected:
    uint8_t readState();   

private:
    void decode(uint8_t state);          
    void emit(int8_t dir);               

    uint16_t   _controlId;
    PinRef     _pinA;
    PinRef     _pinB;
    EncoderMode _mode;
    int16_t    _step;        
    uint8_t    _stepsPerDetent;
    uint8_t    _lastState;   
    int8_t     _delta;       
    bool       _initialized; 
#ifdef ROTARYENCODER_TEST
    uint16_t _emitCount = 0;
    int16_t  _lastValue = 0;
    uint32_t _lastFrame = 0;
#endif
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


