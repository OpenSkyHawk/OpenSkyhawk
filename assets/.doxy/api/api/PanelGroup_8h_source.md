

# File PanelGroup.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**PanelGroup.h**](PanelGroup_8h.md)

[Go to the documentation of this file](PanelGroup_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <STM32Board.h>
#include <CANProtocol.h>

// ── Output objects — DCS state → panel hardware ───────────────────────────────

namespace OpenSkyhawk {

class OutputBase {
public:
    static OutputBase* first; 
    OutputBase* next;         

    OutputBase();

    virtual void onPacket(uint16_t controlId, uint16_t value) = 0;
};

class LED : public OutputBase {
    uint16_t addr_; 
    uint16_t mask_; 
    uint8_t  pin_;  
public:
    LED(uint16_t addr, uint16_t mask, uint8_t pin);

    void onPacket(uint16_t controlId, uint16_t value) override;
};

class IntegerOutput : public OutputBase {
    uint16_t addr_;       
    void (*cb_)(uint16_t); 
public:
    IntegerOutput(uint16_t addr, void (*cb)(uint16_t));

    void onPacket(uint16_t controlId, uint16_t value) override;
};

// ── Input objects — panel hardware → DCS via CAN ─────────────────────────────

class InputBase {
public:
    static InputBase* first; 
    InputBase* next;         

    InputBase();

    virtual void poll() = 0;
};

class Switch2Pos : public InputBase {
    uint16_t addr_;       
    uint8_t  pin_;        
    bool     lastStable_; 
    bool     lastRaw_;    
    uint32_t debounceMs_; 
public:
    Switch2Pos(uint16_t addr, uint8_t pin);

    void poll() override;
};

} // namespace OpenSkyhawk

// ── PanelGroup singleton ──────────────────────────────────────────────────────

namespace PanelGroup {

    void setup();

    void loop();

    bool sendEvent(uint16_t controlId, uint16_t value);

    uint8_t nodeId();

} // namespace PanelGroup

#endif // ARDUINO_ARCH_STM32
```


