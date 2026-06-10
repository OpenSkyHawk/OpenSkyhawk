

# File STM32Board.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**STM32Board**](dir_aa1816754c0645981f9c7af905857f7d.md) **>** [**STM32Board.h**](STM32Board_8h.md)

[Go to the documentation of this file](STM32Board_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <stm32f1xx_hal_can.h>

// CanStatus is owned by CANProtocol; forward-declared here so onCanStatus()
// can accept it without pulling the full CANProtocol header into every user.
enum class CanStatus;

static_assert(NODE_ID <= 63,
    "NODE_ID must be 0-63. 0 is reserved for PanelBridge; 1-63 for PanelGroup nodes.");

namespace STM32Board {

    static constexpr uint8_t PIN_LED_RED   = PB14; 
    static constexpr uint8_t PIN_LED_GREEN = PB15; 

    void begin();

    void setDebug(bool on);

    void tick();

    void onCanStatus(CanStatus status);

    bool isDebug();

    void log(const char* msg);

    HardwareSerial& diagSerial();

    CAN_HandleTypeDef* canHandle();

    // ── Deprecated — will be removed when CANProtocol is implemented ──────────
    // These remain temporarily so PanelGroup and PanelBridge compile unchanged
    // until the CANProtocol PR migrates those calls.

    void canStart();

    bool canSend(uint32_t canId, const uint8_t* data, uint8_t len);

    uint8_t tec();

    uint8_t rec();

    bool busOff();

} // namespace STM32Board

#endif // ARDUINO_ARCH_STM32
```


