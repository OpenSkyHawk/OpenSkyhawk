

# File ActionButton.h

[**File List**](files.md) **>** [**ActionButton**](dir_480a13d53392c311de80938128d7c5e3.md) **>** [**ActionButton.h**](ActionButton_8h.md)

[Go to the documentation of this file](ActionButton_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase, PinRef

namespace OpenSkyhawk {

class ActionButton : public InputBase {
public:
    static constexpr uint32_t DEBOUNCE_MS = 20;

    ActionButton(uint16_t controlId, PinRef pin, bool reverse = false);

    void poll() override;

    void forceReport() override;

    void configure() override;

    // sampleTick() is deliberately not overridden. RotaryEncoder needs it because quadrature
    // transitions can be lost between loop iterations; a button press lasts 50–100 ms, far longer
    // than any loop period, so the cached read is always current enough.

private:
    bool readPressed() const;

    uint16_t _controlId;
    PinRef   _pin;
    bool     _reverse;        // true = active-HIGH (external pull-down required)
    bool     _lastPressed;    // last observed level, in press-polarity
    uint32_t _lastFireMs;     // millis() of the last emit — debounce reference
    bool     _initialized;    // false until forceReport() is called; poll() no-op before this
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


