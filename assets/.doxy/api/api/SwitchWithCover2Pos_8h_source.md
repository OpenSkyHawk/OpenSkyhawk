

# File SwitchWithCover2Pos.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**Inputs**](dir_2e07d2b82251b5bb8c3d5a17dd64c04b.md) **>** [**SwitchWithCover2Pos**](dir_0a1cdbbabc21d9a1747f90460cf2cff5.md) **>** [**SwitchWithCover2Pos.h**](SwitchWithCover2Pos_8h.md)

[Go to the documentation of this file](SwitchWithCover2Pos_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Inputs/Switch2Pos/Switch2Pos.h>  // family base (InputBase, PinRef via PanelGroup.h)

namespace OpenSkyhawk {

class SwitchWithCover2Pos : public Switch2Pos {
public:
    static constexpr uint16_t COVER_DELAY_MS = 200;

    SwitchWithCover2Pos(uint16_t switchId, uint16_t coverId, PinRef pin, bool reverse = false);

    void poll() override;

#ifdef SWITCHWITHCOVER_TEST
    uint8_t state() const { return (uint8_t)_state; }
    uint8_t target() const { return (uint8_t)_target; }
#endif

protected:
    void emit(bool active, bool init) override;

private:
    enum class Cover : uint8_t { OFF_CLOSED = 0, OFF_OPEN = 1, ON_OPEN = 2 };

    void step();                    
    void sendCover(bool open);
    void sendSwitch(bool on);

    uint16_t _coverId;
    Cover    _state;                
    Cover    _target;               
    uint32_t _lastStepMs;           
};

}  // namespace OpenSkyhawk

#endif  // ARDUINO_ARCH_STM32
```


