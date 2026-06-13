

# File PinRef.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**PinRef.h**](PinRef_8h.md)

[Go to the documentation of this file](PinRef_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>

// Forward declarations — PinRef.h stores pointers; full types are not needed here.
// Complete definitions: MCP23017 via <MCP23017.h>, ADS1115 via <ADS1115.h>.
class MCP23017;  
class ADS1115;   

static constexpr uint8_t PORT_A = 0;  
static constexpr uint8_t PORT_B = 1;  

class PinRef {
public:

    // ── Constructors ──────────────────────────────────────────────────────────────

    explicit PinRef(uint8_t pin);

    PinRef(MCP23017& chip, uint8_t port, uint8_t bit);

    PinRef(ADS1115& adc, uint8_t channel);

    PinRef();

    // ── Interface ─────────────────────────────────────────────────────────────────

    bool read() const;

    uint16_t readAnalog() const;

    void write(bool value);

    void writeAnalog(uint16_t val);

    void configureAsInput();

    void configureAsOutput();

    bool isNC() const;

    bool isGpio() const;

    uint8_t gpioPin() const;

private:
    enum class Type : uint8_t {
        GPIO,  
        MCP,   
        ADS,   
        NC,    
    };

    Type _type;  

    union {
        uint8_t pin;                                                
        struct { MCP23017* chip; uint8_t port; uint8_t bit; } mcp; 
        struct { ADS1115*  adc;  uint8_t channel;           } ads; 
    } _src;
};

extern const PinRef PIN_NC;

#endif // ARDUINO_ARCH_STM32
```


