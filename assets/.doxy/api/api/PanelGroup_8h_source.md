

# File PanelGroup.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelGroup**](dir_54a06c409a6161127d200302d3061b3f.md) **>** [**PanelGroup.h**](PanelGroup_8h.md)

[Go to the documentation of this file](PanelGroup_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <Wire.h>
#include <MCP23017.h>
#include "ADS1115.h"
#include "PinRef.h"
#include <CANProtocol.h>

// ── OpenSkyhawk base classes ──────────────────────────────────────────────────

namespace OpenSkyhawk {

class InputBase {
public:
    virtual void configure() {}

    virtual void poll() = 0;

    virtual void forceReport() = 0;

    static InputBase* head();

    InputBase* next() const;

protected:
    InputBase();  

private:
    static InputBase* _head;
    InputBase* _next;
};

class OutputBase {
public:
    virtual void configure() {}

    virtual void onControlPacket(uint16_t controlId, uint16_t value) = 0;

    virtual void update() {}

    static OutputBase* head();

    OutputBase* next() const;

protected:
    OutputBase();  

private:
    static OutputBase* _head;
    OutputBase* _next;
};

} // namespace OpenSkyhawk

// ── PanelGroup namespace — sketch-facing API ──────────────────────────────────

namespace PanelGroup {

    void registerADC(ADS1115& adc, uint8_t addr = 0x48, TwoWire& wire = Wire);

    void registerExpander(MCP23017& chip, uint8_t intaPin, uint8_t intbPin);

    void registerExpander(MCP23017& chip);

    void setup();

    void loop();

    // ── Package-internal — MCP cache bridge for PinRef ───────────────────────
    //
    // Not sketch API. Called exclusively by PinRef::read() and PinRef::write().
    // Direct GPIO and ADS1115 PinRefs bypass this path entirely.
    //
    // MCP23017 reads go through the cache (no live I2C per poll) because:
    //   • A full readPort() at 400 kHz takes ~100 µs — too slow for every poll().
    //   • INTCAP captures port state at interrupt time; a subsequent readPort()
    //     may return an already-transitioned value, losing the captured snapshot.
    //   • All poll() calls after an interrupt must see the same captured snapshot.

    bool readCachedPin(const MCP23017& chip, uint8_t port, uint8_t bit);

    void writeCachedPin(MCP23017& chip, uint8_t port, uint8_t bit, bool value);

} // namespace PanelGroup

#endif // ARDUINO_ARCH_STM32
```


