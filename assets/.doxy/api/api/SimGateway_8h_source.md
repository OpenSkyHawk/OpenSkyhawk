

# File SimGateway.h

[**File List**](files.md) **>** [**Firmware**](dir_74b6a3b63f61c160c0f14b7a283a4c9b.md) **>** [**Libraries**](dir_3540c00680c2664f9f7e8f48ca1cab09.md) **>** [**SimGateway**](dir_a54aa0246e1c520ae49dfef506a428ca.md) **>** [**SimGateway.h**](SimGateway_8h.md)

[Go to the documentation of this file](SimGateway_8h.md)


```C++

#pragma once
#ifdef ARDUINO_ARCH_RP2040

#include <Arduino.h>
#include <Joystick.h>   
#include <CANProtocol.h>

namespace SimGateway {

    void setup(HardwareSerial& panelBridgePort);

    void loop();

    void send(uint16_t controlId, uint16_t value);

    void onDiagRtt(void (*cb)(uint16_t seq, uint32_t sentMs));

    void onDiagHb(void (*cb)(uint8_t nodeId, uint16_t rxCount));

    void onDiagErr(void (*cb)(uint8_t tec, uint8_t rec, uint8_t flags));

    void onDiagEvt(void (*cb)(uint16_t controlId, uint16_t value, uint8_t nodeId));

} // namespace SimGateway

#endif // ARDUINO_ARCH_RP2040
```


