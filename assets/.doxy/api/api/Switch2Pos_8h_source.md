

# File Switch2Pos.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**Switch2Pos**](dir_498c816b3a939baf976ad59345a9b3b2.md) **>** [**Switch2Pos.h**](Switch2Pos_8h.md)

[Go to the documentation of this file](Switch2Pos_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <PanelGroup.h>  // InputBase, PinRef

namespace OpenSkyhawk {

class Switch2Pos : public InputBase {
public:
    static constexpr uint16_t DEBOUNCE_MS = 20;

    Switch2Pos(uint16_t controlId, PinRef pin);

    Switch2Pos(uint16_t controlId, PinRef pin, bool reverse);

    void poll() override;

    void forceReport() override;

    void configure() override;

private:
    uint16_t _controlId;
    PinRef   _pin;
    bool     _reverse;          // true = active-HIGH (external pull-down required)
    bool     _lastConfirmed;    // last emitted state (true = active)
    bool     _pendingRaw;       // raw reading at the last level change
    uint32_t _debounceStartMs;  // millis() when _pendingRaw last changed
    bool     _initialized;      // false until forceReport() is called; poll() no-op before this
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


