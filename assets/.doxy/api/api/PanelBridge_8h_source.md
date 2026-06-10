

# File PanelBridge.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**PanelBridge**](dir_f592a3c441b32532ba8eb6b28add2a90.md) **>** [**PanelBridge.h**](PanelBridge_8h.md)

[Go to the documentation of this file](PanelBridge_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_STM32

#include <Arduino.h>
#include <STM32Board.h>
#include <CANProtocol.h>

namespace PanelBridge {

    void setup(HardwareSerial& uartPort);

    void loop();

    void onNodeAlive(void (*cb)(uint8_t nodeId));

    void onNodeDead(void (*cb)(uint8_t nodeId));

} // namespace PanelBridge

#endif // ARDUINO_ARCH_STM32
```


